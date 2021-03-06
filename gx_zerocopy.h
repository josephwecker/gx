/*
 * Will try to rely on basic gcc os macros but you can do better if desired
 * with real feature tests for the various HAVE_... macros.
 *   HAVE_LINUX_SENDFILE
 *   HAVE_BSD_SENDFILE
 *   HAVE_SPLICE
 *   HAVE_TEE
 *   HAVE_VMSPLICE
 *   HAVE_MMAP
 *   HAVE_MLOCK
 *   (at some point, HAVE_SOSPLICE, HAVE_SOMOVE)
 *
 *
 *
 *
 *   | from   |  to    |  use     |
 *   |--------|--------|----------|
 *   | mmap   | sock   | sendfile |
 *   | sock   | mmap   | recv ? piped-splice ? |
 *   | sock   | pipe   | splice   |
 *   | pipe   | sock   | splice   |
 *   | sock   | sock   | piped splice |
 *
 *
 *
 *   src
 *   src_offset
 *   dst
 *   dst_offset (not supported by most...)
 *   bytes_to_send (length)
 *   bytes_sent
 *   consume::bool
 *
 *   zc_mfd2sock (aka sendfile)
 *   zc_mfd
 *
 *   sock / mmap / pipe / ringbuf
 *   (peek and not)
 *
 *
 *   - handles EINTR
 *   - when return length<desired, assume EAGAIN / EWOULDBLOCK / not enough source
 *
 *   mbuf  -- mmap buffer, i.e., page-aligned etc. ready for vm_splice
 *   mmfd  -- memory-mappable or mmapped fd / file
 *   sock  -- inet or unix-domain socket
 *   pipe  -- yeah
 *   rbuf  -- gx_ringbuf
 *   null  -- oblivion (or zeros if a src)
 *
 */

#ifndef GX_ZEROCOPY_H
#define GX_ZEROCOPY_H

#include "./gx.h"

#if defined(__LINUX__)
  #ifndef _GNU_SOURCE
    #define _GNU_SOURCE 1
  #endif
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/sendfile.h>
#elif defined(__OSX__)
  #include <sys/uio.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include "./gx_ringbuf.h"

/// All return number of bytes transfered or -1 if fatal error. If zero
/// or less then desired was transfered, it could be because of EAGAIN, several
/// EINTRs, or because there was nothing to transer (such as EOF). Can't rely
/// on 0 to mean that EOF was reached.

//                                    | Source    | Source offset | Xfr amt   |Destination | Dest. offset  | Consume?   |
//------------------------------------|-----------|---------------|-----------|------------|---------------|------------|
static optional inline ssize_t zc_rbuf_sock (gx_rb *rbuf,                             int    sock,                 int consume);
static optional inline ssize_t zc_rbuf_null (gx_rb *rbuf,                 size_t len                                          );
static optional inline ssize_t zc_rbuf_mmfd (gx_rb *rbuf,                 size_t len, int    mmfd                             );
static optional inline ssize_t zc_rbuf_mmfd2(gx_rb *rbuf,                 size_t len, int    mmfd,                 int consume);

static optional inline ssize_t zc_rbuf_sock2(gx_rb *rbuf, size_t src_off, size_t len, int    sock,                 int consume);

static optional inline ssize_t zc_mbuf_sock (void  *mbuf, size_t src_off, size_t len, int    sock                             );

static optional inline ssize_t zc_mmfd_sock (int    mmfd, size_t src_off, size_t len, int    sock                             );

static optional inline ssize_t zc_sock_null (int    sock,                 size_t len                                          );
static optional inline ssize_t zc_sock_mmfd (int    sock,                 size_t len, int    mmfd,                 int consume);
static optional inline ssize_t zc_sock_sock (int    in  ,                 size_t len, int    out ,                 int consume);
static optional inline ssize_t zc_sock_rbuf (int    sock,                 size_t len, gx_rb *rbuf,                 int consume);
//static optional ssize_t zc_sock_rbuf (int    sock,                 size_t len, gx_rb *rbuf, size_t dst_off, int consume);

/// Just like sendfile, but with a ringbuffer instead of file
static optional inline ssize_t zc_rbuf_sock2(gx_rb *rbuf, size_t src_off, size_t len, int sock, int consume) {
    ssize_t res = zc_mmfd_sock(rbuf->fd, rbuf->r + src_off, len, sock);
    if(consume) rb_advr(rbuf, len);
    return res;
}

/// Assume portion of ringbuffer that hasn't been read yet.
static optional inline ssize_t zc_rbuf_sock(gx_rb *rbuf, int sock, int consume) {
    return zc_rbuf_sock2(rbuf, 0, rb_used(rbuf), sock, consume);
}

/// Send from a buffer straight to the socket. Not much to optimize at the
/// moment.
static optional ssize_t zc_mbuf_sock(void *mbuf, size_t src_off, size_t len, int sock) {
    return send(sock, mbuf + src_off, len, 0);
}

/// AKA sendfile
/// TODO: header/footer like bsd implementation- esp. if it makes sense for the
///       other zero-copy functions.
static optional inline ssize_t zc_mmfd_sock(int mmfd, size_t src_off, size_t len, int sock) {
  #if defined(__LINUX__)
    int     tries=1;
    ssize_t sent;
    off_t   src_off2 = src_off;
sendagain:
    sent = sendfile(sock, mmfd, &src_off2, len);
    if(sent == -1) {
        switch(errno) {
            case EINTR:
                if(tries++ < 3) goto sendagain;
                return src_off2 - src_off;
            case EAGAIN: return src_off2 - src_off;
            default:     return -1;
        }
    }
    return sent;
  #elif defined(__OSX__)
    int tries=1;
    off_t total_sent=0;
    off_t sent = len;
    int res;
sendagain:
    res = sendfile(mmfd, sock, src_off, &sent, NULL, 0);
    total_sent += sent;
    if(res==-1) {
        switch(errno) {
            case EINTR:
                if(tries++ < 3) {
                    // Unlike linux- some data may have been sent
                    src_off += sent;
                    sent = len - total_sent;
                    goto sendagain;
                }
                return total_sent;
            case EAGAIN: return total_sent;
            default:     return -1;
        }
    }
    return total_sent;
  #else
    #error "Only implemented at the moment for OSX and Linux"
  #endif
}

/// Discard len bytes of a ringbuffer
static optional inline ssize_t zc_rbuf_null(gx_rb *rbuf, size_t len) {
    rb_advr(rbuf, len);
    if(rbuf->r > rbuf->w) rb_clear(rbuf);
    return len;
}

static optional inline ssize_t zc_rbuf_mmfd2(gx_rb *rbuf, size_t len, int mmfd, int consume) {
    ssize_t sent;
do_write:
    switch_esys(sent = write(mmfd, rb_r(rbuf), len)) {
        case EINTR: goto do_write;
        case EAGAIN: errno = 0; return 0;
        default: _raise(-1);
    }
    if (consume) {
        rb_advr(rbuf, sent);
        if(rbuf->r > rbuf->w) rb_clear(rbuf);
    }
    return sent;
}

static optional inline ssize_t zc_rbuf_mmfd(gx_rb *rbuf, size_t len, int mmfd) {
    return zc_rbuf_mmfd2(rbuf, len, mmfd, 1);
}


/// @todo For linux implementation, when I get to it finally, I'll need the
/// calling program to maintain a struct that holds the "entry" pipes (similar
/// to the static global ones I'm using for sock_null). Then the implementation
/// is very straightforward- similar to:
/// http://blog.superpat.com/2010/06/01/zero-copy-in-linux-with-sendfile-and-splice/
///
static optional inline ssize_t zc_sock_mmfd (int sock, size_t len, int mmfd, int consume) {
    int     tries = 0;
    uint8_t tmp_buf[4096];
    size_t  sent = 0, remaining;
    ssize_t just_sent;
    int     rflags = (consume ? 0 : MSG_PEEK) | MSG_DONTWAIT;

    do {
        remaining = len - sent;
        switch_esys(just_sent = recv(sock, tmp_buf, min(remaining, 4096U), rflags)) {
            case EAGAIN: return sent;
            case EINTR:  if(tries++ < 2) continue;
            default:     _raise(-1);
        }
        if(just_sent > 0) {
do_file_write:
            switch_esys(write(mmfd, tmp_buf, just_sent)) {
                case EINTR:  goto do_file_write;
                case EAGAIN: if(tries++ < 2) goto do_file_write; // Should never happen, at least on linux
                default: _raise(-1);
            }
            sent += just_sent;
        }
    } while(sent < len && (just_sent > 0 || tries));
    return sent;
}

#ifdef __LINUX__
static optional int zc_general_pipes[2], zc_pipe_in, zc_pipe_out;
static optional int zc_devnull_fd = -1;
#endif

/// Discard len bytes from a socket. Who would have thought it would be so much
/// code? Haven't done mac version yet even...
static optional ssize_t zc_sock_null(int sock, size_t len) {
// @todo rewrite this so it actually works on linux--SPLICE_F_MOVE seems to have disappeared
// #ifdef __LINUX__
#if 0
    int     tries = 1;
    ssize_t res;
    size_t  total_sent = 0;
    //loff_t  devnull_off = 0;
    size_t  len_to_send = len;
    size_t  just_sent;

    // Initialize devnull sinkpoint
    if(zc_devnull_fd == -1) {
        _ (zc_devnull_fd = open("/dev/null",O_WRONLY)) _raise_error(-1);
        _ (pipe2(zc_general_pipes, O_NONBLOCK)       ) _raise_error(-1);
        zc_pipe_in  = zc_general_pipes[1];
        zc_pipe_out = zc_general_pipes[0];
    }
sendagain:
    // TODO: make this a _separate_ zc call so it can handle EINTR etc.
    //       appropriately.
    // TODO: automatically break it down if too big, because we can't use
    //       devnull_off with a pipe- gives "ESPIPE" error
    res = splice(sock, NULL, zc_pipe_in, NULL, len_to_send, SPLICE_F_MOVE);
    //just_sent = max(res, devnull_off);
    just_sent = max(res, 0);
    total_sent += just_sent;
    if(res == -1) {
        switch(errno) {
            case EAGAIN: return total_sent;
            case EINTR:  break;
            default: return -1;
        }
    }
    if(freq(just_sent > 0)) {
        // TODO: make this a _separate_ zc call so it can handle EINTR etc. appropriately.
        _ (splice(zc_pipe_out, NULL, zc_devnull_fd, NULL, just_sent, SPLICE_F_MOVE)) _raise_error(-1);
    }
    if(tries++ < 3 && total_sent < len) {
        len_to_send = len - total_sent;
        goto sendagain;
    }
    return total_sent;
#else
    int     tries = 1;
    uint8_t devnull_buf[4096];
    size_t  sent = 0, remaining;
    ssize_t just_sent;

    do {
        remaining = len - sent;
        just_sent = recv(sock, devnull_buf, min(remaining, 4096U), 0);
        if(just_sent == -1) {
            if(errno == EINTR && tries++ < 3) continue;
            if(errno == EAGAIN) return sent;
            return -1;
        }
        sent += just_sent;
    } while(sent < len);
    return sent;
#endif
}
static optional inline ssize_t zc_sock_sock (int in, size_t len, int out, int consume) {
    int     tries = 0;
    uint8_t tmp_buf[4096];
    size_t  sent = 0, remaining;
    ssize_t just_sent;
    int     rflags = (consume ? 0 : MSG_PEEK) | MSG_DONTWAIT;

    do {
        remaining = len - sent;
        switch_esys(just_sent = recv(in, tmp_buf, min(remaining, 4096U), rflags)) {
            case EAGAIN: continue;
            case EINTR:  if(tries++ < 2) continue;
            default:     _raise(-1);
        }
        if(just_sent > 0) {
do_file_write:
            switch_esys(write(out, tmp_buf, just_sent)) {
                case EINTR:  goto do_file_write;
                case EAGAIN: goto do_file_write;
                default: _raise(-1);
            }
            sent += just_sent;
        }
    } while(sent < len && (just_sent > 0 || tries));
    return sent;
}
static optional inline ssize_t zc_sock_rbuf (int sock, size_t len, gx_rb *rbuf, int optional consume) {
    ssize_t rcvd;
    do {
        switch_esys(rcvd = recv(sock, rb_w(rbuf), len, 0)) {
            case EAGAIN: rcvd=0; errno=0; break;
            case EINTR:  rcvd=0; errno=0; continue;
            default: _raise_error(-1);
        }
    } while(0);
    rb_advw(rbuf, rcvd);
    return rcvd;
}

#endif

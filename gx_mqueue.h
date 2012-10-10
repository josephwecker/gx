#ifndef _GX_MQUEUE_H
#define _GX_MQUEUE_H

/**
   @file      gx_mqueue.h
   @brief     Some extensions to posix message queues (like scatter io) and pipe implementation fallback for OSX
   @author    Achille Roussel <achille.roussel@gmail.com>
   @copyright
     Except where otherwise noted, Copyright (C) 2012 Joseph A Wecker

     MIT License

     Permission is hereby granted, free of charge, to any person obtaining a
     copy of this software and associated documentation files (the "Software"),
     to deal in the Software without restriction, including without limitation
     the rights to use, copy, modify, merge, publish, distribute, sublicense,
     and/or sell copies of the Software, and to permit persons to whom the
     Software is furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in
     all copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
     THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
     DEALINGS IN THE SOFTWARE.


   @details

*/

#include "gx.h"
#include <fcntl.h>
#ifdef __LINUX__
# include <mqueue.h>
#else
# include <unistd.h>
# include <string.h>
# include <stdio.h>

// Mirrors the Linux behavior, if the name format is invalid return ENOENT.
#define GX_BSD_MQUEUE_PATH(var_name, mqueue_name)                       \
  char var_name[256] = { 0 };                                           \
                                                                        \
  if(   (mqueue_name[0] != '/')                                         \
     || (mqueue_name[1] == '\0')                                        \
     || (strchr(mqueue_name + 1, '/') != NULL)) {                       \
      errno = ENOENT;                                                   \
      return -1;                                                        \
  }                                                                     \
                                                                        \
  snprintf(var_name, sizeof(var_name), "/tmp%s", mqueue_name)

#endif

// The BSD implementation uses a named pipe for now. In order to implement a
// similar message boundary system as the one provided by the Linux message
// queues GX writes the size of the message before the message body.

#ifndef GX_MQUEUE_MSG_COUNT
#define GX_MQUEUE_MSG_COUNT 10
#endif

#ifndef GX_MQUEUE_MSG_SIZE
#define GX_MQUEUE_MSG_SIZE 8192
#endif

// ???
static char _gx_msg_stage[8192] = {0};

static int gx_mq_open(const char *name, int oflags, int mode) {
    int mqfd;

#if __LINUX__
    struct mq_attr attr;

    if(oflags & O_RDWR) {
        errno = EINVAL;
        return -1;
    }

    attr.mq_flags   = 0;
    attr.mq_maxmsg  = GX_MQUEUE_MSG_COUNT;
    attr.mq_msgsize = GX_MQUEUE_MSG_SIZE;
    attr.mq_curmsgs = 0;
    _ (mqfd = mq_open(name, oflags, mode, &attr)) _raise(-1);

#else
    GX_BSD_MQUEUE_PATH(path, name);

    if(oflags & O_CREAT) {
        _ (mkfifo(path, mode)) {
            if(oflags & O_EXCL) {
                _raise(-1);
            }
        }
    }

    _ (mqfd = open(path, oflags & ~(O_CREAT), mode)) _raise(-1);

#endif
    return mqfd;
}

static int gx_mq_recv(int mqfd, void *buffer, int nbytes, unsigned *prio) {
    int n;

#if __LINUX__
    _ (n = mq_receive(mqfd, (char *)buffer, nbytes, prio)) _raise(-1);

#else
    int a = 0;
    int c = 0;

    *prio = 0; // Priorities aren't supported on BSD systems.

    if(nbytes < GX_MQUEUE_MSG_SIZE) {
        errno = EINVAL;
        return -1;
    }

    n = 0;
    do {
        _ (c = read(mqfd, ((char *)&a) + n, sizeof(a) - n)) {
            if(errno == EINTR)  { continue;   }
            if(errno != EAGAIN) { _raise(-1); }
            else if(n == 0)     { return -1;  } // No data- pipe is probably empty, propagate EAGAIN to the caller.
            else                { continue;   }
        }
        if(c == 0) {
          // The pipe was closed or removed, no more data could be read.
          return 0;
        }
        n += c;
    } while(n != sizeof(a));

    n = 0;
    do {
        switch_esys(c = read(mqfd, ((char *)buffer) + n, a - n)) {
            case EINTR:
            case EAGAIN: continue; // No data but we're expecting 'a' bytes soon- so try again.
            default:     _raise(-1);
        }
        n += c;
    } while(n != a);

#endif
    return n;
}

static int gx_mq_send(int mqfd, const void *buffer, int nbytes, unsigned prio) {
    int n;

#if __LINUX__
    _ (n = mq_send(mqfd, (const char *)buffer, nbytes, prio)) _raise(-1);

#else
    int c;

    // We build a the message which is intended to be sent to the pipe, this is
    // a little stack and time consuming but makes the sending operation easier,
    // it could be optimized in the future.
    char msg[GX_MQUEUE_MSG_SIZE + sizeof(nbytes)];
    memmove(msg, &nbytes, sizeof(nbytes));
    memmove(msg + sizeof(nbytes), buffer, nbytes);

    (void)prio; // Priorities aren't supported on BSD systems.

    nbytes += sizeof(nbytes);
    do {
      switch_esys(c = write(mqfd, msg + n, nbytes - n)) {
          case EINTR:  continue;
          case EAGAIN: if(n == 0) return -1; continue;
              // No data could be sent yet, we return -1 and errno is set to
              // EAGAIN so if the file descriptor was associated with a kqueue
              // object it will be notified once more data can be sent to the
              // pipe.
          default:     _raise(-1);
      }
      n += c;
    } while(n != nbytes);

#endif
    return n;
}

static int gx_mq_close(int mqfd) {
#if __LINUX__
    _ (mq_close(mqfd)) _raise(-1);
#else
    _ (close(mqfd)) _raise(-1);
#endif
    return 0;
}

static int gx_mq_unlink(const char *name) {
#if __LINUX__
    _ (mq_unlink(name)) _raise(-1);
#else
    GX_BSD_MQUEUE_PATH(path, name);
    _ (unlink(path)) _raise(-1);
#endif
    return 0;
}

#endif

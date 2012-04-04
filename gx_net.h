#ifndef GX_NET_H
#define GX_NET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <gx/gx.h>


static GX_INLINE int gx_net_tcp_listen(const char *node, const char *port) {
    int fd = -1;
    int bindres = -1;
    struct addrinfo hints;
    struct addrinfo *ainfo;
    struct addrinfo *rp;
    int optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC; /* IPv4 and IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    X (getaddrinfo(node, port, &hints, &ainfo) ) X_RAISE(-1);
    for(rp = ainfo; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(fd == -1) continue;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        bindres = bind(fd, rp->ai_addr, rp->ai_addrlen);
        if(bindres == 0) break;
    }
    freeaddrinfo(ainfo);
    X (fd                          ) X_RAISE(-1);
    X (bindres                     ) X_RAISE(-1);
    Xn(rp                          ) X_RAISE(-1);
    X (fcntl(fd,F_SETFL,O_NONBLOCK)) X_RAISE(-1);

    X (listen(fd, 65535)           ) X_RAISE(-1); /* Backlog invisibly gets truncated to max */
    return fd;
}

static GX_INLINE int gx_net_daemonize(void) {
    /* TODO: chroot option? */
    /* TODO: it should be possible to make this much lighter with clone. */
    pid_t pid, sid;
    X (pid = fork()) { X_ERROR; X_RAISE(-1); }
    if(pid != 0) exit(EXIT_SUCCESS);
    /* Child */
    umask(0);
    X (sid = setsid()) X_WARN;
    X (chdir("/")    ) X_WARN;  /* Don't bind up the working dir */

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

    Xn(freopen("/dev/null", "r", stdin))  X_WARN;
    Xn(freopen("/dev/null", "w", stdout)) X_WARN;

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic pop
#endif

    return 0;
}
#endif

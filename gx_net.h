#ifndef GX_NET_H
#define GX_NET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <gx/gx.h>


static GX_INLINE int gx_net_tcp_open(const char *host, const char *port) {
    int             ires;
    struct addrinfo hints, *servinfo, *p;
    int             sockfd;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    X (ires = getaddrinfo(host, port, &hints, &servinfo)){X_RAISE(-1)}
    for(p = servinfo; p != NULL; p = p->ai_next) {
        X (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) continue;
        X (connect(sockfd, p->ai_addr, p->ai_addrlen)) {close(sockfd); continue;}
        break;
    }
    if(!p){X_RAISE(-1)}
    freeaddrinfo(servinfo);
    return sockfd;
}


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


#if 0
static char _gx_node_agg_id_memoized[12] = {'\0'};

static GX_INLINE int gx_node_agg_id(char *buf, size_t len) {
    if(gx_likely(_gx_node_agg_id_memoized[0])) {
        memcpy
    }


    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic
           form of the latter for the common families) */

        printf("%s  address family: %d%s\n",
                ifa->ifa_name, family,
                //(family == AF_PACKET) ? " (AF_PACKET)" :
                (family == AF_INET) ?   " (AF_INET)" :
                (family == AF_INET6) ?  " (AF_INET6)" : "");

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                    sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\taddress: <%s>\n", host);
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}
#endif

#endif

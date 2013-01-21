/**
 * @todo  binary version of gx_node_uid
 */
#ifndef GX_NET_H
#define GX_NET_H

#include "./gx.h"
#include "./gx_error.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <fcntl.h>
#include "./ext/sha2.h"


static inline int gx_net_tcp_open(const char *host, const char *port) {
    int             ires;
    struct addrinfo hints, *servinfo, *p;
    int             sockfd;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    _ (ires = getaddrinfo(host, port, &hints, &servinfo)) _raise(-1);
    for(p = servinfo; p != NULL; p = p->ai_next) {
        _ (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) continue;
        _ (connect(sockfd, p->ai_addr, p->ai_addrlen)) {close(sockfd); continue;}
        break;
    }
    if(!p) _raise(-1);
    freeaddrinfo(servinfo);
    return sockfd;
}

/// @todo: Use gai_strerror combined w/ gx_error when something goes wrong w/ getaddrinfo
static inline int gx_net_tcp_listen(const char *node, const char *port, char *bound_node, size_t bound_node_len) {
    int              fd      = -1;
    int              bindres = -1;
    int              optval  =  1;
    struct addrinfo  hints, *ainfo, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC; /* IPv4 and IPv6 */
    hints.ai_socktype = SOCK_STREAM;
#ifdef __LINUX__
    hints.ai_flags    = AI_PASSIVE | AI_ALL | AI_ADDRCONFIG;
#else // AI_CANONNAME seems messed up on linux at the moment. yrmv
    hints.ai_flags    = AI_PASSIVE | AI_ALL | AI_ADDRCONFIG | AI_CANONNAME;
#endif

    _ (getaddrinfo(node, port, &hints, &ainfo) ) {log_critical("%s", gai_strerror(errno)); _raise(-1);}
    for(rp = ainfo; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(fd == -1) continue;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        bindres = bind(fd, rp->ai_addr, rp->ai_addrlen);
        if(bindres == 0) break;
    }
    _ (fd                          ) _goto(gx_tcp_listen_err);
    _ (bindres                     ) _goto(gx_tcp_listen_err);
    _N(rp                          ) _goto(gx_tcp_listen_err);
    _ (fcntl(fd,F_SETFL,O_NONBLOCK)) _goto(gx_tcp_listen_err);/// @todo close/delete socket before raising error
    _ (listen(fd, 65535)           ) _goto(gx_tcp_listen_err); // Backlog invisibly gets truncated to max-allowed

    size_t len = 0;
    if(rp->ai_canonname) {
        strncpy(bound_node, rp->ai_canonname, bound_node_len - 1);
        len = strlen(bound_node);
        if(len < bound_node_len - 3) {
            bound_node[len]   = ':';
            bound_node[len+1] = ' ';
            len += 2;
        }
    }
    if(len < bound_node_len - 1)
        inet_ntop(rp->ai_family, rp->ai_addr->sa_data + 2, &(bound_node[len]), bound_node_len - len - 1);

    freeaddrinfo(ainfo);
    return fd;

gx_tcp_listen_err:
    _error();
    freeaddrinfo(ainfo);
    close(fd);
    _raise(-1);
}

static inline int gx_net_daemonize(void) {
    /* TODO: chroot option? */
    /* TODO: it should be possible to make this much lighter with clone. */
    pid_t pid, sid;
    _ (pid = fork()) _raise_error(-1);
    if(pid != 0) return pid; //exit(EXIT_SUCCESS);
    /* Child */
    umask(0);
    _ (sid = setsid()) _warning();
    _ (chdir("/")    ) _warning();  /* Don't bind up the working dir */

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

    _N(freopen("/dev/null", "r", stdin))  _warning();
    _N(freopen("/dev/null", "w", stdout)) _warning();

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic pop
#endif

    return 0;
}


#ifdef __LINUX__
#include <sys/ioctl.h>
#include <net/if.h>
#else
#include <net/if_dl.h>
#endif

#define GX_NODE_UID_LEN (SHA256_DIGEST_STRING_LENGTH+1)
static char _gx_node_uid_memoized[GX_NODE_UID_LEN] = "";
static int  _gx_node_uid_is_memoized = 0;

static inline int gx_node_uid(char *buf) {
    if(rare(!_gx_node_uid_is_memoized)) {
        char             rawdat[1024];
        size_t           len = 0;
        struct ifaddrs  *ifaddr, *ifa;

        log_debug("Constructing node uid for the first time.");
        _ (getifaddrs(&ifaddr)) _raise(-1);
        for(ifa=ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
          #ifdef __LINUX__
            struct ifreq ifinfo;
            int sd, res;
            strcpy(ifinfo.ifr_name, ifa->ifa_name);
            _ (sd = socket(AF_INET, SOCK_DGRAM, 0)   ) _warning();
            _ (res= ioctl(sd, SIOCGIFHWADDR, &ifinfo)) _warning();
            close(sd);

            if ((res == 0) && (ifinfo.ifr_hwaddr.sa_family == 1)) {
                memcpy(rawdat + len, ifinfo.ifr_hwaddr.sa_data, IFHWADDRLEN);
                len += IFHWADDRLEN;
            }
          #else
            if ((ifa->ifa_addr->sa_family == AF_LINK) && ifa->ifa_addr) {
                struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifa->ifa_addr;
                memcpy(rawdat + len, LLADDR(sdl), sdl->sdl_alen);
                len += sdl->sdl_alen;
                rawdat[len++] = '|';
            }
          #endif

            int family = ifa->ifa_addr->sa_family;
            if(family == AF_INET) {
                size_t sz = 4;
                memcpy(rawdat + len,
                        &(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr), sz);
                len += sz;
            } else if(family == AF_INET6) {
                size_t sz = 16;
                memcpy(rawdat + len,
                        &(((struct sockaddr_in6 *)(ifa->ifa_addr))->sin6_addr.s6_addr), sz);
                len += sz;
            }
        }
        rawdat[len] = '\0';
        freeifaddrs(ifaddr);
        SHA256_Data((sha2_byte *)rawdat, len, _gx_node_uid_memoized);
        _gx_node_uid_memoized[GX_NODE_UID_LEN-1] = '\0';
        _gx_node_uid_is_memoized = 1;
    }
    memcpy(buf, _gx_node_uid_memoized, GX_NODE_UID_LEN);
    return 0;
}


#endif

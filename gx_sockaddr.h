/**
 * @file gx_sockaddr.h
 *
 * @author Achille Roussel achille.roussel@gmail.com
 */

#ifndef GX_SOCKADDR_H
#define GX_SOCKADDR_H

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @addtogroup inaddr
 *
 * @details Gx supports both IPv4 and IPv6, in order to make it easy to deal
 * with IP addresses not matter what their type is the system structures for
 * representing addresses are abstracted in this union.
 *
 * @{
 */

/**
 * @brief Internet Protocol address.
 */
typedef union gx_sockaddr {
    struct sockaddr     sin;
    struct sockaddr_in  sin4;
    struct sockaddr_in6 sin6;
} gx_sockaddr;

/**
 * @brief Sets an IP address to the given values.
 *
 * @details Network end points are represented by two components, the address
 * and the port. Adresses are groups of integers which can be represented by
 * strings (in the IPv4 or IPv6 formats according to the address type) and the
 * port is a single numeric value which is system-dependent.
 *
 * @param sa A pointer to the socket address object to set (must not be
 * <tt>NULL</tt>).
 *
 * @param addr A string-representation of the network address (must not be
 * <tt>NULL</tt>).
 *
 * @param port An integer representing the network port (must not be negative).
 *
 * @return The function return <em>true</em> or <em>false</em> whether the call
 * succeeded or failed.
 *
 * @retval true The given arguments were valid and the socket address object
 * has been modified.
 *
 * @retval false The given arguments were invalid and not modifications were
 * made to the socket address object.
 *
 * @see gx_sockaddr_get
 * @see gx_sockaddr_length
 */
static inline int gx_sockaddr_set(gx_sockaddr *sa, const char *addr, int port)
{
    memset(sa, 0, sizeof(*sa));

    if(inet_pton(AF_INET, addr, &(sa->sin4.sin_addr))) {
        sa->sin4.sin_family = AF_INET;
        sa->sin4.sin_port = htons(port);
        return 0;
    }

    if(inet_pton(AF_INET6, addr, &(sa->sin6.sin6_addr))) {
        sa->sin6.sin6_family = AF_INET6;
        sa->sin6.sin6_port = htons(port);
        return 0;
    }

    errno = EINVAL;
    return -1;
}

/**
 * @brief Gets a string representation of the given socket address.
 *
 * @details Socket addresses can be represented by concatenation of the network
 * address and the port separated by a ':' character. For example the socket
 * address "127.0.0.1" on port 80 will be represented "127.0.0.1:80".
 *
 * @param sa A pointer to the socket address object to get a string
 * representation of (must not be <tt>NULL</tt>).
 *
 * @param dst The destination buffer where the string will be written (must not
 * be <tt>NULL</tt>).
 *
 * @param size The size (in bytes) of the destintation buffer.
 *
 * @see gx_sockaddr_set
 * @see gx_sockaddr_length
 */
static inline int gx_sockaddr_get(const gx_sockaddr *sa, char *dst, size_t size)
{
    size_t len;

    if(size < (INET6_ADDRSTRLEN + 1)) {
        errno = EINVAL;
        return -1;
    }

    if(sa->sin.sa_family == AF_INET) {
        inet_ntop(AF_INET, &(sa->sin4.sin_addr), dst, size);
        len = strlen(dst);
        snprintf(dst + len, size - len, ":%i", (int)ntohs(sa->sin4.sin_port));
        return 0;
    }

    if(sa->sin.sa_family == AF_INET6) {
        inet_ntop(AF_INET6, &(sa->sin6.sin6_addr), dst, size);
        len = strlen(dst);
        snprintf(dst + len, size - len, ":%i", (int)ntohs(sa->sin6.sin6_port));
        return 0;
    }

    errno = EINVAL;
    return -1;
}

/**
 * @brief Gives the length (in bytes) of the underlying <em>struct sockaddr</em>
 * object.
 *
 * @param sa A pointer to the socket address object which we want to get the
 * length of (must not be <tt>NULL</tt>).
 *
 * @see gx_sockaddr_set
 * @see gx_sockaddr_get
 */
static inline socklen_t gx_sockaddr_length(const gx_sockaddr *sa)
{
    switch(sa->sin.sa_family) {
        case AF_INET:  return sizeof(sa->sin4);
        case AF_INET6: return sizeof(sa->sin6);
    }
    errno = EINVAL;
    return -1;
}

/**
 * @}
 */

#endif /* GX_SOCKADDR_H */

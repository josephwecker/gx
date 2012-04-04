/**
 * @file      gx_tcpserver.h
 * @brief     Abstractions for a high availability async TCP server.
 * @author    Joseph A Wecker
 * @copyright
 *   Except where otherwise noted, Copyright (C) 2012 Joseph A Wecker
 *
 *   MIT License
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *   copy of this software and associated documentation files (the "Software"),
 *   to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *   and/or sell copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 * @todo      Reincarnation with appropriate data-masage handler
 *
 *
 */


#include <gx/gx.h>
#include <gx/gx_error.h>
#include <gx/gx_ringbuf.h>
#include <gx/gx_net.h>
#include <gx/gx_event.h>

/// First, create a struct that has as its first member:
///   struct gx_tcp_sess _gx_sess;
/// Make sure it's typedef'ed to the same name as the struct name.
/// Then call gx_tcp_register_session_struct with the name of it.
//#define gx_tcp_register_session_struct(STRUCT)
    //gx_pool_init(STRUCT);


#define RD_DEVNULL -2   ///> Throw away the incoming data
#define RD_BUF     -3   ///> Give the handler a buffer pointer and length w/ the data

//static rtmp_sess_pool         *rtmp_pool;
//static gx_rb_pool             *rb_pool;
//static gx_rb                   rcvrb;

typedef struct gx_tcp_sess {
    struct gx_tcp_sess   *next;      ///< Used for resource pooling
    struct gx_tcp_sess   *prev;      ///< For occasional staleness checks
    int                   peer_fd;   ///< TCP/IP connected peer
    void                 *udata;     ///< Misc data associated w/ session
    gx_rb                *rcv_buf;   ///< Only acquired if/when necessary
    gx_rb                *snd_buf;   ///< Only acquired if/when necessary

    int                   rcv_dest;  ///< Where the next set of bytes goes
    size_t                rcv_bytes; ///< How many bytes to rcv before next handler
    int (*fn_rcv_handler) (struct gx_tcp_sess *,    uint8_t *,size_t);
    int (*fn_misc_handler)(struct gx_tcp_sess *,int,uint8_t *,size_t);
} gx_tcp_sess;
gx_pool_init(gx_tcp_sess);

struct gx_tcp_server {
    int                   ev_fd;     ///< File Descriptor for the eventloop
    gx_rb                 rcvrb;
    gx_rb_pool           *rb_pool;   ///< For misc receive/send operations
    gx_tcp_sess_pool     *sess_pool;
    int (*fn_worker_init) (int new_worker_id);
    int (*fn_sess_init)   (struct gx_tcp_sess *); ///< Also sets first handlers
};



static struct gx_tcp_server *gx_tcp_new() {

    /// @todo initialize rb_pool & sess_pool
    /// @todo initialize rcvrb
    return NULL;
}

static void gx_tcp_free(struct gx_tcp_server *gxtcp) {

}

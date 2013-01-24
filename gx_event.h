#ifndef GX_EVENT_H
#define GX_EVENT_H
//=============================================================================
/**
 * Generic fastest event handling and tcp demultiplexing for easy and lightning
 * fast layer 7 development.
 *
 *
 * Simplified Interface
 * ------------------------
 * gx_eventloop_prepare(<name>, expected-sessions, events-at-a-time)  // global
 * gx_eventloop_init(<name>) // before adding etc.- uses global state
 * <name>_add_misc(peer_fd, *misc)
 * <name>_add_sess(peer_fd, disc_handler, dest1, handler1, expected1, readahead1, *misc)
 * <name>_wait(timeout, misc_handler) // returns -1 on error, 0 on timeout
 *
 * GX_EVENT_HANDLER(name) // <name>(sess*, rb*)
 * gx_event_set_handler(sess, handler_function_name);
 * TODO: set_handler that also sets expected & destination etc.
 *
 * destination is buffer, devnull, or filedescriptor.
 * I'm thinking that there will be some utilities for creating an intermediate
 * filedescriptor that allows for broadcasting to any number of additional
 * descriptors, be they sockets or files or whatever.
 *
 *
 * Eventloop internal variables
 * --------------------------------
 * gx_eventloop_prepare(name, expected_num_sessions, events_at_a_time)
 *   To be called (for now) somewhere in global statespace- defines structs
 *   etc. Defines the following pieces- parenthases indicate that it's used
 *   internally:
 *   -  <name>_sess               - session struct (and pool-related stuff)
 *   - ( <name>_sess_pool         - preallocated session structs         )
 *   - ( <name>_events            - GX_EVENT_STRUCT for the eventloop    )
 *   - ( <name>_events_at_a_time  - int global param                     )
 *   - ( <name>_events_fd         - int primary eventloop filedescriptor )
 *   - ( <name>_expected_sessions - expected sessions                    )
 *   - ( <name>_rb_pool           - gx_rb_pool pointer                   )
 *   - ( <name>_acceptor_fd       - if specified, the fd for the listener)
 *
 *
 * Lower-level
 * --------------------
 * struct GX_EVENT_STRUCT events[max_events]
 * gx_event_newset(max_events)  --> ev_fd
 * gx_event_add     (ev_fd, peer_fd, *custom_state)  OR
 * gx_event_add_sess(ev_fd, peer_fd, dest1, handler1, expected1, readahead1, *custom_state)
 * num_fds = gx_event_wait(ev_fd, events, max_events, timeout) OR
 * num_fds = gx_event_wait_sess(ev_fd, events, max_events, timeout) -->
 *             processes all session events internally, breaking out on other
 *             events...
 * gx_event_data(events[i])
 * gx_event_states(events[i])
 *
 *
 *   |sock|-- |rbuf|
 *            |mmfd|
 *            |null|
 *            |pipe|-- |tee|-- *|sock|
 *                             *|tmp-rbuf|
 *
 */

#include <fcntl.h>
#include "./gx.h"
#include "./gx_error.h"
#include "./gx_pool.h"
#include "./gx_ringbuf.h"
#include "./gx_zerocopy.h"

#define GX_DEST_DEVNULL      -3  ///< Discard incoming data
#define GX_DEST_BUF          -2  ///< Save incoming data in a ring buffer
#define GX_DEST_UNDEF         0  ///< Not defined- handler called on every event

#define GX_CONTINUE           0  // TODO: For handler return values- change from 1/0
#define GX_SKIP               1  ///< Not quite abort... just stops processing this connection

//-------------------------------------------------------------
/// Some standard reasons passed to the disconnect callback- also use GX_ABORT
/// Feel free to define more (ideally > 0).

#define GX_CLOSED_BY_PEER     0
#define GX_ABORT             -1
#define GX_INTERNAL_ERR      -2

static char *_gx_closed_reason[] = {
    /* 0 */ "Closed by peer.",
    /* 1 */ "Aborted by us.",
    /* 2 */ "Internal error."};

typedef struct gx_tcp_sess {
    struct gx_tcp_sess   *_next, *_prev;
    int                   rcv_dest;
    gx_rb                *rcv_buf;
    int                   rcv_do_readahead;
    size_t                rcv_max_readahead; ///< 0 for as much as will fit in ringbuffer
    size_t                rcv_peek_avail;
    size_t                rcv_expected;
    size_t                rcvd_so_far;
    int                   peer_fd;
    gx_rb                *snd_buf;
    int                 (*fn_handler)    (struct gx_tcp_sess *, gx_rb *);
    int                 (*fn_disconnect) (struct gx_tcp_sess *, int);
    void                 *udata;
    #ifdef DEBUG_EVENTS
    char                 *fn_handler_name;
    #endif
} gx_tcp_sess;
gx_pool_init(gx_tcp_sess);

#define gx_eventloop_declare(NAME, EXPECTED_SESSIONS, EVENTS_AT_A_TIME)          \
    extern struct GX_EVENT_STRUCT   NAME ## _events[EVENTS_AT_A_TIME];           \
    extern int                      NAME ## _events_at_a_time;                   \
    extern int                      NAME ## _events_fd;                          \
    extern int                      NAME ## _expected_sessions;                  \
    extern gx_tcp_sess_pool       * NAME ## _sess_pool_inst;                     \
    extern gx_rb_pool             * NAME ## _rb_pool;                            \
    extern gx_rb                  * NAME ## _rcvrb;                              \
    extern int                      NAME ## _acceptor_fd;                        \
    extern int                   (* NAME ## _accept_handler)(gx_tcp_sess *);     \
                                                                                 \
    static inline int NAME ## _add_sess(int peer_fd,                                 \
            void  *misc,                                                         \
            int  (*disc_handler)(gx_tcp_sess *, int),                            \
            int    dest,                                                         \
            int  (*handler)(gx_tcp_sess *, gx_rb *),                             \
            size_t bytes_expected, int do_readahead);                            \
    inline int NAME ## _add_misc(int peer_fd, void *misc);                    \
    inline int NAME ## _add_acceptor(int afd, int(*ahandler)(gx_tcp_sess *)); \
    static inline int NAME ## _wait(int timeout,                                     \
                                int (*misc_handler)(gx_tcp_sess *, uint32_t));   \
              int NAME ## _abort_sess(gx_tcp_sess *sess);                        \
              int NAME ## _abort_sess2(gx_tcp_sess *sess,int);                   \

#define gx_eventloop_implement(NAME, EXPECTED_SESSIONS, EVENTS_AT_A_TIME)        \
    struct GX_EVENT_STRUCT   NAME ## _events[EVENTS_AT_A_TIME];                  \
    int                      NAME ## _events_at_a_time     = EVENTS_AT_A_TIME;   \
    int                      NAME ## _events_fd;                                 \
    int                      NAME ## _expected_sessions    = EXPECTED_SESSIONS;  \
    gx_tcp_sess_pool       * NAME ## _sess_pool_inst       = NULL;               \
    gx_rb_pool             * NAME ## _rb_pool              = NULL;               \
    gx_rb                  * NAME ## _rcvrb                = NULL;               \
    int                      NAME ## _acceptor_fd          = 0;                  \
    int                   (* NAME ## _accept_handler)(gx_tcp_sess *) = NULL;     \
                                                                                 \
    static inline int NAME ## _add_sess(int peer_fd,                                 \
            void *misc,                                                          \
            int  (*disc_handler)(gx_tcp_sess *, int),                            \
            int dest,                                                            \
            int (*handler)(gx_tcp_sess *, gx_rb *),                              \
            size_t bytes_expected, int do_readahead) {                           \
        gx_tcp_sess *sess;                                                       \
        _N(sess = acquire_gx_tcp_sess(NAME ## _sess_pool_inst)) _raise(-1);     \
        sess->peer_fd          = peer_fd;                                        \
        sess->rcv_buf          = NULL;                                           \
        sess->udata            = misc;                                           \
        sess->snd_buf          = NULL;                                           \
        sess->fn_disconnect    = disc_handler;                                   \
        sess->rcv_dest         = dest;                                           \
        sess->fn_handler       = handler;                                        \
        sess->rcv_expected     = bytes_expected;                                 \
        sess->rcv_do_readahead = do_readahead;                                   \
        sess->rcvd_so_far      = 0;                                              \
        return gx_event_add(NAME ## _events_fd, peer_fd,(void *)sess);           \
    }                                                                            \
    int NAME ## _abort_sess(gx_tcp_sess *sess) {                       \
        return _gx_close_sess(sess, NAME ## _sess_pool_inst, GX_ABORT, NAME ## _rb_pool); \
    }                                                                            \
    int NAME ## _abort_sess2(gx_tcp_sess *sess, int reason) {          \
        return _gx_close_sess(sess, NAME ## _sess_pool_inst, reason, NAME ## _rb_pool);   \
    }                                                                            \
    /* TODO: <name>_add_misc possibly not needed at all */                       \
    inline int NAME ## _add_misc(int peer_fd, void *misc) {                   \
        return NAME ## _add_sess(peer_fd, misc, NULL, GX_DEST_UNDEF, NULL,0,0);  \
    }                                                                            \
    /* TODO: listener disconnect handler that exits main event "wait" */         \
    inline int NAME ## _add_acceptor(int afd, int(*ahandler)(gx_tcp_sess *)) {\
        NAME ## _acceptor_fd = afd;                                              \
        NAME ## _accept_handler = ahandler;                                      \
        return gx_event_add(NAME ## _events_fd, afd,                             \
                (void *) & NAME ## _acceptor_fd);                                \
    }                                                                            \
                                                                                 \
    static inline int NAME ## _wait(int timeout,                                     \
            int (*misc_handler)(gx_tcp_sess *, uint32_t)) {                      \
        int nfds, i;                                                             \
        uint32_t evstates;                                                       \
        gx_tcp_sess *sess;                                                       \
        while(1) {                                                               \
            if(NAME ## _acceptor_fd)                                             \
                _gx_event_accept_connections(5, NAME ## _acceptor_fd,            \
                        NAME ## _accept_handler, NAME ## _sess_pool_inst,        \
                        NAME ## _events_fd, NAME ## _rb_pool,                    \
                        &NAME ## _rcvrb);                                        \
            /* TODO: a bunch of accepts here first if applicable */              \
            switch_esys(nfds = gx_event_wait(NAME ## _events_fd,                          \
                        NAME ## _events,                                         \
                        EVENTS_AT_A_TIME, timeout)) {                            \
                case EINTR: continue;                                            \
                default: _raise_alert(-1);                                       \
            }                                                                    \
            if(!nfds && timeout != -1) return 0;                                 \
            for(i=0; i < nfds; ++i) {                                            \
                if(rare(gx_event_data(NAME ## _events[i]) ==              \
                            (void *) &NAME ## _acceptor_fd)) {                   \
                    _gx_event_accept_connections(1, NAME ## _acceptor_fd,        \
                            NAME ## _accept_handler, NAME ## _sess_pool_inst,    \
                            NAME ## _events_fd, NAME ## _rb_pool,                \
                            &NAME ## _rcvrb);                                    \
                    continue;                                                    \
                }                                                                \
                sess     = (gx_tcp_sess *)gx_event_data(NAME ## _events[i]);     \
                evstates = gx_event_states(NAME ## _events[i]);                  \
                if(freq(sess->fn_handler)) {                                \
                    _gx_event_incoming(sess, evstates, NAME ## _rb_pool,         \
                            & NAME ## _rcvrb);                                   \
                    if(rare(evstates & GX_EVENT_CLOSED)) {                \
                        NAME ## _abort_sess2(sess, GX_CLOSED_BY_PEER);           \
                    }                                                            \
                } else if(misc_handler) {                                        \
                    _(misc_handler(sess, evstates)) _raise(-1);                 \
                } else return -1;                                                \
            }                                                                    \
        }                                                                        \
        return 0;                                                                \
    }


#define gx_eventloop_init(NAME) { \
    _N(NAME ## _rb_pool        = gx_rb_pool_new(NAME ## _expected_sessions/4+2,0x1000,0)) _abort();\
    _N(NAME ## _rcvrb          = gx_rb_acquire(NAME ## _rb_pool))                         _abort();\
    _N(NAME ## _sess_pool_inst = new_gx_tcp_sess_pool(NAME ## _expected_sessions))        _abort();\
    _ (NAME ## _events_fd      = gx_event_newset(NAME ## _events_at_a_time))              _abort();\
}

#define GX_EVENT_HANDLER(NAME) int NAME(optional gx_tcp_sess * sess, optional gx_rb * rb)
#define gx_next_rbhandle(HANDLER, EXPECTED) gx_next_handle(HANDLER, GX_DEST_BUF, EXPECTED)
#ifdef DEBUG_EVENTS
    #define gx_next_handle(HANDLER, DESTINATION, EXPECTED) do {\
        sess->fn_handler = &HANDLER;                           \
        sess->rcv_dest = DESTINATION;                          \
        sess->rcv_expected = EXPECTED;                         \
        sess->fn_handler_name = #HANDLER ;                     \
    } while (0)
#else
    #define gx_next_handle(HANDLER, DESTINATION, EXPECTED) do {\
        sess->fn_handler = &HANDLER;                           \
        sess->rcv_dest = DESTINATION;                          \
        sess->rcv_expected = EXPECTED;                         \
    } while (0)
#endif

#ifdef DEBUG_EVENTS
#define _gx_call_handler(SESS,RB) ( {                          \
    log_debug("HANDLER: %s | exp: %lu (%lu in rb)"           \
        " | peek-avail: %lu",                                  \
        SESS->fn_handler_name,                                 \
        SESS->rcv_expected,                                    \
        (RB) == NULL ? 0 : rb_used(RB),                        \
        SESS->rcv_peek_avail);                                 \
    if(RB != NULL)                                             \
        gx_hexdump(rb_r(RB),min(232,rb_used(RB)),rb_used(RB)>232); \
    SESS->fn_handler(SESS, RB); } )
#else
#define _gx_call_handler(SESS,RB) SESS->fn_handler(SESS, RB)
#endif

//------- epoll ----------------------------------------------------------------
#if defined(__LINUX__) || defined(HAVE_EPOLL_WAIT)
  #include <sys/epoll.h>

  #define GX_EVENT_WRITABLE  EPOLLOUT
  #define GX_EVENT_READABLE  (EPOLLIN | EPOLLPRI)
  #define GX_EVENT_ERROR     EPOLLERR
  #define GX_EVENT_CLOSED    (EPOLLRDHUP | EPOLLHUP)

  #define GX_EVENT_STRUCT epoll_event

  #define GX_EVENT_IN     (EPOLLIN | EPOLLET)
  #define GX_EVENT_OUT    (EPOLLOUT | EPOLLET)
  #define GX_EVENT_SOCKET (EPOLLRDHUP | EPOLLPRI | EPOLLERR | EPOLLHUP)

  #define gx_event_is_readable(EVENT) ((EVENT).events & GX_EVENT_READABLE)
  #define gx_event_is_writable(EVENT) ((EVENT).events & GX_EVENT_WRITABLE)

  #define gx_event_newset(...) epoll_create1(EPOLL_CLOEXEC)

  static inline int gx_event_add_full(int evfd, int fd, int flags, void *data_ptr) {
      struct GX_EVENT_STRUCT new_event = {
          .events = flags,
          .data   = { .ptr = data_ptr } 
      };
      return epoll_ctl(evfd, EPOLL_CTL_ADD, fd, &new_event);
  }

  static inline int gx_event_add(int evfd, int fd, void *data_ptr) {
      return gx_event_add_full(evfd, fd, GX_EVENT_IN | GX_EVENT_OUT | GX_EVENT_SOCKET, data_ptr);
  }

  static inline int gx_event_del(int evfd, int fd) {
      //pointer to empty event for linux < 2.6.9 bug
      static struct GX_EVENT_STRUCT non_event;
      return epoll_ctl(evfd, EPOLL_CTL_DEL, fd, &non_event);
  }

  // TODO: one day, when I need it, a timer.
  static inline int gx_event_wait(int evfd, struct GX_EVENT_STRUCT *events, int max_returned, int milli_timeout) {
      return epoll_wait(evfd, events, max_returned, milli_timeout);
  }

  #define gx_event_data(EVENT) (EVENT).data.ptr
  #define gx_event_states(EVENT) (EVENT).events


//------- kqueue ---------------------------------------------------------------
#elif defined(__OSX__) || defined(HAVE_KQUEUE)
  #include <sys/types.h>
  #include <sys/event.h>
  #include <sys/time.h>

  #define GX_EVENT_WRITABLE  0x004
  #define GX_EVENT_READABLE  0x001
  #define GX_EVENT_ERROR     EV_ERROR
  //#define GX_EVENT_CLOSED    EV_EOF
  #define GX_EVENT_CLOSED    0x000 // Until I figure out a way to do it for real

  #define GX_EVENT_STRUCT kevent64_s

  #define GX_EVENT_IN     EVFILT_READ
  #define GX_EVENT_OUT    EVFILT_WRITE
  #define GX_EVENT_SOCKET 0

  #define gx_event_is_readable(EVENT) (((EVENT).filter & GX_EVENT_READABLE) == GX_EVENT_READABLE)
  #define gx_event_is_writable(EVENT) (((EVENT).filter & GX_EVENT_WRITABLE) == GX_EVENT_WRITABLE)

  #define gx_event_newset(...) kqueue()

  static inline int gx_event_add_full(int evfd, int fd, int flags, void *data_ptr) {
      struct GX_EVENT_STRUCT new_event = {
          .ident  = (uint64_t)fd,
          .filter = flags,
          .flags  = EV_ADD | EV_RECEIPT | EV_CLEAR | EV_EOF | EV_ERROR,
          .fflags = 0,
          .data   = 0,
          .udata  = (uint64_t)data_ptr,
          .ext    = {0,0}};
      return kevent64(evfd, &new_event, 1, NULL, 0, 0, NULL);
  }

  static inline int gx_event_add(int evfd, int fd, void *data_ptr) {
      return gx_event_add_full(evfd, fd, GX_EVENT_IN | GX_EVENT_OUT | GX_EVENT_SOCKET, data_ptr);
  }

  static inline int gx_event_del(int evfd, int fd) {
      struct GX_EVENT_STRUCT new_event = {
          .ident  = (uint64_t)fd,
          .filter = EVFILT_READ | EVFILT_WRITE,
          .flags  = EV_DISABLE | EV_DELETE,
          .fflags = 0,
          .data   = 0,
          .udata  = (uint64_t)NULL,
          .ext    = {0,0}};
      return kevent64(evfd, &new_event, 1, NULL, 0, 0, NULL);
  }

  // TODO: one day, when I need it, a timer.
  static inline int gx_event_wait(int evfd, struct GX_EVENT_STRUCT *events, int max_returned, int milli_timeout) {
      if (milli_timeout == -1) return kevent64(evfd, NULL, 0, events, max_returned, 0, NULL);
      else {
          struct timespec timeout;
          timeout.tv_sec = milli_timeout/1000;
          timeout.tv_nsec = (milli_timeout % 1000) * 1000 * 1000;
          return kevent64(evfd, NULL, 0, events, max_returned, 0, &timeout);
      }

  }

  #define gx_event_data(EVENT) ((void *)((EVENT).udata))
  #define gx_event_states(EVENT) ((EVENT).filter | GX_EVENT_WRITABLE | GX_EVENT_READABLE)



#else
  // TODO: if ever needed, do a select or poll fallback here
  #error "Don't know how to do the event looping for your OS"
#endif


//-----------------------------------------------------------------------------
/// Receive as much as we can and dispatch to the current handler that's
/// waitinf for data. Send anything waiting to be sent still, etc.
///
/// This loop works differently depending on the state of the system. If the
/// current data destination is already /dev/null or a file, it will try to
/// zero-copy the data straight from the socket into the appropriate place. Any
/// time the full expected amount is received it will, of course, call the
/// handler- which may or may not change the destination for the next piece of
/// data (along with the next handler etc. etc.).
///
/// If the data is meant to be passed directly to the handler for processing,
/// we first pull in as much data as possible to minimize system calls. Of
/// course, then we've possibly read in more data than is needed for the
/// current handler in which case we keep looping until the buffer is drained.
///

static void _gx_event_drainbuf(gx_tcp_sess *sess, gx_rb_pool *rb_pool, gx_rb **rcvrbp); // Forward declaration
static inline void _gx_event_incoming(gx_tcp_sess *sess, uint32_t events, gx_rb_pool *rb_pool, gx_rb **rcvrbp) {
    if(freq(events & GX_EVENT_READABLE)) {
        gx_rb   *rcvrb = *rcvrbp;
        ssize_t  rcvd, curr_remaining;
        int      can_rcv_more;
        do {
            if(rare(sess->peer_fd < 2)) {
                log_warning("Somehow a closed peer got in the inner eventloop.");
                return;
            }
            can_rcv_more = 0;
            curr_remaining = sess->rcv_expected - sess->rcvd_so_far;

            if(sess->rcv_dest == GX_DEST_BUF) {
                ssize_t  bytes_attempted;
                if(sess->rcv_buf) { // If present, use as the receive buffer
                    gx_rb_release(rb_pool, rcvrb);
                    rcvrb = sess->rcv_buf;
                    *rcvrbp = rcvrb;
                    sess->rcv_buf = NULL;
                } else rb_clear(rcvrb);

                // TODO: !!! if curr_remaining > rb_available, going to have to do
                // the lesser and set can_rcv_more to true.
                ssize_t avail = rb_available(rcvrb);
                if(rare(curr_remaining > avail)) {
                    log_error("Handler wants tcp data bigger than what can fit in the allocated ringbuffer.");
                    bytes_attempted = avail;
                } else if(sess->rcv_do_readahead) {
                    if(sess->rcv_max_readahead) {
                        bytes_attempted = min(curr_remaining+(ssize_t)sess->rcv_max_readahead, avail);
                    } else {
                        bytes_attempted = avail;
                    }
                } else {
                    bytes_attempted = curr_remaining;
                }

                if(freq(bytes_attempted > 0)) {
                    _ (rcvd = zc_sock_rbuf(sess->peer_fd, bytes_attempted, rcvrb, 1)){_alert();rcvd=0;}
                    if(freq(rcvd==bytes_attempted)) can_rcv_more = 1; // might still be something on the wire
                }
                _gx_event_drainbuf(sess, rb_pool, rcvrbp);
            } else if(sess->rcv_dest == GX_DEST_DEVNULL) {
                _ (rcvd = zc_sock_null(sess->peer_fd, curr_remaining)){_alert();rcvd=curr_remaining;}
                if(rcvd < curr_remaining) {
                    sess->rcvd_so_far += rcvd;
                    goto done_with_reading; // Not enough thrown away yet.
                } else {
                    sess->rcvd_so_far = 0;
                    sess->rcv_peek_avail  = 0;
                    if(rare(_gx_call_handler(sess, NULL) != GX_CONTINUE)) goto done_with_reading;
                    can_rcv_more = 1;
                }
            } else { // TODO: Check for GX_DEST_UNDEF
                log_error("Not yet implemented");
                /* TODO: the below is from imbibe- needs to be better and also
                 * detect and handle multiple destinations when appropriate.
                 */
                /*rcvd = rtmp_copy_sock(rtmp, curr_remaining, 1);

                if(rcvd < curr_remaining) {
                    rtmp->rcvd_so_far += rcvd;
                    goto done_with_reading;
                }
                rtmp->rcvd_so_far = 0;
                if(!call_handler((struct rtmp_sess *)rtmp, NULL, rtmp->rcv_expected)) goto done_with_reading;
                can_rcv_more = 1;
                */
                goto done_with_reading;
            }
        } while(can_rcv_more);
    }

done_with_reading:
    if(events & GX_EVENT_WRITABLE) {
        if(sess->snd_buf) {
            log_warning("Not yet implemented");
            goto done_with_writing;
        }
    }

done_with_writing:
    return;
}

static void _gx_event_drainbuf(gx_tcp_sess *sess, gx_rb_pool *rb_pool, gx_rb **rcvrbp) {
    // policy is to advance for anything but GX_DEST_BUF immediately
    // but wait till handler call to advance GX_DEST_BUF
    gx_rb   *rcvrb = *rcvrbp;
    while(rb_used(rcvrb)) {
        ssize_t curr_remaining = sess->rcv_expected - sess->rcvd_so_far;
        if(sess->rcv_dest == GX_DEST_BUF) {
            if(rb_used(rcvrb) < curr_remaining) { // Done draining- partial buffered chunk
                sess->rcvd_so_far += rb_used(rcvrb);
                sess->rcv_buf      = rcvrb;
                _N(rcvrb           = gx_rb_acquire(rb_pool)) _alert();
                return;
            }
        } else if(sess->rcv_dest == GX_DEST_DEVNULL) {
            if(rb_used(rcvrb) < curr_remaining) { // Done draining- partial throwaway
                sess->rcvd_so_far += rb_used(rcvrb);
                rb_clear(rcvrb); // Discard
                return;
            }
            rb_advr(rcvrb, curr_remaining); // curr_remaining because we've rb_clear'ed earlier ones
        } else { // File descriptor TODO: check for GX_DEST_UNDEF
            log_error("Not yet implemented");
        }
        // Looks like we have a full chunk / full expected length available.
        sess->rcvd_so_far = 0; // Clear it out so the next part is processed correctly

        if(sess->rcv_dest == GX_DEST_BUF) {
            int handle_res;  // Temporarily hold result of the handler
            if((size_t)rb_used(rcvrb) > sess->rcv_expected) {
                //-------- Alter ringbuffer so it only looks at the part the handler cares about currently.
                sess->rcv_peek_avail  = rb_used(rcvrb) - sess->rcv_expected;
                size_t old_write_head = rcvrb->w;           // True write position
                size_t old_expected   = sess->rcv_expected; // To advance read pointer after handler is done
                rcvrb->w              = rcvrb->r + sess->rcv_expected;
                //uint8_t old_nextbyte  = rb_uintw(rcvrb)[0];
                //rb_uintw(rcvrb)[0]    = '\0';               // So rb_uintr(rb) can be treated as a string if desired

                //-------- Callback on fn_handler
                handle_res            = _gx_call_handler(sess, rcvrb);

                //-------- Restore ringbuffer & advance read-pointer
                //rb_uintw(rcvrb)[0]    = old_nextbyte;
                rcvrb->w              = old_write_head;
                rb_advr(rcvrb, old_expected);
            } else {
                sess->rcv_peek_avail  = 0;
                handle_res            = _gx_call_handler(sess, rcvrb);
                rb_clear(rcvrb);
            }
            if(rare(handle_res != GX_CONTINUE)) return;
        } else {
            sess->rcv_peek_avail  = 0;
            if(rare(_gx_call_handler(sess, NULL) != GX_CONTINUE)) return;
        }
    }
}

static int _gx_close_sess(gx_tcp_sess *sess, gx_tcp_sess_pool *cespool, int reason, gx_rb_pool *rbp) {
    int res=0;
    if(sess->fn_disconnect) res = sess->fn_disconnect(sess, reason);

    if(sess->peer_fd > 1) {
try_close:
        // Explicitly close down both parts of the socket, working directly on
        // the _open file description_ instead of the file descriptor.
        switch_esys(shutdown(sess->peer_fd, SHUT_RDWR)) {
            case EINTR:    goto try_close;
            case ENOTCONN:
            case EBADF:    break;
            default:       res = -1;
        }
        close(sess->peer_fd); // Do this as well to auto-remove from event struct etc.
        sess->peer_fd = -1;
    } else return 0; // Was already aborted earlier
    if(sess->rcv_buf) {
        gx_rb_release(rbp, sess->rcv_buf);
        sess->rcv_buf = NULL;
    }
    if(sess->snd_buf) {
        gx_rb_release(rbp, sess->snd_buf);
        sess->snd_buf = NULL;
    }
    sess->udata   = NULL;  // Sure hope you freed it etc. in the disconnect handler...
    release_gx_tcp_sess(cespool, sess);
    return res;
}

static inline char *gx_closed_reason_txt(int reason) {
    return _gx_closed_reason[- reason];
}

static inline int gx_set_non_blocking(int fd) {
    int flags = 0;
    _ (flags = fcntl(fd, F_GETFL))             _raise(-1);
    _ (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) _raise(-1);
    return 0;
}

static inline void _gx_event_accept_connections(int lim, int afd, int (*ahandler)(gx_tcp_sess *),
        gx_tcp_sess_pool *cespool, int events_fd, gx_rb_pool *rb_pool, gx_rb **rcvrbp) {
    int                 i;
    struct sockaddr     peer_addr;
    socklen_t           slen = sizeof(struct sockaddr);
    int                 peer_fd;
    int                 warn_count = 0;
    gx_tcp_sess        *new_sess = NULL;

    for(i=0; i<lim; i++) {
        switch_esys(peer_fd = accept(afd, &peer_addr, &slen)) {
            case EWOULDBLOCK:
            case EINTR:
                return;
            case ENETDOWN:
            case EPROTO:
            case ENOPROTOOPT:
            case EHOSTDOWN:
#if defined(__linux__) || defined(__LINUX__)
            case ENONET:
#endif
            case EHOSTUNREACH:
            case EOPNOTSUPP:
            case ENETUNREACH:
                warn_count ++;
                if(warn_count < 5) continue; // Transitory as per manpages. Try again
                else return;
            default:
                _error();
                _raise();
        } else {
            _ (gx_set_non_blocking(peer_fd)) { _error(); close(peer_fd); _raise(); }

            if(freq(ahandler != NULL)) {
                _N(new_sess = acquire_gx_tcp_sess(cespool)) {_error(); close(peer_fd); _raise();}
                new_sess->peer_fd     = peer_fd;
                new_sess->rcv_buf     = NULL;
                new_sess->snd_buf     = NULL;
                new_sess->rcvd_so_far = 0;
                if(freq(ahandler(new_sess) == GX_CONTINUE)) {
                    _ (gx_event_add(events_fd, peer_fd, (void *)new_sess)) {
                        _error();
                        _(_gx_close_sess(new_sess, cespool, GX_INTERNAL_ERR, rb_pool)) _error();
                        continue;
                    }
                    // Trigger here because it's very likely to be available
                    _gx_event_incoming(new_sess, GX_EVENT_READABLE | GX_EVENT_WRITABLE, rb_pool, rcvrbp);
                } else {
                    // Will not call the disconnect handler if accept-handler rejected.
                    close(peer_fd);
                    release_gx_tcp_sess(cespool, new_sess);
                }
            }
        }
    }
}

#endif

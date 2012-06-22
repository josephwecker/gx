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
 */

#include <gx/gx.h>
#include <gx/gx_error.h>
#include <gx/gx_ringbuf.h>
#include <gx/gx_zerocopy.h>

#define GX_DEST_DEVNULL      -3  ///< Discard incoming data
#define GX_DEST_BUF          -2  ///< Save incoming data in a ring buffer

typedef struct gx_tcp_sess {
    struct gx_tcp_sess   *_next, *_prev;
    int                   rcv_dest;
    gx_rb                *rcv_buf;
    int                   rcv_do_readahead;
    size_t                rcv_expected;
    size_t                rcvd_so_far;
    int                   peer_fd;
    gx_rb                *snd_buf;
    int (*fn_handler)    (struct gx_tcp_sess *, uint8_t *, size_t);
    void (*fn_disconnect)(struct gx_tcp_sess *);
    //char                 *fn_handler_name;
    void                 *udata;
} gx_tcp_sess;
gx_pool_init(gx_tcp_sess);


#define gx_eventloop_prepare(NAME, EXPECTED_SESSIONS, EVENTS_AT_A_TIME)          \
    struct GX_EVENT_STRUCT   NAME ## _events[EVENTS_AT_A_TIME];                  \
    int                      NAME ## _events_at_a_time = EVENTS_AT_A_TIME;       \
    int                      NAME ## _events_fd;                                 \
    int                      NAME ## _expected_sessions = EXPECTED_SESSIONS;     \
    gx_tcp_sess_pool       * NAME ## _sess_pool_inst;                            \
    gx_rb_pool             * NAME ## _rb_pool;                                   \
    gx_rb                  * NAME ## _rcvrb;                                     \
                                                                                 \
    int GX_INLINE NAME ## _add_sess(int peer_fd,                                 \
            void *misc,                                                          \
            void (*disc_handler)(gx_tcp_sess *),                                 \
            int dest,                                                            \
            int (*handler)(gx_tcp_sess *, uint8_t *, size_t),                    \
            size_t bytes_expected, int do_readahead) {                           \
        gx_tcp_sess *sess;                                                       \
        Xn(sess = acquire_gx_tcp_sess(NAME ## _sess_pool_inst)) X_RAISE(-1);     \
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
                                                                                 \
    int GX_INLINE NAME ## _add_misc(int peer_fd, void *misc) {                   \
        return NAME ## _add_sess(peer_fd, misc, NULL, -1, NULL, 0, 0);           \
    }                                                                            \
                                                                                 \
    int GX_INLINE NAME ## _wait(int timeout,                                     \
            int (*misc_handler)(gx_tcp_sess *, uint32_t)) {                      \
        int nfds, i;                                                             \
        uint32_t evstates;                                                       \
        gx_tcp_sess *sess;                                                       \
        while(1) {                                                               \
            X (nfds = gx_event_wait(NAME ## _events_fd,                          \
                        NAME ## _events,                                         \
                        EVENTS_AT_A_TIME, timeout)) X_RAISE(-1);                 \
            if(!nfds) return 0;                                                  \
            for(i=0; i < nfds; ++i) {                                            \
                sess     = (gx_tcp_sess *)gx_event_data(NAME ## _events[i]);     \
                evstates = gx_event_states(NAME ## _events[i]);                  \
                if(gx_likely(sess->fn_handler)) {                                \
                    _gx_event_incoming(sess, evstates, NAME ## _rb_pool,         \
                            & NAME ## _rcvrb);                                   \
                    if(gx_unlikely(evstates & GX_EVENT_CLOSED)) {                \
                        if(sess->fn_disconnect) sess->fn_disconnect(sess);       \
                        close(sess->peer_fd);                                    \
                        if(sess->rcv_buf)                                        \
                            gx_rb_release(NAME ## _rb_pool, sess->rcv_buf);      \
                        if(sess->snd_buf)                                        \
                            gx_rb_release(NAME ## _rb_pool, sess->snd_buf);      \
                        release_gx_tcp_sess(NAME ## _sess_pool_inst, sess);      \
                    }                                                            \
                } else if(misc_handler) {                                        \
                    X(misc_handler(sess, evstates)) X_RAISE(-1);                 \
                } else return -1;                                                \
            }                                                                    \
        }                                                                        \
        return 0;                                                                \
    }


#define gx_eventloop_init(NAME) { \
    Xn(NAME ## _rb_pool        = gx_rb_pool_new(NAME ## _expected_sessions/4+2,0x1000,0)) {X_ERROR; X_EXIT;}\
    Xn(NAME ## _rcvrb          = gx_rb_acquire(NAME ## _rb_pool))                         {X_ERROR; X_EXIT;}\
    Xn(NAME ## _sess_pool_inst = new_gx_tcp_sess_pool(NAME ## _expected_sessions))        {X_ERROR; X_EXIT;}\
    X (NAME ## _events_fd      = gx_event_newset(NAME ## _events_at_a_time))              {X_ERROR; X_EXIT;}\
}

#define GX_EVENT_HANDLER(NAME) int NAME(gx_tcp_sess *sess, uint8_t *dat, size_t len)
#define gx_event_set_handler(SESS, HANDLER) {SESS->fn_handler = &HANDLER; SESS->fn_handler_name = #HANDLER;}

//#define rtmp_handler(NAME) extern int NAME(struct rtmp_sess *rtmp, uint8_t *dat, ssize_t len)
//rtmp_handler( dumper                       );
//rtmp_handler( hs_check_version             );

//#define set_handler(RTMP, HANDLER) {RTMP->handler = &HANDLER; RTMP->handler_name = #HANDLER; }


//------- epoll ----------------------------------------------------------------
#if defined(__LINUX__) || defined(HAVE_EPOLL_WAIT)
  #include <sys/epoll.h>

  #define GX_EVENT_WRITABLE  EPOLLOUT
  #define GX_EVENT_READABLE  (EPOLLIN | EPOLLPRI)
  #define GX_EVENT_ERROR     EPOLLERR
  #define GX_EVENT_CLOSED    (EPOLLRDHUP | EPOLLHUP)

  #define GX_EVENT_STRUCT epoll_event

  #define gx_event_newset(max_returned) epoll_create(max_returned)

  static GX_INLINE int gx_event_add(int evfd, int fd, void *data_ptr) {
      struct GX_EVENT_STRUCT new_event = {
          .events = EPOLLIN  | EPOLLOUT | EPOLLET  | EPOLLRDHUP |
                    EPOLLPRI | EPOLLERR | EPOLLHUP,
          .data   = { .ptr = data_ptr }};
      return epoll_ctl(evfd, EPOLL_CTL_ADD, fd, &new_event);
  }

  static GX_INLINE int gx_event_del(int evfd, int fd) {
      //pointer to empty event for linux < 2.6.9 bug
      static struct GX_EVENT_STRUCT non_event;
      return epoll_ctl(evfd, EPOLL_CTL_DEL, fd, &non_event);
  }

  // TODO: one day, when I need it, a timer.
  static GX_INLINE int gx_event_wait(int evfd, struct GX_EVENT_STRUCT *events, int max_returned, int milli_timeout) {
      return epoll_wait(evfd, events, max_returned, milli_timeout);
  }

  #define gx_event_data(EVENT) (EVENT).data.ptr
  #define gx_event_states(EVENT) (EVENT).events


//------- kqueue ---------------------------------------------------------------
#elif defined(__DARWIN__) || defined(HAVE_KQUEUE)
  #include <sys/types.h>
  #include <sys/event.h>
  #include <sys/time.h>

  #define GX_EVENT_WRITABLE  0x004
  #define GX_EVENT_READABLE  0x001
  #define GX_EVENT_ERROR     EV_ERROR
  //#define GX_EVENT_CLOSED    EV_EOF
  #define GX_EVENT_CLOSED    0x000 // Until I figure out a way to do it for real

  #define GX_EVENT_STRUCT kevent64_s

  #define gx_event_newset(max_returned) kqueue()

  static GX_INLINE int gx_event_add(int evfd, int fd, void *data_ptr) {
      struct GX_EVENT_STRUCT new_event = {
          .ident  = (uint64_t)fd,
          .filter = EVFILT_READ | EVFILT_WRITE,
          .flags  = EV_ADD | EV_RECEIPT | EV_CLEAR | EV_EOF | EV_ERROR,
          .fflags = 0,
          .data   = 0,
          .udata  = (uint64_t)data_ptr,
          .ext    = {0,0}};
      return kevent64(evfd, &new_event, 1, NULL, 0, 0, NULL);
  }

  static GX_INLINE int gx_event_del(int evfd, int fd) {
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
  static GX_INLINE int gx_event_wait(int evfd, struct GX_EVENT_STRUCT *events, int max_returned, int milli_timeout) {
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

GX_INLINE void _gx_event_drainbuf(gx_tcp_sess *sess, gx_rb_pool *rb_pool, gx_rb **rcvrbp); // Forward declaration
GX_INLINE void _gx_event_incoming(gx_tcp_sess *sess, uint32_t events, gx_rb_pool *rb_pool, gx_rb **rcvrbp) {
    if(gx_likely(events & GX_EVENT_READABLE)) {
        gx_rb   *rcvrb = *rcvrbp;
        ssize_t  rcvd, curr_remaining;
        int      can_rcv_more;
        do {
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

                if(sess->rcv_do_readahead) bytes_attempted = rb_available(rcvrb);
                else bytes_attempted = curr_remaining;

                if(gx_likely(bytes_attempted > 0)) {
                    X (rcvd = zc_sock_rbuf(sess->peer_fd, bytes_attempted, rcvrb, 1)){X_FATAL;rcvd=0;}
                    if(gx_likely(rcvd==bytes_attempted)) can_rcv_more = 1; // might still be something on the wire
                }
                _gx_event_drainbuf(sess, rb_pool, rcvrbp);
            } else if(sess->rcv_dest == GX_DEST_DEVNULL) {
                X (rcvd = zc_sock_null(sess->peer_fd, curr_remaining)){X_FATAL;rcvd=curr_remaining;}
                if(rcvd < curr_remaining) {
                    sess->rcvd_so_far += rcvd;
                    goto done_with_reading; // Not enough thrown away yet.
                } else {
                    sess->rcvd_so_far = 0;
                    if(!sess->fn_handler(sess, NULL, sess->rcv_expected)) goto done_with_reading;
                    can_rcv_more = 1;
                }
            } else {
                X_LOG_ERROR("Not yet implemented");
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
            X_LOG_WARN("Not yet implemented");
            goto done_with_writing;
        }
    }

done_with_writing:
    return;
}

GX_INLINE void _gx_event_drainbuf(gx_tcp_sess *sess, gx_rb_pool *rb_pool, gx_rb **rcvrbp) {
    uint8_t *hbuf  = NULL;
    gx_rb   *rcvrb = *rcvrbp;
    while(rb_used(rcvrb)) {
        ssize_t curr_remaining = sess->rcv_expected - sess->rcvd_so_far;
        if(sess->rcv_dest == GX_DEST_BUF) {
            if(rb_used(rcvrb) < curr_remaining) { // Done draining- partial buffered chunk
                sess->rcvd_so_far += rb_used(rcvrb);
                sess->rcv_buf      = rcvrb;
                Xn(rcvrb           = gx_rb_acquire(rb_pool)) X_FATAL;
                return;
            }
            hbuf = rb_read_adv(rcvrb, sess->rcv_expected); // full expected
        } else if(sess->rcv_dest == GX_DEST_DEVNULL) {
            if(rb_used(rcvrb) < curr_remaining) { // Done draining- partial throwaway
                sess->rcvd_so_far += rb_used(rcvrb);
                rb_clear(rcvrb); // Discard
                return;
            }
            rb_advr(rcvrb, curr_remaining); // curr_remaining because we've rb_clear'ed earlier ones
        } else { // File descriptor
            X_LOG_ERROR("Not yet implemented");
            hbuf = NULL;
        }
        // Looks like we have a full chunk / full expected length available.
        sess->rcvd_so_far = 0; // Clear it out
        if(!sess->fn_handler(sess, hbuf, sess->rcv_expected)) return;
        //if(sess->rcv_buf)
    }
}

#endif

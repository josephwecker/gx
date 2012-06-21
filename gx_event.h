/*
 *
 * gx_eventloop_prepare(<name>, expected-sessions, events-at-a-time)  // global
 * gx_eventloop_init(<name>) // before adding etc.- uses global state
 * <name>_add_misc(peer_fd, *misc)
 * <name>_add_sess(peer_fd, disc_handler, dest1, handler1, expected1, readahead1, *misc)
 * <name>_wait(timeout, misc_handler) // returns -1 on error, 0 on timeout
 *
 * gx_eventloop_prepare(name, expected_num_sessions, events_at_a_time)
 *   To be called (for now) somewhere in global statespace- defines structs
 *   etc. Defines the following pieces- parenthases indicate that it's used
 *   internally:
 *   -  <name>_sess             - session struct (and pool-related stuff)
 *   - ( main_<name>_sess_pool   - preallocated session structs         )
 *   - ( <name>_events           - GX_EVENT_STRUCT for the eventloop    )
 *   - ( <name>_events_at_a_time - int global param                     )
 *   - ( <name>_events_fd        - int primary eventloop filedescriptor )
 *   - ( TODO: expected_sessions )
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



#define gx_eventloop_prepare(NAME, EXPECTED_SESSIONS, EVENTS_AT_A_TIME) \
    typedef struct NAME ## _sess {
        struct NAME ## _sess *_next, *_prev;
        int                   rcv_dest;
        gx_rb                *rcv_buf;
        size_t                rcv_readahead;
        ssize_t               rcv_expected;
        size_t                rcvd_so_far;
        int                   peer_fd;
        gx_rb                *snd_buf;
        int (*fn_handler)    (struct gx_tcp_sess *, uint8_t *, ssize_t);
        char                 *fn_handler_name;
        void                 *udata;
    } NAME ## _sess;
    gx_pool_init(NAME ## _sess);
    main_ ## NAME ## _sess_pool = new_ ## NAME ## _sess_pool(EXPECTED_SESSIONS);
    struct GX_EVENT_STRUCT NAME ## _events;
    int NAME ## _events_at_a_time = EVENTS_AT_A_TIME;
    int NAME ## _events_fd;
    int NAME ## _expected_sessions = EXPECTED_SESSIONS;
    
    // TODO:
    //   - define rb-pool
    //   - define primary receive buffer

#define gx_eventloop_init(NAME)
    // TODO:
    // Xn(NAME ## rb_pool = gx_rb_pool_new(NAME ## _expected_sessions / 4 + 2, 0x1000, 2) ){X_ERROR; X_EXIT;}
    // (acquire a pool as primary receive buffer)
    // rtmp_pool    = new_rtmp_sess_pool(0x10);
    // initialize actual event-structure into _events_fd


#define tcp_sess_handler(NAME) extern int NAME(struct gx_tcp_sess *sess, uint8_t *dat, ssize_t len)
#define set_handler(SESS, HANDLER) {SESS->fn_handler = &HANDLER; SESS->fn_handler_name = #HANDLER; }


/* TODO: YOU ARE HERE- make these functions generic */
GX_INLINE void rtmp_event_drain_buf(rtmp_sess *rtmp);
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
GX_INLINE void rtmp_event(rtmp_sess *rtmp, uint32_t events) {
    if(gx_likely(events & GX_EVENT_READABLE)) {
        ssize_t rcvd, curr_remaining;
        ssize_t bytes_attempted;
        int     can_rcv_more;
        do {
            can_rcv_more = 0;
            
            if(rtmp->rcv_dest == RD_BUF) {
                if(rtmp->rcv_buf) { // Swap any sess-storage in as the current receive buffer
                    gx_rb_release(rb_pool, rcvrb);
                    rcvrb = rtmp->rcv_buf;
                    rtmp->rcv_buf = NULL;
                } else rb_clear(rcvrb);

                if(gx_unlikely(rtmp->rcv_readahead)) bytes_attempted = rb_available(rcvrb);
                else bytes_attempted = rtmp->rcv_expected - rtmp->rcvd_so_far;

                /// @todo  use zerocopy library here, for the sake of the children
                if (gx_likely(bytes_attempted > 0)) {
                    rcvd = zc_sock_rbuf(rtmp->peer_fd, bytes_attempted, rcvrb, 1);
                    if(gx_likely(rcvd == bytes_attempted)) can_rcv_more = 1;
                }
                rtmp_event_drain_buf((struct rtmp_sess *)rtmp);
            } else if(rtmp->rcv_dest == RD_DEVNULL) {
                curr_remaining = rtmp->rcv_expected - rtmp->rcvd_so_far;
                /// @todo  possibly recover here on error
                X (rcvd = zc_sock_null(rtmp->peer_fd, curr_remaining)) {X_FATAL; X_RAISE();}
                if(rcvd < curr_remaining) {
                    rtmp->rcvd_so_far += rcvd;
                    return;
                }
                rtmp->rcvd_so_far = 0;

                if(!call_handler(rtmp, NULL, rtmp->rcv_expected)) return;
                can_rcv_more = 1;
            } else {
                curr_remaining = rtmp->rcv_expected - rtmp->rcvd_so_far;
                rcvd = rtmp_copy_sock(rtmp, curr_remaining, 1);

                if(rcvd < curr_remaining) {
                    rtmp->rcvd_so_far += rcvd;
                    return;
                }
                rtmp->rcvd_so_far = 0;
                if(!call_handler((struct rtmp_sess *)rtmp, NULL, rtmp->rcv_expected)) return;
                can_rcv_more = 1;
            }
        } while(can_rcv_more);
    }
    if(events & GX_EVENT_WRITABLE) {
        if (rtmp->send_buf) {
            zc_rbuf_sock(rtmp->send_buf, rtmp->peer_fd, 1);
            if (rb_used(rtmp->send_buf) == 0) {
                gx_rb_release(rb_pool, rtmp->send_buf); 
                rtmp->send_buf = NULL;
            }
        }
        
        // @todo Sending
        // * If not all sent in first call, copy to ringbuffer, attach to session,
        //   and done - or add to existing ringbuffer if present. (if at the
        //   maximum size, something wrong has happened- esp. for rtmp...)
        // * In this function check for active ringbuffer. If one exists and
        //   writeable, try to send more data.
        // * As soon as there is nothing left in the ring-buffer, release it to the
        //   pool.
    }
    // @todo  unlikely socket errors etc., or handle them in imbibe_server
}

GX_INLINE void rtmp_event_drain_buf(struct rtmp_sess *rtmp) {
    uint8_t *hbuf=NULL;
    #ifdef DEBUG_VERBOSE
        X_LOG_DEBUG("draining all this :");
        gx_hexdump(rb_r(rcvrb), rb_used(rcvrb));
    #endif
    while(rb_used(rcvrb)) {
        ssize_t curr_remaining = rtmp->rcv_expected - rtmp->rcvd_so_far;
        if(rtmp->rcv_dest == RD_BUF) {
            if(rb_used(rcvrb) < curr_remaining) {
                rtmp->rcvd_so_far += rb_used(rcvrb);
                rtmp->rcv_buf      = rcvrb;
                Xn(rcvrb           = gx_rb_acquire(rb_pool)) X_ERROR; // @todo abort?
                return;
            }
            hbuf = rb_read_adv(rcvrb, rtmp->rcv_expected);
        
        } else if(rtmp->rcv_dest == RD_DEVNULL) {
            if(rb_used(rcvrb) < curr_remaining) {
                rtmp->rcvd_so_far += rb_used(rcvrb);
                rb_clear(rcvrb); // discard
                return;
            }
            hbuf = rb_read_adv(rcvrb, curr_remaining); // discard
        } else {  // File descriptor
            if(rb_used(rcvrb) < curr_remaining) {
                rtmp->rcvd_so_far += rb_used(rcvrb);
                rtmp_copy_rbuf(rtmp, rcvrb, rb_used(rcvrb), TO_FLV_FD | TO_LISTENERS, 1);
                return;
            }
            hbuf = rb_r(rcvrb);
            rtmp_copy_rbuf(rtmp, rcvrb, curr_remaining, TO_FLV_FD | TO_LISTENERS, 1);
        }
        // Call handler, leaving excess bytes still on buffer
        rtmp->rcvd_so_far = 0;
        if(!call_handler(rtmp, hbuf, rtmp->rcv_expected)) return;
        if(rtmp->rcv_buf) X_LOG_ERROR("Should be no partially saved ringbuffer here (%p seen)", rtmp->rcv_buf);
    }
}


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

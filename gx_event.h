#include <gx/gx.h>

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

  // TODO: one day, when I need it, a timer.
  static GX_INLINE int gx_event_wait(int evfd, struct GX_EVENT_STRUCT *events, int max_returned, int milli_timeout) {
      return epoll_wait(evfd, events, max_returned, milli_timeout);
  }

  #define gx_event_data(EVENT) (EVENT).data.ptr
  #define gx_event_states(EVENT) (EVENT).events


//------- poll -----------------------------------------------------------------
// I want this because kevent64 isn't implemented in valgrind
#elif defined(DEBUG)
  #include <poll.h>
  #define GX_EVENT_STRUCT pollfd
  #define gx_event_newset(max_returned) -1
  static struct pollfd pollfds[1024];
  static int pollfd_count = 0;
  static GX_INLINE int gx_event_add(int evfd, int fd, void *data_ptr) {
    struct GX_EVENT_STRUCT new_event;
    new_event.fd = fd;
    new_event.events = POLLIN  | POLLOUT | POLLPRI | POLLERR | POLLHUP;
    pollfds[pollfd_count++] = new_event;
    return 0;
  }
  static GX_INLINE int gx_event_wait(int evfd, struct GX_EVENT_STRUCT *events, int max_returned, int milli_timeout) {
    X_LOG_DEBUG("polling...");
    return poll(pollfds, pollfd_count, -1);
  }

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

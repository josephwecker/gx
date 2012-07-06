/**
 * @file      gx.h
 * @brief     Generic eXtra stuff (GX) primary header file- misc. macros.
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
 * @todo  Normalize all the naming conventions
 * @todo  Update documentation / fix all for doxygen
 * @todo  Move resource pool to its own module
 * @todo  gx_prefetch_(rw_keep|rw_toss|ro_keep|ro_toss)
 *
 */
#ifndef   GX_H
  #define GX_H

  /// Normalize some OS names
  #ifdef __linux__
    #define __LINUX__ 1
  #elif defined(__APPLE__) || defined(__MACH__)
    #define __DARWIN__ 1
  #endif

  #if defined(__LINUX__)
    #ifndef _GNU_SOURCE
      #define _GNU_SOURCE 1
    #endif
  #endif

  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <string.h>
  #include <pthread.h> // For pool mutexes
  #include <sys/wait.h>
  #include <syscall.h>

  /// Types
  #include <stdint.h>
  #include <inttypes.h>
  #include <sys/types.h>
  typedef uint8_t  byte,     uint8,       uint8_bitmask;
  typedef uint16_t uint16,   uint16_be,   uint16_le;
  typedef int16_t  sint16,   sint16_be,   sint16_le;
  typedef uint32_t uint32,   uint32_be,   uint32_le;
  typedef int32_t  sint32,   sint32_be,   sint32_le;
  typedef uint64_t uint64,   uint64_le,   uint64_be;
  typedef int64_t  sint64,   sint64_le,   sint64_be;
  typedef struct __uint24   {uint8 b[3];} uint24, uint24_be, uint24_le;
  typedef uint64_t number64, number64_le, number64_be;

  /* Misc */
  #ifndef MIN
  #define MIN(m,n) ((m) < (n) ? (m) : (n))
  #endif

  #ifndef MAX
  #define MAX(m,n) ((m) > (n) ? (m) : (n))
  #endif


  /// Compiler Optimization Cues
  /// Borrowed ideas from haproxy and the Linux kernel
  #if          __GNUC__ > 3
    #ifndef    gx_likely
      #define  gx_likely(x)   __builtin_expect(!!(x), 1)
      #define  gx_unlikely(x) __builtin_expect(!!(x), 0)
    #endif
    #ifndef    gx_cold_f
      #define  gx_pure_f      __attribute__((pure))
      #define  gx_cold_f      __attribute__((cold))
      #define  gx_hot_f       __attribute__((hot))
    #endif
    #ifndef    GX_INLINE
      #ifdef   DEBUG
        #define GX_INLINE
      #else 
        #define  GX_INLINE   inline __attribute__((always_inline))
      #endif
    #endif
  #else
    #ifndef    gx_likely
      #define  gx_likely(x)   (x)
      #define  gx_unlikely(x) (x)
    #endif
    #ifndef    gx_cold_f
      #define  gx_cold_f
      #define  gx_hot_f
      #define  gx_pure_f
    #endif
    #ifndef    GX_INLINE
    #define    GX_INLINE   inline
    #endif
  #endif

  static GX_INLINE void gx_hexdump(void *buf, size_t len) {
      size_t i;
      flockfile(stderr);
      //fprintf(stderr, "-------------\n");
      for(i=0; i<len; i++) {
          fprintf(stderr, "%02X ", ((uint8_t *)buf)[i]);
          if(i%8==7) fprintf(stderr, "| ");
          if(i%24==23)fprintf(stderr, "\n");
      }
      //fprintf(stderr, "\n-------------\n\n");
      fprintf(stderr, "\n");
      funlockfile(stderr);
  }


  // TYPE must be a type that has a "_next" and "_prev" member that is a pointer to the same
  // type. Also, the struct should be aligned (?).

  #define gx_pool_init(TYPE)                                                             \
                                                                                         \
    typedef struct pool_memory_segment ## TYPE {                                         \
        struct pool_memory_segment ## TYPE  *next;                                       \
        TYPE                                *segment;                                    \
    } pool_memory_segment ## TYPE;                                                       \
                                                                                         \
    typedef struct TYPE ## _pool {                                                       \
        pthread_mutex_t                      mutex;                                      \
        size_t                               total_items;                                \
        TYPE                                *available_head;                             \
        TYPE                                *active_head, *active_tail;                  \
        TYPE                                *prereleased[0x10000];                         \
        pool_memory_segment ## TYPE         *memseg_head;                                \
    } TYPE ## _pool;                                                                     \
                                                                                         \
    static int TYPE ## _pool_extend(TYPE ## _pool *pool, size_t by_number);              \
    static GX_INLINE TYPE ## _pool *new_  ##  TYPE ## _pool (size_t initial_number) {    \
        TYPE ## _pool *res;                                                              \
        Xn(res=(TYPE ## _pool *)malloc(sizeof(TYPE ## _pool))) X_RAISE(NULL);            \
        memset(res, 0, sizeof(TYPE ## _pool));                                           \
        TYPE ## _pool_extend(res, initial_number);                                       \
        X (pthread_mutex_init(&(res->mutex), NULL)) X_RAISE(NULL);                          \
        return res;                                                                      \
    }                                                                                    \
                                                                                         \
    static int TYPE ## _pool_extend(TYPE ## _pool *pool, size_t by_number) {             \
        TYPE *new_seg;                                                                   \
        size_t curr;                                                                     \
        pool_memory_segment ## TYPE *memseg_entry;                                       \
        Xn(new_seg = (TYPE *)malloc(sizeof(TYPE) * by_number)) X_RAISE(-1);              \
                                                                                         \
        /* Link to memory-segments for freeing later */                                  \
        Xn(memseg_entry = (pool_memory_segment ## TYPE *)malloc(                         \
                    sizeof(pool_memory_segment ## TYPE))) X_RAISE(-1);                   \
        memseg_entry->segment = new_seg;                                                 \
        memseg_entry->next    = pool->memseg_head;                                       \
        pool->memseg_head     = memseg_entry;                                            \
                                                                                         \
        /* Link them up */                                                               \
        new_seg[by_number - 1]._next = (struct TYPE *)pool->available_head;              \
        pool->available_head = new_seg;                                                  \
        for(curr=0; curr < by_number-1; curr++) {                                        \
            new_seg[curr]._next = (struct TYPE *) &(new_seg[curr+1]);                    \
        }                                                                                \
        pool->total_items += by_number;                                                  \
        return 0;                                                                        \
    }                                                                                    \
                                                                                         \
    static GX_INLINE void _prepend_ ## TYPE (TYPE ## _pool *pool, TYPE *entry) {         \
        /* put an object at the front of the active list */                              \
        entry->_next = pool->active_head;                                                \
        if (gx_likely(pool->active_head != NULL)) { pool->active_head->_prev = entry; }  \
        pool->active_head = entry;                                                       \
        if (gx_unlikely(pool->active_tail == NULL)) { pool->active_tail = entry; }       \
    }                                                                                    \
                                                                                         \
    static GX_INLINE void _remove_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {           \
        /* remove an object from the active list */                                      \
        if (gx_likely(entry->_prev != NULL)) { entry->_prev->_next = entry->_next; }     \
        else { pool->active_head = entry->_next; }                                       \
        if (gx_likely(entry->_next != NULL)) { entry->_next->_prev = entry->_prev; }     \
        else { pool->active_tail = entry->_prev; }                                       \
    }                                                                                    \
                                                                                         \
    static GX_INLINE TYPE *acquire_ ## TYPE(TYPE ## _pool *pool) {                       \
        TYPE *res = NULL;                                                                \
        pthread_mutex_lock(&(pool->mutex));                                              \
        if(gx_unlikely(!pool->available_head))                                           \
            if(TYPE ## _pool_extend(pool, pool->total_items) == -1) goto fin;            \
        res = pool->available_head;                                                      \
        pool->available_head = res->_next;                                               \
        memset(res, 0, sizeof(TYPE));                                                    \
        _prepend_ ## TYPE(pool, res);                                                    \
      fin:                                                                               \
        pthread_mutex_unlock(&(pool->mutex));                                            \
        return res;                                                                      \
    }                                                                                    \
                                                                                         \
    static GX_INLINE void prerelease_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {        \
        pthread_mutex_lock(&(pool->mutex));                                              \
        pid_t cpid;                                                                      \
        cpid = syscall(SYS_getpid);                                                      \
        unsigned int idx = (unsigned int)cpid & 0xffff;                                  \
        if(gx_unlikely(pool->prereleased[idx]))                                          \
            X_LOG_ERROR("Snap, prerelease snafu: %d", idx);                              \
        pool->prereleased[idx] = entry;                                                  \
        pthread_mutex_unlock(&(pool->mutex));                                            \
    }                                                                                    \
                                                                                         \
    static GX_INLINE void finrelease_ ## TYPE(TYPE ## _pool *pool, pid_t cpid) {         \
        pthread_mutex_lock(&(pool->mutex));                                              \
        unsigned int idx = (unsigned int)cpid & 0xffff;                                  \
        TYPE *entry = pool->prereleased[idx];                                            \
        if(gx_unlikely(!entry)) X_LOG_ERROR("Snap, prerelease snafu: %d", idx);          \
        _remove_ ## TYPE(pool, entry);                                                   \
        entry->_next = pool->available_head;                                             \
        pool->available_head = entry;                                                    \
        pool->prereleased[idx] = NULL;                                                   \
        pthread_mutex_unlock(&(pool->mutex));                                            \
    }                                                                                    \
                                                                                         \
    static GX_INLINE void release_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {           \
        pthread_mutex_lock(&(pool->mutex));                                              \
        _remove_ ## TYPE(pool, entry);                                                   \
        entry->_next = pool->available_head;                                             \
        pool->available_head = entry;                                                    \
        pthread_mutex_unlock(&(pool->mutex));                                            \
    }                                                                                    \
                                                                                         \
    static GX_INLINE void move_to_front_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {     \
        /* move an object to the front of the active list */                             \
        _remove_ ## TYPE(pool, entry);                                                   \
        _prepend_ ## TYPE(pool, entry);                                                  \
    }


    static GX_INLINE int gx_to_vlq(uint64_t x, uint8_t *out) {
        int i, j, count=0;
        for (i = 9; i > 0; i--) { if (x & 127ULL << i * 7) break; }
        for (j = 0; j <= i; j++) {
            out[j] = ((x >> ((i - j) * 7)) & 127) | 128;
            ++count;
        }
        out[i] ^= 128;
        return ++count;
    }

    static GX_INLINE uint64_t gx_from_vlq(uint8_t *in) {
        uint64_t r = 0;
        do r = (r << 7) | (uint64_t)(*in & 127);
        while (*in++ & 128);
        return r;
    }


  /*=============================================================================
   * MISC MATH
   * gx_pos_ceil(x)                - Integer ceiling (for positive values)
   * gx_fits_in(container_size, x) - How many containers needed?
   *---------------------------------------------------------------------------*/
  #define gx_pos_ceil(x) (((x)-(int)(x)) > 0 ? (int)((x)+1) : (int)(x))
  #define gx_fits_in(container_size, x) gx_pos_ceil((float)(x) / (float)(container_size))
  #define gx_in_pages(SIZE) (((SIZE) & ~(gx_pagesize - 1)) + gx_pagesize)

  /*=============================================================================
   * OS PARAMETERS
   * gx_pagesize                   - Gets OS kernel page size
   * Yeah, needs to coordinate somewhat w/ a config file or something...
   *---------------------------------------------------------------------------*/
  static int _GXPS __attribute__ ((unused)) = 0;

  #if defined(HAS_SYSCONF) && defined(_SC_PAGE_SIZE)
    #define gx_pagesize ({ if(!_GXPS) _GXPS=sysconf(_SC_PAGE_SIZE); _GXPS; })
  #elif defined (HAS_SYSCONF) && defined(_SC_PAGESIZE)
    #define gx_pagesize ({ if(!_GXPS) _GXPS=sysconf(_SC_PAGESIZE);  _GXPS; })
  #else
    #define gx_pagesize ({ if(!_GXPS) _GXPS=getpagesize();          _GXPS; })
  #endif


  #include <gx/gx_error.h>

    /// Very "lightweight" fork, calls given function in child. Falls back to
    /// fork on systems that don't have anything lighter. Otherwise tries to
    /// create a new process with everything shared with the parent except a
    /// new, _small_ stack. The parent is not signaled when the child
    /// finishes [at least on linux... don't know yet for other...]
    #ifdef __LINUX__
      #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
      #endif
      #include <sched.h>
      #include <sys/prctl.h>
      #include <signal.h>

      typedef struct _gx_clone_stack {
          struct _gx_clone_stack *_next, *_prev;
          int (*child_fn)(void *);
          void *child_fn_arg;
          // Will be able to reduce the stack when we've solidified the clone's actions...
          char stack[0x1fff];
      } __attribute__((aligned)) _gx_clone_stack;

      gx_pool_init(_gx_clone_stack);
      static _gx_clone_stack_pool *_gx_csp __attribute__ ((unused)) = NULL;

      static void sigchld_clone_handler(int sig) {
          int status, saved_errno;
          pid_t child_pid;
          saved_errno = errno;
          while((child_pid = waitpid(-1, &status, __WCLONE | WNOHANG)) > 0) {
              finrelease__gx_clone_stack(_gx_csp, child_pid);
          }
          errno = saved_errno;  // For reentrancy
      }

      /// Wraps the clone target function so we can release the stack
      static int _gx_clone_launch(void *arg) {
          _gx_clone_stack *csp = (_gx_clone_stack *)arg;
          prctl(PR_SET_PDEATHSIG, SIGTERM);
          int res = csp->child_fn(csp->child_fn_arg);
          prerelease__gx_clone_stack(_gx_csp, csp);
          return res;
      }

      static GX_INLINE int gx_clone(int (*fn)(void *), void *arg) {
          int flags = SIGCHLD | CLONE_FILES   | CLONE_FS      | CLONE_IO      | CLONE_PTRACE |
                                CLONE_SYSVSEM | CLONE_VM;//      | CLONE_SIGHAND;
          _gx_clone_stack *cstack;

          if(gx_unlikely(!_gx_csp)) {
              // "Global" setup
              Xn(_gx_csp = new__gx_clone_stack_pool(1)) {X_FATAL; X_RAISE(-1);}
              struct sigaction sa;
              sigemptyset(&sa.sa_mask);
              sa.sa_flags = 0;
              sa.sa_handler = sigchld_clone_handler;
              X (sigaction(SIGCHLD, &sa, NULL)) {X_FATAL; X_RAISE(-1);}
          }
          Xn(cstack = acquire__gx_clone_stack(_gx_csp)  ) {X_FATAL; X_RAISE(-1);}
          cstack->child_fn = fn;
          cstack->child_fn_arg = arg;
          int cres;
          X(cres = clone(&_gx_clone_launch, (void *)(cstack->stack) + sizeof(cstack->stack) - 1, flags, (void *)cstack)) {X_FATAL; X_RAISE(-1);}
          return cres;
      }
    #else
        // exit(fn(arg)); // (in child)  wait- also release the stack here.
    #endif
#endif

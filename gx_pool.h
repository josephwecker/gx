#ifndef _GX_POOL_H
#define _GX_POOL_H

#include "./gx_error.h"

// This macros is intended to be used as EXTRA parameter to gx_pool_init_full when the
// memory pool allocates reference counted objects.
// Reference counted objects have to provide a "void *_pool" and "size_t _refc" fields
  #define GX_POOL_REFC                                                                   \
                                                                                         \
    static inline TYPE *acquire_ ## TYPE(TYPE ## _pool *pool) {                          \
        TYPE *res = NULL;                                                                \
        pthread_mutex_lock(&(pool->mutex));                                              \
        if(rare(!pool->available_head))                                                  \
            if(TYPE ## _pool_extend(pool, pool->total_items) == -1) goto fin;            \
        res = pool->available_head;                                                      \
        pool->available_head = res->_next;                                               \
        memset(res, 0, sizeof(TYPE));                                                    \
        _prepend_ ## TYPE(pool, res);                                                    \
      fin:                                                                               \
        pthread_mutex_unlock(&(pool->mutex));                                            \
        if(likely(res != NULL)) {                                                        \
            if(unlikely(CONSTRUCT(res) != 0)) {                                          \
                release_ ## TYPE(res);                                                   \
                res = NULL;                                                              \
            }                                                                            \
        }                                                                                \
        if(likely((res = (acquire_ ## TYPE(pool))) != NULL)) {                           \
            res->_refc = 1;                                                              \
            res->_pool = pool;                                                           \
        }                                                                                \
        return res;                                                                      \
    }                                                                                    \
                                                                                         \
    static inline void TYPE ## _incr_refc(TYPE *entry) {                                 \
        ++(entry->_refc);                                                                \
    }                                                                                    \
                                                                                         \
    static inline void TYPE ## _decr_refc(TYPE *entry) {                                 \
        if((--(entry->_refc)) == 0) {                                                    \
            release_ ## TYPE(entry->_pool, entry);                                       \
        }                                                                                \
    }

// This is macro is intended to be used as EXTRA parameter to gx_pool_init_full when the
// memory pool allocates simple objects.
  #define GX_POOL_SIMPLE                                                                 \
                                                                                         \
    static inline TYPE *acquire_ ## TYPE(TYPE ## _pool *pool) {                          \
        TYPE *res = NULL;                                                                \
        pthread_mutex_lock(&(pool->mutex));                                              \
        if(rare(!pool->available_head))                                                  \
            if(TYPE ## _pool_extend(pool, pool->total_items) == -1) goto fin;            \
        res = pool->available_head;                                                      \
        pool->available_head = res->_next;                                               \
        memset(res, 0, sizeof(TYPE));                                                    \
        _prepend_ ## TYPE(pool, res);                                                    \
      fin:                                                                               \
        pthread_mutex_unlock(&(pool->mutex));                                            \
        if(likely(res != NULL)) {                                                        \
            if(unlikely(CONSTRUCT(res) != 0)) {                                          \
                release_ ## TYPE(res);                                                   \
                res = NULL;                                                              \
            }                                                                            \
        }                                                                                \
        return res;                                                                      \
    }

// TYPE must be a type that has a "_next" and "_prev" member that is a pointer to the same
// type. Also, the struct should be aligned (?).
  #define gx_pool_init_full(TYPE, ALLOCATE, DEALLOCATE, CONSTRUCT, DESTROY, EXTRA)       \
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
        TYPE                                *prereleased[0x10000];                       \
        pool_memory_segment ## TYPE         *memseg_head;                                \
    } TYPE ## _pool;                                                                     \
                                                                                         \
    static int TYPE ## _pool_extend(TYPE ## _pool *pool, size_t by_number);              \
    static inline TYPE ## _pool *new_  ##  TYPE ## _pool (size_t initial_number) {       \
        TYPE ## _pool *res;                                                              \
        _N(res=(TYPE ## _pool *)malloc(sizeof(TYPE ## _pool))) _raise(NULL);             \
        memset(res, 0, sizeof(TYPE ## _pool));                                           \
        TYPE ## _pool_extend(res, initial_number);                                       \
        _ (pthread_mutex_init(&(res->mutex), NULL)) _raise(NULL);                        \
        return res;                                                                      \
    }                                                                                    \
                                                                                         \
    static inline void destroy_ ## TYPE ## _pool(TYPE ## _pool *pool) {                  \
        pool_memory_segment ## TYPE * seg1;                                              \
        pool_memory_segment ## TYPE * seg2;                                              \
        TYPE *ptr;                                                                       \
        if(pool != NULL) {                                                               \
            pthread_mutex_lock(&(pool->mutex));                                          \
            for(ptr = pool->available_head; ptr != NULL; ptr = ptr->_next) {             \
                DEALLOCATE(ptr);                                                         \
            }                                                                            \
            seg1 = pool->memseg_head;                                                    \
            while(seg1 != NULL) {                                                        \
                seg2 = seg1;                                                             \
                seg1 = seg1->next;                                                       \
                free(seg2->segment);                                                     \
                free(seg2);                                                              \
            }                                                                            \
            pthread_mutex_unlock(&(pool->mutex));                                        \
        }                                                                                \
    }                                                                                    \
                                                                                         \
    static int TYPE ## _pool_extend(TYPE ## _pool *pool, size_t by_number) {             \
        TYPE *new_seg;                                                                   \
        size_t curr;                                                                     \
        pool_memory_segment ## TYPE *memseg_entry;                                       \
        _N(new_seg = (TYPE *)malloc(sizeof(TYPE) * by_number)) _raise(-1);               \
                                                                                         \
        /* Link to memory-segments for freeing later */                                  \
        _N(memseg_entry = (pool_memory_segment ## TYPE *)                                \
           malloc(sizeof(pool_memory_segment ## TYPE))) _raise(-1);                      \
        memseg_entry->segment = new_seg;                                                 \
        memseg_entry->next    = pool->memseg_head;                                       \
                                                                                         \
        /* Link them up */                                                               \
        new_seg[by_number - 1]._next = (TYPE *)pool->available_head;                     \
        pool->available_head = new_seg;                                                  \
        for(curr=0; curr < by_number-1; curr++) {                                        \
            if(unlikely(ALLOCATE(new_seg + curr) != 0)) {                                \
                while(curr-- != 0) {                                                     \
                    DEALLOCATE(new_seg + curr);                                          \
                }                                                                        \
                free(memseg_entry);                                                      \
                free(new_seg);                                                           \
                _raise(-1);                                                              \
            }                                                                            \
            new_seg[curr]._next = (TYPE *)(new_seg + curr + 1);                          \
        }                                                                                \
        pool->total_items += by_number;                                                  \
        pool->memseg_head  = memseg_entry;                                               \
        return 0;                                                                        \
    }                                                                                    \
                                                                                         \
    static inline void _prepend_ ## TYPE (TYPE ## _pool *pool, TYPE *entry) {            \
        /* put an object at the front of the active list */                              \
        entry->_next = pool->active_head;                                                \
        if (freq(pool->active_head != NULL)) { pool->active_head->_prev = entry; }       \
        pool->active_head = entry;                                                       \
        if (rare(pool->active_tail == NULL)) { pool->active_tail = entry; }              \
    }                                                                                    \
                                                                                         \
    static inline void _remove_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {              \
        /* remove an object from the active list */                                      \
        if (freq(entry->_prev != NULL)) { entry->_prev->_next = entry->_next; }          \
        else { pool->active_head = entry->_next; }                                       \
        if (freq(entry->_next != NULL)) { entry->_next->_prev = entry->_prev; }          \
        else { pool->active_tail = entry->_prev; }                                       \
    }                                                                                    \
                                                                                         \
    static inline void prerelease_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {           \
        pthread_mutex_lock(&(pool->mutex));                                              \
        pid_t cpid;                                                                      \
        cpid = syscall(SYS_getpid);                                                      \
        unsigned int idx = (unsigned int)cpid & 0xffff;                                  \
        if(rare(pool->prereleased[idx]))                                                 \
            log_error("Snap, prerelease snafu: %d", idx);                                \
        pool->prereleased[idx] = entry;                                                  \
        pthread_mutex_unlock(&(pool->mutex));                                            \
    }                                                                                    \
                                                                                         \
    static inline void finrelease_ ## TYPE(TYPE ## _pool *pool, pid_t cpid) {            \
        pthread_mutex_lock(&(pool->mutex));                                              \
        unsigned int idx = (unsigned int)cpid & 0xffff;                                  \
        TYPE *entry = pool->prereleased[idx];                                            \
        if(rare(!entry)) log_error("Snap, prerelease snafu: %d", idx);                   \
        _remove_ ## TYPE(pool, entry);                                                   \
        entry->_next = pool->available_head;                                             \
        pool->available_head = entry;                                                    \
        pool->prereleased[idx] = NULL;                                                   \
        pthread_mutex_unlock(&(pool->mutex));                                            \
    }                                                                                    \
                                                                                         \
    static inline void release_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {              \
        DESTROY(entry);                                                                  \
        pthread_mutex_lock(&(pool->mutex));                                              \
        _remove_ ## TYPE(pool, entry);                                                   \
        entry->_next = pool->available_head;                                             \
        pool->available_head = entry;                                                    \
        pthread_mutex_unlock(&(pool->mutex));                                            \
    }                                                                                    \
                                                                                         \
    static inline void move_to_front_ ## TYPE(TYPE ## _pool *pool, TYPE *entry) {        \
        /* move an object to the front of the active list */                             \
        _remove_ ## TYPE(pool, entry);                                                   \
        _prepend_ ## TYPE(pool, entry);                                                  \
    }                                                                                    \
                                                                                         \
    EXTRA

  #define gx_pool_init(TYPE)                                                             \
    static inline int _allocate_   ## TYPE(TYPE *object) { (void)object; return 0; }     \
    static inline int _deallocate_ ## TYPE(TYPE *object) { (void)object; return 0; }     \
    static inline int _construct_  ## TYPE(TYPE *object) { (void)object; return 0; }     \
    static inline int _destroy_    ## TYPE(TYPE *object) { (void)object; return 0; }     \
                                                                                         \
    gx_pool_init_full(TYPE,                                                              \
                      _allocate_   ## TYPE,                                              \
                      _deallocate_ ## TYPE,                                              \
                      _construct_  ## TYPE,                                              \
                      _destroy_    ## TYPE,                                              \
                      GX_POOL_SIMPLE)


#endif

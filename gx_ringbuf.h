#ifndef GX_RINGBUF_H
#define GX_RINGBUF_H
#include "./gx.h"
#include "./gx_error.h"
#include "./gx_endian.h"
#include <sys/mman.h>

//=============================================================================
/**
 * Very fast RING BUFFER (gx_rb*)
 * Uses the mmap trick to map the same memory right next to itself. Rounds
 * desired size up to nearest page-size (which is good for things like vmsplice
 * anyway).
 *
 * TODO:
 *  - turn it into a resource pool
 *  - possibly better bounds check when write gets too far ahead (no more
 *    space)
 *  - macros/inlines for writing/reading that appropriately advance the
 *    pointers as well. (gx_rb_memcpy ...)
 *  - check for reading past the write-head
 *  - rb_read, if it's ever needed.
 *  - fix documentation
 */

//-----------------------------------------------------------------------------
/// Ring Buffer
typedef struct gx_rb {
    struct gx_rb *next; ///< For when it's used in a resource pool
    void         *addr; ///< Actual mmap region
    int           fd;   ///< File descriptor associated w/ mmap region
    ssize_t       len;  ///< Total size
    ssize_t       w;    ///< Write head / offset    TODO: these should probably be size_t instead
    ssize_t       r;    ///< Read head  / offset
} gx_rb;


//-----------------------------------------------------------------------------
/// Allocate memory / initiate new ring-buffer.
///
/// @param rb            Allocated ringbuffer structure
/// @param min_size      Minimum size of the ring buffer
///                      (rounds up to system pages)
/// @param stay_in_ram   If non-zero, ring-buffer gets "locked" into RAM-
///                      doesn't swap out.
static int gx_rb_create(gx_rb *rb, ssize_t min_size, int stay_in_ram) {
#ifdef __LINUX__
    char    path[] = "/dev/shm/rb-XXXXXX"; // Doesn't actually get written to
#else
    char    path[] = "/tmp/rb-XXXXXX";
#endif
    int     fd;
    void   *addr;
    int     page_size = pagesize(); // Configurable, so discover @ runtime
    _ (fd=mkstemp(path) )      _raise_error(-1);
    _ (unlink(path)     )      _raise_error(-1);
    rb->len = gx_fits_in(page_size, min_size) * page_size;
    rb->w   = rb->r = 0;
    _ (ftruncate(fd, rb->len)) _raise_error(-1);
    // First call finds a good chunk- second and third one map the same
    // memory twice so it loops back on itself.
    int flags = MAP_FIXED | MAP_SHARED;
    _M(rb->addr=mmap(NULL, rb->len<<1, PROT_NONE,MAP_ANON|MAP_PRIVATE, -1, 0)    ) _raise_error(-1);
    _M(addr =   mmap(rb->addr,        rb->len, PROT_READ|PROT_WRITE, flags, fd,0)) _raise_error(-1);
    _M(addr =   mmap(rb->addr+rb->len,rb->len, PROT_READ|PROT_WRITE, flags, fd,0)) _raise_error(-1);
    if(stay_in_ram) {
        mlock(rb->addr, rb->len); // Any errors here are non-fatal, so ignoring
        mlock(rb->addr+rb->len, rb->len);
    }
    rb->fd = fd; // used for sendfile, etc.- otherwise could be closed here np
    return 0;
}

/// Address for the current write position for directly writing.
static inline void *rb_w(gx_rb *rb) {return rb->addr + rb->w;}
/// Address for the current read position for directly reading.
static inline void *rb_r(gx_rb *rb) {return rb->addr + rb->r;}

/// Address for the current write position for directly writing.
static inline uint8_t *rb_uintw(gx_rb *rb) {return (uint8_t*) (rb->addr + rb->w);}
/// Address for the current read position for directly reading.
static inline uint8_t *rb_uintr(gx_rb *rb) {return (uint8_t*) (rb->addr + rb->r);}

/// Advance write cursor (after writing directly usually).
static inline void rb_advw (gx_rb *rb, ssize_t len) {rb->w += len;}
/// Advance read cursor (after reading directly usually).
static inline void rb_advr (gx_rb *rb, ssize_t len) {
    rb->r+=len;
    if(rb->r >= rb->len) {
        rb->r -= rb->len;
        rb->w -= rb->len;
    }
}
/// Write into ringbuffer from other buffer and advance write head. Essentially memcpy
static inline ssize_t rb_write (gx_rb *rb, const void *src, ssize_t len) {
    memcpy(rb_w(rb), src, len);
    rb_advw(rb, len);
    return len;
}

static inline void rb_wbyte(gx_rb *rb, uint8_t data) {
    ((uint8_t *)rb_w(rb))[0] = data;
    rb_advw(rb, 1);
}

static inline void rb_wbe16(gx_rb *rb, uint16_t data) {
    rb_wbyte(rb, (data & 0xFF00) >> 8);
    rb_wbyte(rb, (data & 0x00FF)     );
}

static inline void rb_wbe24(gx_rb *rb, uint32_t data) {
    rb_wbyte(rb, (data & 0x00FF0000U) >> 16);
    rb_wbyte(rb, (data & 0x0000FF00U) >>  8);
    rb_wbyte(rb, (data & 0x000000FFU)      );
}

static inline void rb_wbe32(gx_rb *rb, uint32_t data) {
    rb_wbyte(rb, (data & 0xFF000000U) >> 24);
    rb_wbyte(rb, (data & 0x00FF0000U) >> 16);
    rb_wbyte(rb, (data & 0x0000FF00U) >>  8);
    rb_wbyte(rb, (data & 0x000000FFU)      );
}

static inline void rb_wse32(gx_rb *rb, uint32_t data) {
    if(rare(gx_is_big_endian())) {
        log_critical("Small endian conversion not implemented.");
        return;
    }
    rb_write(rb, &data, 4);
}

//static inline void rb_wbe64(gx_rb *rb, uint64_t data) {
static inline void rb_wbe64(gx_rb *rb, void *dat) {
    uint64_t data = *((uint64_t *)dat);
    rb_wbyte(rb, (data & 0xFF00000000000000ULL) >> 56);
    rb_wbyte(rb, (data & 0x00FF000000000000ULL) >> 48);
    rb_wbyte(rb, (data & 0x0000FF0000000000ULL) >> 40);
    rb_wbyte(rb, (data & 0x000000FF00000000ULL) >> 32);
    rb_wbyte(rb, (data & 0x00000000FF000000ULL) >> 24);
    rb_wbyte(rb, (data & 0x0000000000FF0000ULL) >> 16);
    rb_wbyte(rb, (data & 0x000000000000FF00ULL) >>  8);
    rb_wbyte(rb, (data & 0x00000000000000FFULL)      );
}

/// Gives the current write position and pre-advances the write-head however
/// many bytes you're about to write.
static inline void *rb_write_adv(gx_rb *rb, ssize_t length) {
    void *res = rb_w(rb);
    rb_advw(rb, length);
    return res;
}

static inline void *rb_read_adv(gx_rb *rb, ssize_t length) {
    void *res = rb_r(rb);
    rb_advr(rb, length);
    return res;
}
/// Reset ringbuffer.
static inline void rb_clear(gx_rb *rb) {rb->w = rb->r = 0;}
/// Number of bytes currently stored in ringbuffer but not read yet.
static inline ssize_t rb_used(gx_rb *rb) {return rb->w - rb->r;}
/// Bytes available still for writing
static inline ssize_t rb_available (gx_rb *rb) {return rb->len - rb_used(rb);}
/// Free allocated memory for internal structures (and close filehandle).
static inline int rb_free (gx_rb *rb) {
    _ (munmap(rb->addr, rb->len << 1)) _raise(-1);
    close(rb->fd);
    return 0;
}


/*=============================================================================
 * Reusable pool of ring buffers
 *
 * Like the normal gx_pool stuff except optimized a bit more for the special
 * case of ring-buffers, which are already doing some memory-mapping etc.
 * Differences:
 *  - Function name conventions
 *  - Functions not generated to be static
 *  - gx_rb_pool_new gets ring-buffer params as well
 *  - Specify how many rbs you want to stay in RAM (starting w/ most active)
 *  - Calls the initializer a little differently.
 *
 * TODO:
 *  - Figure out a clean way to share most changes here w/ gx.h pool
 *    implementation so they don't diverge too much / become a maintenance
 *    headache.
 *  - If there's a clean way, drop the malloc call and put the struct inside
 *    the mmap (adjusting number of pages as necessary). Might not be worth the
 *    saved malloc calls - then again, might be if it improves cache
 *    efficiency.
 *  - correctly free them if a pool is freed (in fact, finish being able to
 *    free a pool in the first place).
 *  - Make sure the correct error conditions are propagated up in such a way
 *    that allows for some kind of recovery.
 *  - Much smaller increments instead of exponential
 *
 *---------------------------------------------------------------------------*/
typedef struct gx_rb_poolseg {
    struct gx_rb_poolseg *next;
    gx_rb                *segment;
} gx_rb_poolseg;

typedef struct gx_rb_pool {
    ssize_t               total_items;
    ssize_t               min_rbsize;
    gx_rb                *available_head;
    gx_rb_poolseg        *memseg_head;
} gx_rb_pool;

static inline int gx_rb_pool_extend(gx_rb_pool *pool, ssize_t by_number, ssize_t num_in_ram);

/*---------------------------------------------------------------------------*/
static inline gx_rb_pool *gx_rb_pool_new (ssize_t initial_number, ssize_t min_rbsize,
                                         ssize_t num_in_ram) {
    gx_rb_pool *res;
    _N(res=(gx_rb_pool *)malloc(sizeof(gx_rb_pool))      ) _raise(NULL);
    memset(res, 0, sizeof(gx_rb_pool));
    if(min_rbsize <= 0) min_rbsize = pagesize();
    res->min_rbsize = min_rbsize;
    _ (gx_rb_pool_extend(res, initial_number, num_in_ram)) _raise(NULL);
    return res;
}

/*---------------------------------------------------------------------------*/
static int gx_rb_pool_extend(gx_rb_pool *pool, ssize_t by_number, ssize_t num_in_ram) {
    gx_rb *new_seg;
    ssize_t curr;
    gx_rb_poolseg *memseg_entry;
    _N(new_seg = (gx_rb *)malloc(sizeof(gx_rb) * by_number)) _raise(-1);

    /* Link to memory-segments for freeing later */
    _N(memseg_entry=(gx_rb_poolseg *)malloc(sizeof(gx_rb_poolseg))) _raise(-1);
    memseg_entry->segment = new_seg;
    memseg_entry->next    = pool->memseg_head;
    pool->memseg_head     = memseg_entry;

    /* Link them up */

    // Create ringbuffer for the last one in the new pool
    _ (gx_rb_create(&new_seg[by_number - 1], pool->min_rbsize, num_in_ram > 0)) _raise(-1);
    if(num_in_ram > 0) num_in_ram --;

    // Last in new pool links to the current available-head
    new_seg[by_number - 1].next = (struct gx_rb *)pool->available_head;
    // Point available-head to the first of the new segment
    pool->available_head = new_seg;

    // First through second-to-last
    for(curr=0; curr < by_number-1; curr++) {
        _ (gx_rb_create(&new_seg[curr], pool->min_rbsize, num_in_ram > 0)) _raise(-1);
        if(num_in_ram > 0) num_in_ram --;
        new_seg[curr].next = (struct gx_rb *) &(new_seg[curr+1]);
    }
    pool->total_items += by_number;

#if 0
    //----- DEBUG DUMP LINKED LIST --------
    gx_rb *c = pool->available_head;
    gx_log_debug("ringbuf pool:\n"); int j = 0;
    while(c) { fprintf(stderr, "  %d: %p -> %p\n", j, c, c->next); c = c->next; j++; }
    fprintf(stderr, "----------\n%ld\n\n", pool->total_items);
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/
static inline gx_rb *gx_rb_acquire(gx_rb_pool *pool) {
    gx_rb *res;
    if(rare(!pool->available_head))
        if(gx_rb_pool_extend(pool, pool->total_items, 0) == -1) return NULL;
    res = pool->available_head;
    pool->available_head = res->next;
    rb_clear(res); // Don't want to completely clear the structure because it has preallocated mmapped fd etc.
    return res;
}

/*---------------------------------------------------------------------------*/
static inline void gx_rb_release(gx_rb_pool *pool, gx_rb *entry) {
    entry->next = pool->available_head;
    pool->available_head = entry;
}
/*---------------------------------------------------------------------------*/
#endif

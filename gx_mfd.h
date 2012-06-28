/**
 * Fun with memory-maps. The fruits of persistence! Faster than inotify (etc.)
 * for stuff you are pretty sure is going to be in the page-cache anyway
 * (recently used files etc.) Not necessarily file-based- or files that will
 * stay after the process is gone (like, for example, the pre-unlinked files
 * that gx_ringbuf uses).
 *
 * Including a function that returns a filedescriptor that can be used in epoll
 * etc. when a variable is changed (usually a memory-mapped variable changed by
 * another process). Coded to be very fast, at least for Linux.
 *
 * At some point I'll probably put in here mome mmap simplifications /
 * abstractions in order to take advantage of various optimizations since
 * that'll be the primary usecase for memfd.
 *
 *  http://gustedt.wordpress.com/2011/01/28/linux-futexes-non-blocking-integer-valued-condition-variables/
 * 
 * RANDOM:
 *  (mac osx backup: memory poll w/ sleep, or something similar)
 *  use pipe pair or something even cheaper (if exists) instead of eventfd for
 *  portability...
 *
 *
 * USAGE:
 *  - Allocate gx_mfd pool:     new_gx_mfd_pool (INITIAL_SIZE)    --> mfd_pool or NULL on error
 *  - Get gx_mfd from pool:     acquire_gx_mfd  (mfd_pool)        --> actual gx_mfd
 *  - Initialize writer gx_mfd: gx_mfd_create_w (mfd,* char *path)--> -1 on error, otherwise mfd->fd.
 *  - Prepare to read or write: mfd_c_adv       (mfd*, length)    --> pointer to where you can now read/write length bytes.
 *  - Update fd's offset to c:  mfd_autoseek    (mfd*)            --> -1 on error, in preparation for fd based read/write/sendfile/etc.
 *  - Update c to fd's offset:  mfd_autotell    (mfd*)            --> -1 on error
 *  - Tell readers re new data: mfd_broadcast   (mfd*)            --> -1 on error
 *
 *  - Get "watcher" fd:
 *
 * TODO:
 *  - Abstraction for auto-update-files from writer perspective
 *    * Gives the fd back for subsequent operations (for zerocopy ones, that is).
 *    * Gives back also a pointer and some abstractions for writing directly to
 *      the memory (which re-mmaps as appropriate etc.)
 *    * Optimize (initially) for serialized writing + updating that very first
 *      page.
 *    * Avoid syncing (under the assumption that anything that wants to see the
 *      data is using memory-mappings as well) but expose the interface for it.
 *    * Trigger for waking up waiting processes (FUTEX_WAKE on linux, nothing
 *      on mac-osx, which is going to be polling) - automatic where possible
 *      and exposed where not.
 *  - Abstraction for monitoring. i.e., the reader perspective
 *  - madv-dontneed on pages that aren't relevant anymore- maybe a madvise
 *    wrapper for quickly expressing your "state" in the rest of the
 *    memory-mapped file.
 *  - Be sure readonly stuff is set on readers
 *  - Make sure and mark variable as volatile (in the smallest appropriate
 *    scope ala wikipedia)
 *  
 *
 *  - Possibly lock first page of every mfd into RAM?
 *
 */
#ifndef GX_MFD_H
#define GX_MFD_H

#include <gx/gx.h>
#include <gx/gx_error.h>

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>


#define _GX_MFD_ERR_IF     { X_LOG_ERROR("Not a properly formatted file for mfd (found other data inside)."); X_RAISE(-1);}
#define _GX_MFD_FILESIG    UINT64_C(0x1c1c1c1c1c1c1c1c)

#define GX_MFD_RO 0
#define GX_MFD_WO 1

typedef struct gx_mfd_head {
    uint64_t        sig;           ///< Will always be 0x1c... - so we don't clobber some unsuspecting file not made for this.
    uint64_t        size;          ///< Data size (not including header). Host-endian and not necessarily accurate; used as a kind of semaphore.
} gx_mfd_head;

typedef struct gx_mfd {
    struct gx_mfd  *_next, *_prev; ///< For resource pooling
    int             type;          ///< Readonly or writeonly at the moment
    int             fd;            ///< File descriptor, ready for IO operations
    size_t          c;             ///< Current pointer (write or read position depending on context)
    size_t          map_size;      ///< Currently allocated map-size- may or may not correspond to filesize (usually it'll be larger)
    void           *full_map;      ///< For remapping etc.
    gx_mfd_head    *head;          ///< Points to the very front of the mapped file
    void           *dat;           ///< Points to the data- just after the head
} gx_mfd;

gx_pool_init(gx_mfd);


/** Initialize an mfd struct for a given file, mapping the file for writes etc. */
static int gx_mfd_create_w(gx_mfd *mfd, const char *path) {
    struct stat  filestat;
    off_t        curr_size;

    X(  mfd->fd=open(path,O_RDWR|O_NONBLOCK|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) X_RAISE(-1);
    X(  flock(mfd->fd, LOCK_EX | LOCK_NB)                                                     ) X_RAISE(-1);
    X(  fstat(mfd->fd, &filestat)                                                             ) X_RAISE(-1);
    curr_size = filestat.st_size;
    mfd->map_size = MAX(curr_size, gx_pagesize);
    Xm( mfd->full_map = mmap(NULL, mfd->map_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd->fd,0)) X_RAISE(-1);
    mfd->head = (gx_mfd_head *)(mfd->full_map);
    mfd->dat  = mfd->head + sizeof(gx_mfd_head);
    mfd->type = GX_MFD_WO;
    if(curr_size > 0) {
        // There should be a valid header in place then.
        if(mfd->head->sig != _GX_MFD_FILESIG) _GX_MFD_ERR_IF;
        if(curr_size < sizeof(gx_mfd_head))   _GX_MFD_ERR_IF;
        // TODO: possibly warn if curr_size is very wrong indicating some kind
        //       of inconsistent state last time this stuff was run...
        mfd->head->size = curr_size - sizeof(gx_mfd_head);
    } else {
        // Using write so that the calling process has the offset pointer
        // pointing to the actual data section.
        uint64_t sig = _GX_MFD_FILESIG;
        write(mfd->fd, &sig, sizeof(uint64_t));
        sig = 0;
        write(mfd->fd, &sig, sizeof(uint64_t));
    }
    mfd->c = 0;
    return mfd->fd;
}

/// Preferred method for reading/writing. Returns a pointer to the "current"
/// position after preparing for user to read or write 'length' bytes of data.
static GX_INLINE void *mfd_c_adv(gx_mfd *mfd, size_t length) {

}

// TODO: (if needed)  mfd_seek(...) - does lseek to move actual fd
// filepointer to the current cursor before normal IO reads/writes.



/// Address for current cursor
static GX_INLINE void *mfd_c(gx_mfd *mfd) {return mfd->dat + mfd->c;}

/// Advance the read/write cursor
/// TODO: Always keep at least a couple of blank pages in front so that a
///       spontaneous write or read is guaranteed to be able to do at least
///       page-size (and/or 4096 bytes).
/// TODO: Ask for average throughput when initializing to optimize "pages at a
///       time" so it's not doing a mremap every 4096 bytes shovelled...
static GX_INLINE int mfd_adv(gx_mfd *mfd, size_t len) {
    // TODO:
    //  - If @ mmap boundary:
    //    - If write-context, ftruncate (can be avoided w/o sigbus?)
    //    - Remap
    //    - madvise on new & older pages
    //  - On Linux, trigger a FUTEX_WAKE
}

// TODO: (as needed)
//  mfd_write(mfd*, void *src, len)
//  mfd_wbyte, wbe16/24/32/64, wse16/24/32/64, etc.


/** Tell system that a write (using some standard IO or zerocopy) occurred so
 * it can notify readers.
 *
static GX_INLINE int gx_mfd_did_write(gx_mfd *mfd, size_t just_wrote) {

    return 0;
}

//static GX_INLINE int gx_mfd_write(gx_mfd *mfd, 
*/




#endif

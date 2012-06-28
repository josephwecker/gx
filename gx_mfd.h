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
 *
 * TODO:
 *  - Abstraction for auto-update-files from writer perspective
 *    * Opens for writing, respecting any advisory locks
 *    * Acquires write locks etc.
 *    * mmaps
 *    * Fills in initial standard structure if required (and ftruncates, or
 *      just use write so fd is all queued up correctly).
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
 */
#ifndef GX_MFD_H
#define GX_MFD_H

#include <gx/gx.h>
#include <gx/gx_error.h>

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>


typedef struct gx_mfd_head {
    uint64_t     sig;        // Will always be 0x1c1c1c1c1c1c1c1c
    uint64_t     size;       // Data size (not including header). Host-endian
                             //     and not necessarily accurate- used as a
                             //     kind of semaphore.
} gx_mfd_head;

typedef struct gx_mfd {
    struct gx_mfd *_next, *_prev;
    int          fd;
    void        *full_map;
    gx_mfd_head *head;
} gx_mfd;

gx_pool_init(gx_mfd);

static gx_mfd_open_writer(gx_mfd *mfd, const char *path) {
    int          fd;
    void        *map;
    struct stat  filestat;
    off_t        curr_size;

    X(  fd = open(path, O_RDWR|O_NONBLOCK|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)  ) X_RAISE(-1);
    X(  flock(fd, LOCK_EX | LOCK_NB)                                                          ) X_RAISE(-1);
    X(  fstat(fd, &filestat)                                                                  ) X_RAISE(-1);
    curr_size = filestat.st_size;
    Xm( map = mmap(NULL, MAX(curr_size, gx_pagesize), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) X_RAISE(-1);
    if(curr_size > 0) { // Should be valid header in place
        if(((uint64_t *)map)[0] != 0x1c1c1c1c1c1c1c1cUL){X_LOG_ERROR("Not a proper mfd file."); X_RAISE(-1);}
        // TODO: possibly warn if curr_size is very wrong indicating some kind of bad state.
    } else {

    }
}




#endif

/**
 * Fun with memory-maps. The fruits of persistence! Faster than inotify (etc.)
 * for stuff you are pretty sure is going to be in the page-cache anyway
 * (recently used files etc.) Not necessarily file-based- or files that will
 * stay after the process is gone (like, for example, the pre-unlinked files
 * that gx_ringbuf uses).
 *
 * Including a function that returns a filedescriptor that can be used in epoll
 * etc. when a variable is changed (usually a memory-mapped variable changed by
 * another process). Coded to be very fast, at least for Linux. At this point
 * mostly used as a very, very fast alternative to inotify etc. that works even
 * when the data hasn't been written to any inodes etc.
 *
 * At some point I'll probably put in here mome mmap simplifications /
 * abstractions in order to take advantage of various optimizations since
 * that'll be the primary usecase for memfd.
 *
 *  http://gustedt.wordpress.com/2011/01/28/linux-futexes-non-blocking-integer-valued-condition-variables/
 * Also, for mac-osx mremap emulation, see:
 *  https://dank.qemfd.net/bugzilla/show_bug.cgi?id=119
 * 
 * USAGE:
 * RW|Allocate gx_mfd pool    |new_gx_mfd_pool(SIZE)     |mfd_pool or NULL on error
 * RW|Get gx_mfd from pool    |acquire_gx_mfd (mfd_pool) |actual gx_mfd
 *  W|Initialize writer gx_mfd|gx_mfd_create_w(mfd*,path)|-1 on error, otherwise mfd->fd.
 * R |Initialize reader gx_mfd|gx_mfd_create_r(mfd*,path)|-1 or populate mfd & mfd->watch_fd
 * RW|Prepare to read or write|mfd_c_adv      (mfd*,len) |pointer to where you can now r/w len bytes.
 * RW|Update fd's offset to c |mfd_autoseek   (mfd*)     |-1=err, do before fd based read/write/sendfile/etc
 * RW|Update c to fd's offset |mfd_autotell   (mfd*)     |-1 on error
 *  W|Tell readers re new data|mfd_broadcast  (mfd*)     |-1 on error
 *
 *
 * TODO:
 *  - Abstraction for auto-update-files from writer perspective
 *    * Trigger for waking up waiting processes (FUTEX_WAKE on linux, nothing
 *      on mac-osx, which is going to be polling) - automatic where possible
 *      and exposed where not.
 *  - Be sure readonly stuff is set on readers
 *  - Make sure and mark variable as volatile (in the smallest appropriate
 *    scope ala wikipedia)
 *  - Programatically check and warn on bad system limitations
 *  -  W| |mfd_write(mfd*, void *src, len)
 *  -  W| |mfd_wbyte, wbe16/24/32/64, wse16/24/32/64, etc.
 *
 * RELEVANT SYSTEM LIMITATIONS:
 *  - vm.max_map_count  (65530)
 *  - vm.swappiness (60 - should be more ~ 20)
 *  - ulimit / open file descriptors
 *  - virtual memory available per process (RLIMIT_AS)
 *  - RLIMIT_MEMLOCK (needs one per writer-mfd at least)
 *
 * IMPLEMENTATION (again):
 *
 *  - (optimized for sequential write/read, but will allow for random access)
 *  - (optimized for data persistence, but can easily add unlinking and use
 *     ram-based filesystems if needed).
 *
 *  - [D] Open mfd file or (WRITER) create one
 *  - [D] Turn off atime modifications to avoid unnecessary IO
 *  - [D] If a writer, lock the file (other writer attempts will fail)
 *  - [D] Map only the first page, lock it in RAM, and point the header structure
 *    to it. It will be redundant but only use up (essentially limitless)
 *    virtual memory. Must be locked into RAM for futexes / notifications to
 *    work well etc.
 *  - Check for a correct header if there is any data
 *
 *  (WRITER only)
 *  - Create a correct header if there is none
 *  - Map the entire file + page_precache pages
 *  - ftruncate or lseek+write to extend _just barely_ into the last mapped
 *    page to eliminate sigbus signals. On Linux at least, if contents to
 *    happen to write out to disk, a hole will be created and therefore actual
 *    disk-space won't really be affected.
 *  - Update header, trigger futex-wake on the current size in page-locked
 *    header, and wait for write operations.
 *  - On a write operation, mremap (or mac equiv) occurs if needed, including
 *    ftruncate et al. Madvise any new pages to be sequential.
 *  - After a write operation, size variable is changed and futex-wake invoked
 *    (on Linux).
 *  - When write cursor advances across page boundaries, mark older pages as
 *    dontneed.
 *  - When mfd is closed, clean up futex, clean up memory maps, clean up file
 *    as appropriate (for persistent mfds, be sure and ftruncate to the end of
 *    the last page instead of the beginning of it), etc.
 *
 *  (READER only)
 *  - Map the entire file + page_precache pages - but all PROT_READ / RONLY,
 *    etc. to cause very efficient kernel optimizations. (possibly same w/
 *    memory-locked header page, but only if it doesn't interfere with the
 *    futex).
 *  - Create a new pipe pair
 *  - Do the cheapest sys_clone possible (vfork on mac, alas), and have one
 *    branch return the read-end of the pipe (or have it as part of the mfd
 *    structure that will be referenced).
 *  - Have the other end do the following:
 *    - Prepare to handle any specific signals indicating that the writer is
 *      done or futex doesn't exist any more etc. but without disturbing the
 *      main process if at all possible.
 *    - Prepare to gracefully shut down when the parent process needs it to for
 *      any reason.
 *    - Block on a volatile futex created on the "size" part of memorymapped
 *      header - simply waiting for _any_ change on the variable (hence mfd =
 *      memory-file-descriptor).
 *    - When futex is awakened, current size is written to the pipe, and then
 *      the futex is reacquired.
 *    - All of the above will be a simple poll/sleep loop for mac osx plus the
 *      signal handling.
 *  - User can put the "notification fd" (read side of the pipe) in an event
 *    loop and trigger simple memory read operations whenever desired.
 *  - When read operations advance the cursor across page boundaries, mark
 *    earlier pages as dontneed and new pages as sequential, like the writer,
 *    BUT obviously don't do ftruncate or anything. Any sigbus errors are
 *    indications that there are serious problems w/ disk or with the
 *    writer-side, so allow it to propagate at least to parent.
 *
 *  (FUTURE)
 *  - Non-persistent (or, "briefly" persistent) flag:
 *    Flag on init to decide if the file is going to persist. If not, when the
 *    mfd is closed: (1) unlink, (2) msync(DIRTY), (3) close-fd, (4) munmap -
 *    to avoid disk churn as much as possible (or RAM churn if /dev/shm)
 *    Unfortunately going to have some churn regardless while the file is
 *    building or else new processes wouldn't be able to find it.
 *  - Also, possibly for non-persistent/non-random-access mfds: on Linux we can
 *    possibly madvise pages to be completely freed when they're on a ram-based
 *    filesystem
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
    uint64_t  sig;   ///< Will always be 0x1c... - so we don't clobber some unsuspecting file not made for this.
    uint64_t  size;  ///< Data size (not including header). Host-endian and not necessarily accurate; used as a kind of semaphore.
} gx_mfd_head;

typedef struct gx_mfd {
    struct gx_mfd    *_next, *_prev; ///< For resource pooling
    int               type;          ///< Readonly or writeonly at the moment
    int               premap;        ///< Pages at a time to map- higher avoids more remapping+syscalls
    int               fd;            ///< File descriptor, ready for IO operations
    size_t            real_fsize;    ///< File is currently truncated to this size
    union {
        void         *head_map;      ///< Same as first page of map, but locked into RAM
        gx_mfd_head  *header;        ///< Alternative perspective for accessing futex etc.
    }
    void             *map;           ///< Full mapping- may change locations as data grows
    size_t            map_size;      ///< Current end of map. Usually >= filesize

    void             *data;          ///< Points into map just after header- used for read/write
    size_t            r;             ///< Current read cursor = offset into data (which may, change locations)
    size_t            w;             ///< Current write cursor, also offset into data
} gx_mfd;

gx_pool_init(gx_mfd);


/** Initialize an mfd struct for a given file, mapping the file for writes etc.
 * Pages-at-a-time should be large enough to avoid remapping a fast-growing
 * file constantly. Set to roughly throughput in bytes/per-second time 3 or 4
 * divided by page-size (usually 4096)- then bring it lower if too much virtual
 * memory is being sucked up (will help, at the expense of speed & processor
 * resources and slightly more churn earlier on).
 */
static int gx_mfd_create_w(gx_mfd *mfd, int pages_at_a_time, const char *path) {
    #ifdef __LINUX__
      int open_flags = O_RDWR | O_NONBLOCK | O_CREAT | O_APPEND | O_NOATIME | O_NOCTTY;
    #else
      int open_flags = O_RDWR | O_NONBLOCK | O_CREAT | O_APPEND;
    #endif

    mfd->type   = GX_MFD_WO;
    mfd->premap = pages_at_a_time;
    X(  mfd->fd=open(path, open_flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) X_RAISE(-1);
    X(  flock(mfd->fd, LOCK_EX | LOCK_NB)                              ) X_RAISE(-1);
    X(  _gx_initial_mapping(mfd)                                       ) X_RAISE(-1);

    // TODO: header stuff
    /*if(curr_size > 0) {
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
    }*/

    // TODO: set mfd->data, mfd->r, mfd->w
    return 0;
}

static GX_INLINE int _gx_initial_mapping(gx_mfd *mfd) {
    struct stat filestat;
    off_t  fsz;
    size_t map_size;
    #ifdef __LINUX__
      int  head_flags = MADV_RANDOM | MADV_WILLNEED | MADV_DONTFORK;
    #else
      int  head_flags = MADV_RANDOM | MADV_WILLNEED;
    #endif
      int  protection = PROT_READ;

    X(  fstat(mfd->fd, &filestat) ) X_RAISE(-1);
    mfd->real_fsize = fsz = filestat.st_size;
    mfd->map_size = gx_in_pages(fsz) + (gx_pagesize * mfd->premap);
    if(mfd->type == GX_MFD_WO) {
        protection |= PROT_WRITE;
        // Truncates just within last page to avoid sigbus but not to the very
        // end to avoid intermediate disk flushes (at least on the last page)
        // before it's really ready. Feel free to just ftruncate it to map_size
        // if this seems to be causing strangeness.
        ftruncate(mfd->fd, mfd->map_size - gx_pagesize + 2);
    }
    Xm( mfd->head_map=mmap(NULL,gx_pagesize,protection,MAP_SHARED,mfd->fd,0)) X_RAISE(-1);
    X ( madvise(mfd->head_map, gx_pagesize, head_flags)                     ) X_RAISE(-1);
    X ( mlock(mfd->head_map, gx_pagesize)                                   ) X_RAISE(-1);
    Xm( mfd->map=mmap(NULL,gx_in_pages(fsz),protection,MAP_SHARED,mfd->fd,0)) X_RAISE(-1);
    return _gx_advise_map(mfd);
}

static GX_INLINE int _gx_advise_map(gx_mfd *mfd) {
    // TODO: mark DONTNEEDs & SEQUENTIALs based on current cursor position
    return 0;
}

/*
 *  - Always keep at least a couple of blank pages in front so that a
 *    spontaneous write or read is guaranteed to be able to do at least
 *    page-size (and/or 4096 bytes).
 *  - Ask for average throughput when initializing to optimize "pages at a
 *    time" so it's not doing a mremap every 4096 bytes shovelled...
 *
 */

static int _gx_mfd_adjust_map(gx_mfd *mfd, size_t needed_size) {
    // TODO:
    //   (1) round up needed_size to nearest page-size -> to_map
    //   (2) remap to the size of to_map
    //   (3) (ensure that page 1 is locked into RAM?)
    //   (4) mark any pages > first but < page_of(mfd->c) as DONT_NEED or FREE
    //   (5) 
    return 0;
}

/// Returns a pointer to the "current" position after preparing for user to
/// read or write 'length' bytes of data.
static GX_INLINE void *mfd_c_adv(gx_mfd *mfd, size_t length) {

}


/// Address for current cursor
static GX_INLINE void *mfd_c(gx_mfd *mfd) {return mfd->dat + mfd->c;}

/// Advance the read/write cursor
static GX_INLINE int mfd_adv(gx_mfd *mfd, size_t len) {
    // TODO:
    //  - If @ mmap boundary:
    //    - If write-context, ftruncate (can be avoided w/o sigbus?) (OR lseek OR pwrite? to keep file sparse)
    //    - Remap
    //    - madvise on new & older pages
    //  - On Linux, trigger a FUTEX_WAKE
}




#endif

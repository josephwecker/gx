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


 *
 *
 *        freed/
 *       removed      active
 *          v           v
 *     =---------~~~~~~===#######
 *     ^           ^            ^
 *     mfd        marked       "file"-length
 *     head       dontneed     writer-mapped
 *                (per proc)   often reader-mapped.
 *
 * - Only writing process will free/remove pages under the following
 *   conditions:
 *    - Was previously marked dontneed by writer
 *    - Has been in that state for at least [something] seconds
 *    - Mincore shows that it is not in resident memory
 *    - MFD is not marked persistent
 * - Writer will indicate in the header the offset of the first page that is
 *   _not_ freed/removed. This is, in effect, the earliest offset besides the
 *   header page that a reader can operate on.
 * - A reader has the obligation to lock a page if it really needs to hold onto
 *   it for a while- only one reader should do this as locks don't stack.
 * - Initial mapping / remapping should never try to map pages that are marked
 *   free.
 *
 *
 *  MFD_DEPRICATE   = MADV_FREE |\ MADV_REMOVE
 *
 * - Header, then, needs to include
 *    - persistence
 *    - (start_freed is always the second page)
 *    - start_writer_done
 *    - start_writer_active
 *    - file_length
 *
 * - Each mfd instance will, of course, keep track of its own:
 *    - start_active (indicating that all pages before this are marked dontneed)
 *    - map_file_offset
 *    - mapped_pages
 *
 *
 * TODO:
 *  - Memory-map resizing as appropriate
 *  - Page advising on "used" pages
 *  - Add an easy "add" hook to gx_event for utilizing notify_fd
 *  - Abstraction for auto-update-files from writer perspective
 *    * Trigger for waking up waiting processes (FUTEX_WAKE on linux, nothing
 *      on mac-osx, which is going to be polling) - automatic where possible
 *      and exposed where not.
 *  - Be sure readonly stuff is set on readers
 *  - Make sure and mark variable as volatile (in the smallest appropriate
 *    scope ala wikipedia)
 *  - Programatically check and warn on bad system limitations
 *  -  W|\ |mfd_write(mfd*, void *src, len)
 *  -  W|\ |mfd_wbyte, wbe16/24/32/64, wse16/24/32/64, etc.
 *
 * RELEVANT SYSTEM LIMITATIONS:
 *  - vm.max_map_count  (65530)
 *  - vm.swappiness (60 - should be more ~ 20 or lower)
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
 *  - [D] Check for a correct header if there is any data
 *
 *  (WRITER only)
 *  - [D] Create a correct header if there is none
 *  - [D] Map the entire file + page_precache pages
 *  - [D] ftruncate or lseek+write to extend _just barely_ into the last mapped
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
 *  - [D] Map the entire file + page_precache pages - but all PROT_READ / RONLY,
 *    etc. to cause very efficient kernel optimizations. (possibly same w/
 *    memory-locked header page, but only if it doesn't interfere with the
 *    futex).
 *  - [D] Create a new pipe pair
 *  - [D] Do the cheapest sys_clone possible (vfork on mac, alas), and have one
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
#ifdef __LINUX__
  #ifndef _GNU_SOURCE
    #define _GNU_SOURCE
  #endif
  #include <unistd.h>
  #include <sys/syscall.h>
  #include <sys/types.h>
  #include <linux/futex.h>
  #include <sys/time.h>
#endif

#define _GX_MFD_ERR_IF  {X_LOG_ERROR("Not a properly formatted mfd file (misc data inside)."); X_RAISE(-1);}
#define _GX_MFD_FILESIG UINT64_C(0x1c1c1c1c1c1c1c1c)

#define GXMFDR 0
#define GXMFDW 1


static inline int _gx_futex(int *f, int op, int val) {
    #ifdef __LINUX__
      return syscall(SYS_futex, f, op, val, (void *)NULL, (int *)NULL, 0);
    #else
      return 0; // NOP
    #endif
}

static inline int gx_futex_wake(uint64_t *f) {
    #ifdef __LINUX__
      return _gx_futex((int *)f, FUTEX_WAKE, 0xFFFF);
    #else
      return 0; // NOP
    #endif
}

static inline int gx_futex_wait(void *f, int curr_val) {
    register int volatile * const p = (int volatile *)f;
    for(;;) {
        register int v = *p;
        if(v != curr_val) return v;
        #ifdef __LINUX__
            if(rare(_gx_futex(f, FUTEX_WAIT, v) < 0)) {
                int errn = errno;
                if(rare(errn != EAGAIN && errn != EINTR)) return -1;
                errno = 0;
            }
        #endif
        if(v != curr_val) return v;
        gx_sleep(0,2000); // Works for generic case and as a callback for linux
    }
}

/*
/// Mapped to the first part of the first page of the file- common offsets for mapcs.
typdef struct mapc_head {
    uint64_t file_sig;         ///< Will always be 0x1c... (so we don't clobber some unsuspecting file)
    uint64_t file_size;        ///< Full size of a file- including head(?)
    uint64_t start_available;  ///< File offset to data that hasn't been freed
    uint64_t start_active;     ///< File offset to data still mapped in writer
    uint64_t start_unused;     ///< Points to just after the last byte of actual data
} mapc_head;
*/

typedef struct gx_mfd_head {
    uint64_t  sig;    ///< Will always be 0x1c... - so we don't clobber some unsuspecting file not made for this
    uint64_t  size;   ///< Data size (not including header). Host-endian and not necessarily accurate- futex points here

    uint64_t  h1;     ///< User-defined - there should be a better way to do this...
    uint64_t  h2;     ///< User-defined
    uint64_t  h3;     ///< User-defined
    uint64_t  h4;     ///< User-defined
    uint64_t  h5;     ///< User-defined
    uint64_t  h6;     ///< User-defined
    uint64_t  h7;     ///< User-defined
} gx_mfd_head;

typedef struct gx_mfd {
    struct gx_mfd    *_next, *_prev; ///< For resource pooling
    int               type;          ///< Readonly or writeonly at the moment
    int               premap;        ///< Pages at a time to map- higher avoids more remapping+syscalls
    int               fd;            ///< File descriptor, ready for IO operations
    int               fdh;           ///< Reader temporarily opens a write fd so that the futex works
    size_t            off_eof;       ///< Offset to underlying file's EOF == currently truncated size
    union {
        void         *head_map;      ///< Same as first page of map, but locked into RAM
        gx_mfd_head  *head;          ///< Alternative perspective for accessing futex etc.
    };
    void             *map;           ///< Full mapping- may change locations as data grows
    size_t            off_eom;       ///< Offset to current end of map. Usually >= filesize

    void             *data;          ///< Points into map just after header- used for read/write
    size_t            off_r;         ///< Current read cursor = offset into data (which may change locations)
    size_t            off_w;         ///< Current write cursor, also offset into data

    int               n_in_fd;
    int               notify_fd;
} gx_mfd;

gx_pool_init(gx_mfd);

/// Some forward declarations
static optional int  gx_mfd_create_w    (gx_mfd *mfd, int pages_at_a_time, const char *path);
static optional int _gx_initial_mapping (gx_mfd *mfd);
static inline   int _gx_advise_map      (gx_mfd *mfd);
static inline   int _gx_update_eof      (gx_mfd *mfd);
static inline   int _gx_update_fpos     (gx_mfd *mfd);



/** Initialize an mfd struct for a given file, mapping the file for writes etc.
 * Pages-at-a-time should be large enough to avoid remapping a fast-growing
 * file constantly. Set to roughly throughput in bytes/per-second time 3 or 4
 * divided by page-size (usually 4096)- then bring it lower if too much virtual
 * memory is being sucked up (will help, at the expense of speed & processor
 * resources and slightly more churn earlier on).
 */
static optional int gx_mfd_create_w(gx_mfd *mfd, int pages_at_a_time, const char *path) {
    #ifdef __LINUX__
      int open_flags = O_RDWR | O_NONBLOCK | O_CREAT | O_APPEND | O_NOATIME | O_NOCTTY;
    #else
      int open_flags = O_RDWR | O_NONBLOCK | O_CREAT | O_APPEND;
    #endif
    size_t initial_size;

    mfd->type   = GXMFDW;
    mfd->premap = pages_at_a_time;
    X( mfd->fd=open(path, open_flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) {X_FATAL; X_RAISE(-1);}
    X( flock(mfd->fd, LOCK_EX | LOCK_NB)                              ) {X_FATAL; X_RAISE(-1);}
    X( initial_size = _gx_initial_mapping(mfd)                        ) {X_FATAL; X_RAISE(-1);}

    if(initial_size > 0) {
        // There should be a valid header in place then.
        if(mfd->head->sig != _GX_MFD_FILESIG)  _GX_MFD_ERR_IF;
        if(initial_size < sizeof(gx_mfd_head)) _GX_MFD_ERR_IF;
        if((uint64_t)(initial_size) != mfd->head->size)
            X_LOG_WARN("%s in a possibly inconsistent state.", path);
        mfd->head->size = initial_size - sizeof(gx_mfd_head);
        mfd->off_w      = initial_size - sizeof(gx_mfd_head);
    } else {
        mfd->head->sig  = _GX_MFD_FILESIG;
        mfd->head->size = 0;
        mfd->off_w      = 0;
    }
    mfd->data  = mfd->map + sizeof(gx_mfd_head);
    mfd->off_r = 0;
    _gx_update_fpos(mfd); // Seek to current write position for standard IO
    return 0;
}

static int _gx_mfd_readloop(void *vmfd) {
    gx_mfd *mfd = (gx_mfd *)vmfd;
    uint64_t size = mfd->head->size;
    int res;
    for(;;) {
        // TODO: remove X_WARN when appropriate...- got to keep this as
        // lightweight as possible to reduce stack size once it's been tested well.
        Xs(res = gx_futex_wait((void *)&(mfd->head->size), (int)size)) {
            case EFAULT: X_FATAL; return -1;
            default:     X_WARN;  return 0;
        }
        size = mfd->off_w = mfd->head->size;

        // TODO: completely inline this syscall so we can have an even smaller stack.
        X(write(mfd->n_in_fd, &size, sizeof(size))) X_RAISE(-1);
    }
    return 0;
}

static optional int gx_mfd_create_r(gx_mfd *mfd, int pages_at_a_time, const char *path) {
    int pipes[2];
    #ifdef __LINUX__
      int h_open_flags = O_RDWR   | O_NONBLOCK | O_NOATIME | O_NOCTTY;
      int open_flags   = O_RDONLY | O_NONBLOCK | O_NOATIME | O_NOCTTY;
    #else
      int h_open_flags = O_RDWR   | O_NONBLOCK;
      int open_flags   = O_RDONLY | O_NONBLOCK;
    #endif

    mfd->type   = GXMFDR;
    mfd->premap = pages_at_a_time;
    X( mfd->fd  = open(path, open_flags)    ) {X_FATAL; X_RAISE(-1);}
    X( mfd->fdh = open(path, h_open_flags)  ) {X_FATAL; X_RAISE(-1);}
    X( _gx_initial_mapping(mfd)             ) {X_FATAL; X_RAISE(-1);}
    X( close(mfd->fdh)                      )  X_WARN;
    #ifdef __LINUX__
      X( pipe2(pipes, O_NONBLOCK)           ) {X_FATAL; X_RAISE(-1);}
    #else
      X( pipe(pipes)                        ) {X_FATAL; X_RAISE(-1);}
      // TODO: probably fcntl to make it nonblocking
    #endif
    mfd->notify_fd = pipes[0];
    mfd->n_in_fd   = pipes[1];
    // TODO: possible race condition (kind of) - should possibly get the
    // current "size" stored seperately here to feed in and make sure that the
    // fd is added to some event loop _before_ activating the futex etc... As
    // it is currently, it may miss a change event as it's getting initialized-
    // but as a practical matter I don't think it'll mean much for the current
    // application...
    X(gx_clone(_gx_mfd_readloop,(void *)mfd)) {X_FATAL; X_RAISE(-1);}

    return 0;
}

/// Will be just like remapping but afaict this is easier for now
static int _gx_initial_mapping(gx_mfd *mfd) {
    struct stat filestat;
    off_t  fsz;
    int    protection = PROT_READ;
    int    head_flags = MADV_RANDOM | MADV_WILLNEED;

    X (fstat(mfd->fd, &filestat) ) X_RAISE(-1);
    mfd->off_eof = fsz = filestat.st_size;
    mfd->off_eom = gx_in_pages(fsz) + (pagesize() * mfd->premap);
    if(mfd->type == GXMFDW) {
        protection |= PROT_WRITE; // Otherwise will stay read-only for performance
        Xm(mfd->head_map=mmap(NULL,pagesize(),
                    protection,MAP_SHARED,mfd->fd,0)                       ) {X_FATAL; X_RAISE(-1);}
        X(_gx_update_eof(mfd)                                              ) {X_FATAL; X_RAISE(-1);}
    } else {
        Xm(mfd->head_map=mmap(NULL,pagesize(),
                    protection|PROT_WRITE,MAP_SHARED,mfd->fdh,0)           ) {X_FATAL; X_RAISE(-1);}
    }
    X (madvise(mfd->head_map, pagesize(), head_flags)                     ) {X_FATAL; X_RAISE(-1);}
    X (mlock(mfd->head_map, pagesize())                                   ) {X_FATAL; X_RAISE(-1);}
    Xm(mfd->map=mmap(NULL,gx_in_pages(fsz),protection,MAP_SHARED,mfd->fd,0)) {X_FATAL; X_RAISE(-1);}
    X (_gx_advise_map(mfd)                                                 ) {X_FATAL; X_RAISE(-1);}
    return fsz;
}

static inline int _gx_advise_map(gx_mfd *mfd) {
    // TODO: mark DONTNEEDs & SEQUENTIALs based on current cursor position
    return 0;
}

static inline int _gx_update_eof(gx_mfd *mfd) {
    // Truncates just within last page to avoid sigbus but not to the very
    // end to avoid intermediate disk flushes (at least on the last page)
    // before it's really ready. Feel free to just ftruncate it to off_eom
    // size if this seems to be causing strangeness.
    size_t new_pos = mfd->off_eom - pagesize() + 2;
    if(mfd->off_eof < new_pos) {
        X(ftruncate(mfd->fd, new_pos)) X_RAISE(-1);
        mfd->off_eof = new_pos;
    }
    return 0;
}

/// Gets the file position cursor (used for normal IO) synced up with the
/// position indicated by the mfd structure- when off_w or off_r has changed,
/// etc.
static inline int _gx_update_fpos(gx_mfd *mfd) {
    size_t used_offset = mfd->type == GXMFDW ? mfd->off_w : mfd->off_r;
    X(lseek(mfd->fd,used_offset+sizeof(gx_mfd_head),SEEK_SET)) X_RAISE(-1);
    return 0;
}

static inline void *mfd_w(gx_mfd *mfd) { return mfd->data + mfd->off_w; }

static inline int mfd_write(gx_mfd *mfd, const void *buf, size_t len) {
    Xn(memcpy(mfd_w(mfd), buf, len)) X_RAISE(-1);
    mfd->off_w += len;
    mfd->head->size = mfd->off_w;
    return gx_futex_wake(&(mfd->head->size));
}



#endif

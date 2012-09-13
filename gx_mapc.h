/**
 * Abstractions for IPC via memory-mapped pages (with and without file
 * persistence).
 *
 * Essentially for very fast essentially zero-copy bit-shoveling. A writer can
 * write to the structure either directly to memory or via standard I/O
 * functions and a reader in a potentially different process looking at the
 * same mapc will be immediately notified about new content (via a fd that can
 * be put in select/poll/epoll and friends) immediately- even before the data
 * syncs to disk and even if the data never goes to disk at all. The reader can
 * then use sendfile or other zero-copy mechanisms to very quickly copy the
 * data to something else if desired.
 *
 * Should be significantly faster than inotify / kqueue etc. for anything that
 * would be in the page-cache anyway like recently used files. At the same
 * time, it allows readers to fall behind the writer- effectively decoupling
 * them while still taking advantage of the speed of having all of the data
 * already in kernel-space.
 *
 * If an mapc is marked as not being persistent and it is observed that the
 * path is on a tmpfs / ram-disk based filesystem (like /dev/shm) mapc can go
 * a step further and completely free pages as soon as the writer and all known
 * readers are done with them. This effectively frees up the RAM or swap space
 * for all data that has been completely communicated/transfered.
 *
 * Implementation
 * ---------------
 *
 * A file is created, either a normal file that is meant to persist or
 * something on /dev/shm etc. It is not meant to be a general-purpose file- it
 * has a specific signature at the head used for futexes etc. during IPC.
 *
 *      removed      active
 *         v           v
 *    =---------~~~~~~===#### < end-of-file (truncated to this)
 *    ^           ^
 *    mfd        marked
 *    head       dontneed
 *               (per proc)
 *
 * TODO
 * -----
 *
 */
#ifndef GX_MMIPC_H
#define GX_MMIPC_H

#include <gx/gx.h>

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>


/// Mapped to the first part of the first page of the file- common offsets for mfds.

typdef struct gx_mapc_head {
    uint64_t file_sig;         ///< Will always be 0x1c... (so we don't clobber some unsuspecting file)
    uint64_t file_size;        ///< Full size of a file- including head(?)
    uint64_t start_available;  ///< File offset to data that hasn't been freed
    uint64_t start_active;     ///< File offset to data still mapped in writer
    uint64_t start_unused;     ///< Points to just after the last byte of actual data
} gx_mapc_head;


/// Holds the state for an mapc- including memory-mapped data pointers.

typedef gx_mapc {
    struct gx_mfd     *_next, *_prev; ///< For resource pooling
    int                type;          ///< Reader or writer
    int                is_volatile;   ///< Used to record whether or not the file is RAM-based

    union {
        void          *head_map;
        gx_mapc_head *head;
    };
    void              *map;



} gx_mapc;

gx_pool_init(gx_mapc);






static GX_OPTIONAL gx_mapc_open_w(gx_mapc *mm, const char *path, int persistent, ) {

}

#endif

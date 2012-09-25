/**
  Abstractions for IPC via memory-mapped pages (Similar to shared memory but
  with optional file-backed persistence).

  Essentially for very fast essentially zero-copy bit-shoveling. A writer can
  write to the structure either directly to memory or via standard I/O
  functions and a reader in a potentially different process looking at the
  same mapc will be immediately notified about new content (via a fd that can
  be put in select/poll/epoll and friends) immediately- even before the data
  syncs to disk and even if the data never goes to disk at all. The reader can
  then use sendfile or other zero-copy mechanisms to very quickly copy the
  data to something else if desired.

  Should be significantly faster than inotify / kqueue etc. for anything that
  would be in the page-cache anyway like recently used files. At the same
  time, it allows readers to fall behind the writer- effectively decoupling
  them while still taking advantage of the speed of having all of the data
  already in kernel-space.

  If an mapc is marked as not being persistent and it is observed that the
  path is on a tmpfs / ram-disk based filesystem (like /dev/shm) mapc can go
  a step further and completely free pages as soon as the writer and all known
  readers are done with them. This effectively frees up the RAM or swap space
  for all data that has been completely communicated/transfered.

  Implementation
  ---------------

  A file is created, either a normal file that is meant to persist or
  something on /dev/shm etc. It is not meant to be a general-purpose file- it
  has a specific signature at the head used for futexes etc. during IPC.

       removed      active
          v           v
     =---------~~~~~~===#### < end-of-file (truncated to this)
     ^           ^
     mapc        marked
     head       dontneed
                (per proc)


  Primary communication methods:
    - io using fd
    - io using memory buffer
    - memory poking + manual wakeup-calls



 | function                   | description                                                               |
 | -------------------------- | ------------------------------------------------------------------------- |
 | mapc *mapc_open(path,flgs) | Initialize & open reader or writer. Ret null on error.                    |

 | new_mapc_pool (SIZE)       | Allocate mapc pool. Ret mapc_pool or NULL on error                        |
 | acquire_mapc  (mapc_pool)  | Get mapc from pool. Ret actual mapc                                       |
 | mapc_getmem   (mapc*,len)  | Get a pointer where you can read or write len bytes directly to memory    |
 | mapc_autoseek (mapc*)      | Update fd's offset to c do before fd based read/write/etc. Ret -1=err     |
 | mapc_autotell (mapc*)      | Update c to fd's offset. Ret -1 on error                                  |
 | mapc_broadcast(mapc*,offs) | Signal any readers that care that data at offset offs is new.             |

 TODO
 -----

*/
#ifndef GX_MAPC_H
#define GX_MAPC_H

#include "./gx.h"
#include "gx_error.h"

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>


/// Holds the state for an mapc- including memory-mapped data pointers.
typedef struct mapc {
    struct mapc     *_next, *_prev; ///< For resource pooling
    char             type;           ///< Reader or writer
    char             is_volatile;    ///< Used to record whether or not the file is RAM-based

} packed mapc;

gx_pool_init(mapc);


static optional mapc_open(mapc *mm, const char *path, int flags) {

}

#endif

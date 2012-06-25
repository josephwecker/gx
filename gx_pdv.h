#ifndef GX_PDV_H
#define GX_PDV_H
//=============================================================================
/**
 * Persistent Data Vectors.
 * Especially for pure dataflow applications with backtracking and persistence.
 *
 *
 * Unreliable backfilled pdvs are special cases- possibly able to punt forever
 * on them and assume that they'll fill up / get solid in time that anything in
 * the rest of the sequence can treat them simply.
 *
 * (mremap implementation for mac osx that does munmap+mmap)
 *
 * check for lock, error if there is one
 * open and lock file (ensure 64-bit / largefile)
 * stat file for current size
 * mmap file, pointer to actual end.
 * mremap file when it runs out of room.
 * 
 * size_t ps = sysconf (_SC_PAGESIZE);
 * size_t ns = (at + size + ps - 1) & ~(ps - 1);
 *
 * 
 * b/a, d(b/a), f(b/a) :: int32 (except maybe f(sig) for some of the bigger original signals?)
 *      d(b/a) could stay integer even w/ half-diffs assuming a constant x2 factor...
 * 
 *
 *
 */

#include <gx/gx.h>
#include <gx/gx_error.h>

#define _GX_PATHSIZE 0x400

// Metainfo: stuff to be read from or written to 
typedef struct gx_pdv_meta {
    int          data_type;   ///< READ/WRITE: For reference only, really
    size_t       size;        ///< READ/WRITE: Size in data_types. i.e., number of values.
    size_t       last_valid;  ///< Pointer to latest valid slot for reading
} gx_pdv_meta;

typedef struct gx_pdv_map {
    int          persist_fd;
    char         persist_path[_GX_PATHSIZE];
    size_t       data_size;   ///< Filesize in bytes, always a multiple of system page size
    void        *mapped_file;
    gx_pdv_meta *meta_info;
    //void        *curr;        ///< Pointer to the next available slot for writing
} gx_pdv_map;

#endif

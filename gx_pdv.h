#ifndef GX_PDV_H
#define GX_PDV_H
//=============================================================================
/**
 * Persistent Data Vectors.
 * Especially for pure dataflow applications with backtracking and persistence.
 *
 *
 * 
 *
 *
 *
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
 */

#include <gx/gx.h>
#include <gx/gx_error.h>

#define _GX_PATHSIZE 0x400

typedef struct gx_pdv_map {
    int        persist_fd;
    char       persist_path[_GX_PATHSIZE];

    size_t     fsize;      ///< Actual filesize (via ftruncate etc.)
    size_t     allocated;  ///< (maybe same as fsize- file in actual pagesize chunks ok?)

    // Metainfo
    //int        data_type; // ?
    size_t     curr;        ///< Pointer to the next available slot
    size_t     last_valid;  ///< Maybe not the current one
    size_t     first_valid; 
} gx_pdv_map;


#endif

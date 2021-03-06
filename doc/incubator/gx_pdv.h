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
 *
 */

#include <gx/gx.h>
#include <gx/gx_error.h>
#include <gx/gx_mfd.h>

#define GX_PATHSIZE 0x400


// TODO:
//   - Simple writer-pdv definition/implementation
//   - Simple reader-pdv definition/implementation with callbacks
//
//





/*
// Metainfo: stuff to be read from or written to
typedef struct gx_pdv_meta {
    int          data_type;   ///< READ/WRITE: For reference only, really
    size_t       size;        ///< READ/WRITE: Size in data_types. i.e., number of values.
    size_t       last_valid;  ///< Pointer to latest valid slot for reading
} gx_pdv_meta;

typedef struct gx_pdv_map {
    int          persist_fd;
    char         persist_path[GX_PATHSIZE];
    size_t       data_size;   ///< Filesize in bytes, always a multiple of system page size
    void        *mapped_file;
    gx_pdv_meta *meta_info;
    //void        *curr;        ///< Pointer to the next available slot for writing
} gx_pdv_map;


char gx_pdv_base[GX_PATHSIZE];

typedef struct gx_port_meta {
    uint64_t      last_valid_n;
} gx_port_meta;

typedef struct gx_input_port {
    char          abs_path[GX_PATHSIZE];
    int           fd;
    size_t        last_n;
    gx_port_meta *meta;                   ///< Will point to the first part of the memory-mapped file.
} gx_input_port;

*/





#endif

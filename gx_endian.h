/**
 * @file      gx_endian.h
 * @brief     Detect endianness at runtime and various _portable_ macros for
 *            conversion.
 *
 * Thanks to stackoverflow:
 * http://stackoverflow.com/questions/1001307/detecting-endianness-programmatically-in-a-c-program
 *
 * @author    Joseph A Wecker
 * @copyright
 *   Except where otherwise noted, Copyright (C) 2012 Joseph A Wecker
 *
 *   MIT License
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *   copy of this software and associated documentation files (the "Software"),
 *   to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *   and/or sell copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef GX_ENDIAN_H
#define GX_ENDIAN_H
#include <gx/gx.h>

#define gx_econst64_def(NAME, B1,B2,B3,B4,B5,B6,B7,B8) \
  const uint64_t NAME ## __host=0x ## B1 ## B2 ## B3 ## B4 ## B5 ## B6 ## B7 ## B8;\
  const uint64_t NAME ## __tsoh=0x ## B8 ## B7 ## B6 ## B5 ## B4 ## B3 ## B2 ## B1

#define gx_econst64_use(NAME) \
  (gx_unlikely(gx_is_big_endian()) ? NAME ## __host : NAME ## __tsoh)



/// Apparently this can be determined at compile-time, so the functions below
/// that depend only on this conditional effectively have the dead branch
/// eliminated.
///
static GX_INLINE int gx_is_big_endian(void) {
    union {
        uint32_t i;
        char c[4];
    } check_int = {(uint32_t)0x01020304};
    return check_int.c[0] == 1; 
}


static GX_INLINE uint16_t big_endian16(uint16_t x) {
    if(gx_is_big_endian()) return x;
    return ((x & 0x00FFU) << 8) | ((x & 0xFF00U) >> 8);
}

static GX_INLINE uint32_t big_endian32(uint32_t x) {
    if(gx_is_big_endian()) return x;
    return ((x & 0x000000FFU) << 24) |
           ((x & 0x0000FF00U) <<  8) |
           ((x & 0x00FF0000U) >>  8) |
           ((x & 0xFF000000U) >> 24) ;
}

static GX_INLINE uint64_t big_endian64(uint64_t x) {
    if(gx_is_big_endian()) return x;
    return ((x & 0x00000000000000FFULL) << 56) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x00000000FF000000ULL) <<  8) |
           ((x & 0x000000FF00000000ULL) >>  8) |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0xFF00000000000000ULL) >> 56) ;
}
#endif

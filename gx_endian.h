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

#include "./gx.h"
#include <sys/param.h>

#if defined(BYTE_ORDER) && !defined(__BYTE_ORDER)
  #define __BYTE_ORDER  BYTE_ORDER
#endif
#if defined(BIG_ENDIAN) && !defined(__BIG_ENDIAN)
  #define __BIG_ENDIAN  BIG_ENDIAN
#endif
#if defined(LITTLE_ENDIAN) && !defined(__LITTLE_ENDIAN)
  #define __LITTLE_ENDIAN  LITTLE_ENDIAN
#endif

#ifndef __LITTLE_ENDIAN
  #define __LITTLE_ENDIAN  1234
#endif
#ifndef __BIG_ENDIAN
  #define __BIG_ENDIAN  4321
#endif

#ifndef __BYTE_ORDER
  #warning "Byte order not defined- assuming little endian"
  #define __BYTE_ORDER  __LITTLE_ENDIAN
#endif
#ifndef __FLOAT_WORD_ORDER
  #define __FLOAT_WORD_ORDER  __BYTE_ORDER
#endif
#if !defined(__BYTE_ORDER) || !defined(__FLOAT_WORD_ORDER)
  #error "Undefined byte or float word order"
#endif
#if __FLOAT_WORD_ORDER != __BIG_ENDIAN && __FLOAT_WORD_ORDER != __LITTLE_ENDIAN
  #error "Unknown/unsupported float word order"
#endif
#if __BYTE_ORDER != __BIG_ENDIAN && __BYTE_ORDER != __LITTLE_ENDIAN
  #error "Unknown/unsupported byte order"
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
  #define gx_bytes_BE 1
  #define gx_bytes_NE 1
  #define gx_bytes_LE 0
#else
  #define gx_bytes_BE 0
  #define gx_bytes_NE 0
  #define gx_bytes_LE 1
#endif

#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
  #define gx_floats_BE 1
  #define gx_floats_NE 1
  #define gx_floats_LE 0
#else
  #define gx_floats_BE 0
  #define gx_floats_NE 0
  #define gx_floats_LE 1
#endif


#define gx_econst64_def(NAME, B1,B2,B3,B4,B5,B6,B7,B8) \
  const uint64_t NAME ## __host=0x ## B1 ## B2 ## B3 ## B4 ## B5 ## B6 ## B7 ## B8;\
  const uint64_t NAME ## __tsoh=0x ## B8 ## B7 ## B6 ## B5 ## B4 ## B3 ## B2 ## B1

#define gx_econst64_use(NAME) \
  (rare(gx_is_big_endian()) ? NAME ## __host : NAME ## __tsoh)


/// TODO: Use as Fallbacks for when things are not determined above at
///       preprocessor time (?)
///
/// Apparently this can still be determined at compile-time, so the functions
/// below that depend only on this conditional effectively have the dead branch
/// eliminated.
///
static inline int gx_is_big_endian(void) {
    union {
        uint32_t i;
        char c[4];
    } check_int = {(uint32_t)0x01020304};
    return check_int.c[0] == 1;
}


static inline uint16_t big_endian16(uint16_t x) {
    if(gx_is_big_endian()) return x;
    return ((x & 0x00FFU) << 8) | ((x & 0xFF00U) >> 8);
}

static inline uint32_t big_endian32(uint32_t x) {
    if(gx_is_big_endian()) return x;
    return ((x & 0x000000FFU) << 24) |
           ((x & 0x0000FF00U) <<  8) |
           ((x & 0x00FF0000U) >>  8) |
           ((x & 0xFF000000U) >> 24) ;
}

static inline uint64_t big_endian64(uint64_t x) {
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

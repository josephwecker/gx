/**
 * @file      gx.h
 * @brief     Generic eXtra stuff (GX) primary header file- misc. macros.
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
 * @details
 *   # Conventions / Guidelines
 *   - As with all the gx* header files, optimized for linux, most optimized
 *     for x86_64, but should all work on osx. Should all work with gcc,
 *     llvm-backed-gcc, and clang.
 *   - While C/C++ programmers are used to not being allowed to start an
 *     identifier with an underscore, it is actually allowed for file-scope
 *     identifiers, which the ones below are.
 *   - Even more often developers don't like to use $ (or @) in C identifiers.
 *     For good reason- it's not very C-like. They aren't allowed or disallowed
 *     by standards, but some assemblers reject them in identifier names so
 *     they are almost globally frowned upon. GX uses them in some very
 *     strictly controled pieces (that you can change if you want) and only by
 *     the preprocessor - so that they don't exist by the time any code gets to
 *     the assember.
 *
 *
 *   Includes
 *   -----------------
 *   - The usual- stdio, stdlib, unistd, string, fcntl.
 *
 *   Typedefs
 *   ------------------
 *   For clarity, to indicate purpose or internal format, and for brevity.
 *
 *   - uint8_t  / U8  / byte   / uint8 / uint8_bitmask
 *   - uint16_t / U16 / uint16[_(be|le)]
 *   - int16_t  / S16 / sint16[_(be|le)]
 *   - (and also for 32 & 64 bit sizes. Can easily add 128-bit sizes [supported by gcc] if necessary)
 *   - uint24 (U24 etc.) (struct with three bytes- e.g., U24.b[2])
 *
 *   Constants
 *   -------------------
 *   (Ensures that the following are defined- sometimes missing from older preprocessors)
 *
 *   | __LINUX__          | set if host OS is linux |
 *   | __OSX__            | set if host OS is mach/apple/osx |
 *   | __SIZEOF_INT__     | in bytes |
 *   | __SIZEOF_POINTER__ | in bytes |
 *
 *
 *   Macros
 *   -------------------
 *   (may sometimes be implemented as macros, inline-functions, functions, or
 *   even expression attributes)
 *
 *   Naming convention is thus:
 *     - normal identifier for a few pre-compile operations like sizeofm etc.
 *       so they look like basic extensions to normal directives.
 *     - function-like macros have normal naming for now. I've alternated
 *       between namespace prefixes, underscore prefixes, and nothing. Going
 *       with nothing for now because for me at least these are the things that
 *       I don't want _other_ packages colliding with. These are the general
 *       purpose.
 *     - EXCEPT when they already would collide- like sleep(). in which case
 *       they get the gx_ prefix.
 *     - macros that are obviously preprocessor utilities are all upper-case
 *
 *   | sizeofm(TP,MEM)    | ctval| extension of sizeof for member field                    |
 *   | typeofm(TP,MEM)    | type | extension to typeof for member field                    |
 *   | build_bug_on(COND) | expr | in function scope, causes build failure if COND is true |
 *
 *   | min(a,b)           | expr | typesafe & multiple-side-effect-safe  |
 *   | min3(a,b,c)        | expr | typesafe & multiple-side-effect-safe  |
 *   | max(a,b)           | expr | typesafe & multiple-side-effect-safe  |
 *   | max3(a,b,c)        | expr | typesafe & multiple-side-effect-safe  |
 *   | cpu_ts()           | val  | very, very fast but somewhat innacurate cpu timestamp (nanosecond res.) |
 *   | bswap64(n)         | val  | very fast little <-> big-endian                  |
 *   | ntz(n)             | val  | very fast index of the bit set to 1              |
 *   | uint_to_vlq        | val  | variable-length-encode (or BER, etc.) an integer |
 *   | vlq_to_uint        | val  | decode a variable-length-encoded integer         |
 *
 *   | unlikely(expr)     | expr | compiler optimizes for expr to rarely succeed    |
 *   | rare(expr)         | expr | alias for _unlikely                              |
 *   | likely(expr)       | expr | compiler optimizes for expr to succeed often     |
 *   | freq(expr)         | expr | alias for _likely                                |
 *   | pagesize()         | expr | memoized current pagesize                                   |
 *   | gx_sleep(...)      | sfx  | gx_sleep(8,004,720,010) would sleep for 8.004720010 seconds |
 *
 *   | $(FMT,...)         | val  | quick sprintf, useful for function args, uses static buff |
 *   | $reset()           | sfx  | resets static buffer when existing $(...) strings aren't needed |
 *   | $s(BUF,FMT,...)    | val  | like $(), but specify your own static char buffer |
 *   | $sreset(BUF)       | sfx  | like $reset(), but specify a buffer               |
 *   |
 *
 *   | NARG(...)          | pp   | number of arguments given                       |
 *
 *
 *
 *   Attributes
 *   -------------------
 *
 *   | cold            | function               |
 *   | hot             | function               |
 *   | pure            | function               |
 *   | inline          | function               |
 *   | always_inline   | function               |
 *   | noinline        | function               |
 *   | optional        | function/global/static | (supresses "not-used" warnings) |
 *   | packed          | struct                 |
 *
 *
 *
 *
 * @todo  Normalize all the naming conventions
 * @todo  Update documentation / fix all for doxygen
 * @todo  Move resource pool to its own module
 * @todo  gx_prefetch_(rw_keep|rw_toss|ro_keep|ro_toss)
 *
 */
#ifndef GX_H
#define GX_H

/// Generic macro help

#define TO_STR(OTHER) _inner_def_as_string(OTHER)
#define _inner_def_as_string(OTHER) #OTHER

/// Normalize some OS names
#if defined(__linux__) || defined(__LINUX) || defined(__LINUX__) || defined(_LINUX)
  #define __LINUX__ 1
  #undef  __OSX__
#elif defined(__APPLE__) || defined(__MACH__) || defined(__OSX__) || defined(__MACH) || defined(_MACH)
  #define __OSX__ 1
  #undef  __LINUX__
#endif

#ifdef  __LINUX__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

/// Main system includes needed below
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h> // For pool mutexes / gx_clone
#include <sys/wait.h>
#ifdef __LINUX__
  #include <syscall.h>
#else
  #include <sys/syscall.h>
#endif

/// Typedefs
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
typedef uint8_t                    U8,    byte,     uint8,       uint8_bitmask;
typedef uint16_t                   U16,   uint16,   uint16_be,   uint16_le;
typedef int16_t                    S16,   sint16,   sint16_be,   sint16_le;
typedef uint32_t                   U32,   uint32,   uint32_be,   uint32_le;
typedef int32_t                    S32,   sint32,   sint32_be,   sint32_le;
typedef uint64_t                   U64,   uint64,   uint64_be,   uint64_le;
typedef int64_t                    S64,   sint64,   sint64_be,   sint64_le;
typedef struct __uint24{U8 b[3];}  U24,   uint24,   uint24_be,   uint24_le;
typedef float                      F32,   float32,  float32_be,  float32_le;
typedef double                     F64,   float64,  float64_be,  float64_le;
typedef long double                F96,   float96,  float96_be,  float96_le;


/// @todo Generalize min/max to use variadic input
#define min(A,B)     ({ typeof(A) _a = (A); typeof(B) _b = (B); _b < _a ? _b : _a; })
#define min3(A,B,C)  min(min(A,B),C)

#define max(A,B)     ({ typeof(A) _a = (A); typeof(B) _b = (B); _b > _a ? _b : _a; })
#define max3(A,B,C)  max(max(A,B),C)


/// Compiler-specific intrinsics and fixes: bswap64, ntz, rdtsc, ...

#if __INTEL_COMPILER
  #define cpu_ts ((unsigned)__rdtsc())
#elif (__GNUC__ && (__x86_64__ || __amd64__ || __i386__))
  #define cpu_ts ({unsigned res; __asm__ __volatile__ ("rdtsc" : "=a"(res) : : "edx"); res;})
#elif (_M_IX86)
  #include <intrin.h>
  #pragma intrinsic(__rdtsc)
  #define cpu_ts ((unsigned)__rdtsc())
#else
  #warning "Don't know how to implement RDTSC intrinsic on your system."
#endif

#if __GNUC__ > 3
  #define    freq(E)       __builtin_expect(!!(E), 1)
  #define    rare(E)       __builtin_expect(!!(E), 0)
  #define    optional      __attribute__ ((__unused__))
  #define    noinline      __attribute__ ((__noinline__))
  #define    packed        __attribute__ ((__packed__))
  #ifndef    DEBUG
    #define  pure          __attribute__ ((__pure__))
    #define  cold          __attribute__ ((__cold__))
    #define  hot           __attribute__ ((__hot__))
    #define  always_inline inline __attribute__ ((__always_inline__))
  #else
    #define  inline
    #define  pure
    #define  cold
    #define  hot
    #define  always_inline
  #endif
#else
  #define    freq(E)       (E)
  #define    rare(E)       (E)
  #define    cold
  #define    hot
  #define    pure
  #define    inline        __inline__
  #define    always_inline __inline__
#endif



#ifndef __SIZEOF_INT__
  #ifdef __INTMAX_MAX__
    #if (__INT_MAX__ == 0x7fffffff) || (__INT_MAX__ == 0xffffffff)
      #define __SIZEOF_INT__ 4
    #else
      // Warning: Guessing the size of an integer is 2 bytes
      #define __SIZEOF_INT__ 2
    #endif
  #elif defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
    #define __SIZEOF_INT__ 4
  #else
    // Warning: Guessing the size of an integer is 2 bytes
    #define __SIZEOF_INT__ 2
  #endif
#endif

#ifndef __SIZEOF_POINTER__
  // Warning Guessing the size of a pointer is 2 * __SIZEOF_INT__
  #define __SIZEOF_POINTER__ (2 * __SIZEOF_INT__)
#endif

#define _STR2(x)      #x
#define _STR(x)       _STR2(x)
#define __LINE_STR__  _STR(__LINE__)

#define sizeofm(type, member) sizeof(((type *)0)->member)
#define typeofm(type, member) typeof(((type *)0)->member)


/// A trick to get the number of args passed to a variadic macro
/// @author Laurent Deniau <laurent.deniau@cern.ch> (I think)
/// @author Joseph Wecker (fixed zero-va_args bug)
#define NARG(...)  _NARG_(DUMMY, ##__VA_ARGS__,_RSEQ_N())
#define _NARG_(...) _ARG_N(__VA_ARGS__)
#define _ARG_N( \
          _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define _RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0


#define build_bug_on(condition) ((void)sizeof(char[1 - 2*!!(condition)]))


/// @todo generalize these a little better and perhaps combine w/ gx_log's kv stuff in its own module
// Quick and dirty check to make sure that keys are only integers in a range
// reserved for key-value pair keys.
#define M_K(KEY) ({ build_bug_on(sizeof(KEY) != sizeof(int) || KEY > 512); KEY; })

#define KV(...) NARG(__VA_ARGS__) KV2(NARG(__VA_ARGS__), ##__VA_ARGS__)
#define KV2(N,...) KV3(N, ##__VA_ARGS__)
#define KV3(N,...) KV_ ## N (__VA_ARGS__)
#define   KV_0()
#define   KV_1(K1)         ,ERROR___key_without_a_value
#define   KV_2(K1,V1)      ,M_K(K1),V1
#define   KV_3(K1,V1,...)  KV_2(K1,V1) KV_1(__VA_ARGS__)
#define   KV_4(K1,V1,...)  KV_2(K1,V1) KV_2(__VA_ARGS__)
#define   KV_5(K1,V1,...)  KV_2(K1,V1) KV_3(__VA_ARGS__)
#define   KV_6(K1,V1,...)  KV_2(K1,V1) KV_4(__VA_ARGS__)
#define   KV_7(K1,V1,...)  KV_2(K1,V1) KV_5(__VA_ARGS__)
#define   KV_8(K1,V1,...)  KV_2(K1,V1) KV_6(__VA_ARGS__)
#define   KV_9(K1,V1,...)  KV_2(K1,V1) KV_7(__VA_ARGS__)
#define  KV_10(K1,V1,...)  KV_2(K1,V1) KV_8(__VA_ARGS__)
#define  KV_11(K1,V1,...)  KV_2(K1,V1) KV_9(__VA_ARGS__)
#define  KV_12(K1,V1,...)  KV_2(K1,V1)KV_10(__VA_ARGS__)
#define  KV_13(K1,V1,...)  KV_2(K1,V1)KV_11(__VA_ARGS__)
#define  KV_14(K1,V1,...)  KV_2(K1,V1)KV_12(__VA_ARGS__)
#define  KV_15(K1,V1,...)  KV_2(K1,V1)KV_13(__VA_ARGS__)
#define  KV_16(K1,V1,...)  KV_2(K1,V1)KV_14(__VA_ARGS__)
#define  KV_17(K1,V1,...)  KV_2(K1,V1)KV_15(__VA_ARGS__)
#define  KV_18(K1,V1,...)  KV_2(K1,V1)KV_16(__VA_ARGS__)
#define  KV_19(K1,V1,...)  KV_2(K1,V1)KV_17(__VA_ARGS__)
#define  KV_20(K1,V1,...)  KV_2(K1,V1)KV_18(__VA_ARGS__)
#define  KV_21(K1,V1,...)  KV_2(K1,V1)KV_19(__VA_ARGS__)
#define  KV_22(K1,V1,...)  KV_2(K1,V1)KV_20(__VA_ARGS__)
#define  KV_23(K1,V1,...)  KV_2(K1,V1)KV_21(__VA_ARGS__)
#define  KV_24(K1,V1,...)  KV_2(K1,V1)KV_22(__VA_ARGS__)
#define  KV_25(K1,V1,...)  KV_2(K1,V1)KV_23(__VA_ARGS__)
#define  KV_26(K1,V1,...)  KV_2(K1,V1)KV_24(__VA_ARGS__)
#define  KV_27(K1,V1,...)  KV_2(K1,V1)KV_25(__VA_ARGS__)
#define  KV_28(K1,V1,...)  KV_2(K1,V1)KV_26(__VA_ARGS__)
#define  KV_29(K1,V1,...)  KV_2(K1,V1)KV_27(__VA_ARGS__)
#define  KV_30(K1,V1,...)  KV_2(K1,V1)KV_28(__VA_ARGS__)
#define  KV_31(K1,V1,...)  KV_2(K1,V1)KV_29(__VA_ARGS__)
#define  KV_32(K1,V1,...)  KV_2(K1,V1)KV_30(__VA_ARGS__)
#define  KV_33(K1,V1,...)  KV_2(K1,V1)KV_31(__VA_ARGS__)
#define  KV_34(K1,V1,...)  KV_2(K1,V1)KV_32(__VA_ARGS__)
#define  KV_35(K1,V1,...)  KV_2(K1,V1)KV_33(__VA_ARGS__)
#define  KV_36(K1,V1,...)  KV_2(K1,V1)KV_34(__VA_ARGS__)
#define  KV_37(K1,V1,...)  KV_2(K1,V1)KV_35(__VA_ARGS__)
#define  KV_38(K1,V1,...)  KV_2(K1,V1)KV_36(__VA_ARGS__)
#define  KV_39(K1,V1,...)  KV_2(K1,V1)KV_37(__VA_ARGS__)
#define  KV_40(K1,V1,...)  KV_2(K1,V1)KV_38(__VA_ARGS__)
#define  KV_41(K1,V1,...)  KV_2(K1,V1)KV_39(__VA_ARGS__)
#define  KV_42(K1,V1,...)  KV_2(K1,V1)KV_40(__VA_ARGS__)
#define  KV_43(K1,V1,...)  KV_2(K1,V1)KV_41(__VA_ARGS__)
#define  KV_44(K1,V1,...)  KV_2(K1,V1)KV_42(__VA_ARGS__)
#define  KV_45(K1,V1,...)  KV_2(K1,V1)KV_43(__VA_ARGS__)
#define  KV_46(K1,V1,...)  KV_2(K1,V1)KV_44(__VA_ARGS__)
#define  KV_47(K1,V1,...)  KV_2(K1,V1)KV_45(__VA_ARGS__)
#define  KV_48(K1,V1,...)  KV_2(K1,V1)KV_46(__VA_ARGS__)
#define  KV_49(K1,V1,...)  KV_2(K1,V1)KV_47(__VA_ARGS__)
#define  KV_50(K1,V1,...)  KV_2(K1,V1)KV_48(__VA_ARGS__)
#define  KV_51(K1,V1,...)  KV_2(K1,V1)KV_49(__VA_ARGS__)
#define  KV_52(K1,V1,...)  KV_2(K1,V1)KV_50(__VA_ARGS__)
#define  KV_53(K1,V1,...)  KV_2(K1,V1)KV_51(__VA_ARGS__)
#define  KV_54(K1,V1,...)  KV_2(K1,V1)KV_52(__VA_ARGS__)
#define  KV_55(K1,V1,...)  KV_2(K1,V1)KV_53(__VA_ARGS__)
#define  KV_56(K1,V1,...)  KV_2(K1,V1)KV_54(__VA_ARGS__)
#define  KV_57(K1,V1,...)  KV_2(K1,V1)KV_55(__VA_ARGS__)
#define  KV_58(K1,V1,...)  KV_2(K1,V1)KV_56(__VA_ARGS__)


static optional inline int uint_to_vlq(uint64_t x, uint8_t *out) {
    int i, j, count=0;
    for (i = 9; i > 0; i--) { if (x & 127ULL << i * 7) break; }
    for (j = 0; j <= i; j++) {
        out[j] = ((x >> ((i - j) * 7)) & 127) | 128;
        ++count;
    }
    out[i] ^= 128;
    return ++count;
}

static optional inline uint64_t vlq_to_uint(uint8_t *in) {
    uint64_t r = 0;
    do r = (r << 7) | (uint64_t)(*in & 127);
    while (*in++ & 128);
    return r;
}


/// @todo refactor, check, rename, and restyle (ugh)
/*=============================================================================
* MISC MATH
* gx_pos_ceil(x)                - Integer ceiling (for positive values)
* gx_fits_in(container_size, x) - How many containers needed?
*---------------------------------------------------------------------------*/
#define gx_pos_ceil(x) (((x)-(int)(x)) > 0 ? (int)((x)+1) : (int)(x))
#define gx_fits_in(container_size, x) gx_pos_ceil((float)(x) / (float)(container_size))
#define gx_in_pages(SIZE) (((SIZE) & ~(pagesize() - 1)) + pagesize())

/*=============================================================================
* OS PARAMETERS
* pagesize()                   - Gets OS kernel page size
* Yeah, needs to coordinate somewhat w/ a config file or something...
*---------------------------------------------------------------------------*/
static int _GXPS optional = 0;

/// @todo don't use HAS_SYSCONF - use __LINUX__ etc. instead
#if defined(HAS_SYSCONF) && defined(_SC_PAGE_SIZE)
#define pagesize() ({ if(!_GXPS) _GXPS=sysconf(_SC_PAGE_SIZE); _GXPS; })
#elif defined (HAS_SYSCONF) && defined(_SC_PAGESIZE)
#define pagesize() ({ if(!_GXPS) _GXPS=sysconf(_SC_PAGESIZE);  _GXPS; })
#else
#define pagesize() ({ if(!_GXPS) _GXPS=getpagesize();          _GXPS; })
#endif



/**
 * gx_base64_urlencode_m3()
 *
 * Encodes inp into outp, optimized for inputs that are multiples of 3 in
 * length. Be sure that outp is allocated to be at least GX_BASE64_SIZE(sizeof(input_data));
 *
 * The "normal" base-64 encoding lookup is done on this string:
 *
 *      static const optional char _gx_t64[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
 *
 * Currently I'm using a modified version that is in incrementing ascii value
 * for easier sorting when it represents some numeric value. So it's good for
 * all my current purposes, but it would fail to be decoded currently by
 * something expecting true base64-url-encoding.
 */
static const optional char _gx_t64[]= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-";
#define GX_BASE64_SIZE(DATSIZE) (4 * (DATSIZE) / 3 + 1)
static optional inline ssize_t gx_base64_urlencode_m3(const void *indata, size_t insize, char *outdata) {
    const char *inp  = (const char *)indata;
    char       *outp = outdata;
    if(rare(insize % 3 != 0)) {errno = EINVAL; return -1;}
    while(inp < (const char *)(indata + insize)) {
        outp[0] = _gx_t64[((inp[0] & 0xFC) >> 2) ];
        outp[1] = _gx_t64[((inp[0] & 0x03) << 4) | ((inp[1] & 0xF0) >> 4)];
        outp[2] = _gx_t64[((inp[1] & 0x0F) << 2) | ((inp[2] & 0xC0) >> 6)];
        outp[3] = _gx_t64[ (inp[2] & 0x3F)       ];
        inp    += 3;
        outp   += 4;
    }
    outp[0] = '\0';
    return outp + 1 - outdata;
}


/// bswap64 - most useful for big to little-endian
/// @todo (builtin bswap32?)
#if __GNUC__
    #define bswap64(x) __builtin_bswap64(x)           /* Assuming GCC 4.3+ */
    #define bswap32(x) __builtin_bswap32(x)
    #define ntz(x)     __builtin_ctz((unsigned)(x))   /* Assuming GCC 3.4+ */
#else              /* Assume some C99 features: stdint.h, inline, restrict */
    #define bswap32(x)                                              \
       ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) | \
        (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))

     static always_inline uint64_t bswap64(uint64_t x) {
        union { uint64_t u64; uint32_t u32[2]; } in, out;
        in.u64 = x;
        out.u32[0] = bswap32(in.u32[1]);
        out.u32[1] = bswap32(in.u32[0]);
        return out.u64;
    }

    #if (L_TABLE_SZ <= 9) && (L_TABLE_SZ_IS_ENOUGH)   /* < 2^13 byte texts */
    static always_inline unsigned ntz(unsigned x) {
        static const unsigned char tz_table[] = {0,
        2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,6,2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,7,
        2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,6,2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,8,
        2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,6,2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,7,
        2,3,2,4,2,3,2,5,2,3,2,4,2,3,2,6,2,3,2,4,2,3,2,5,2,3,2,4,2,3,2};
        return tz_table[x/4];
    }
    #else       /* From http://supertech.csail.mit.edu/papers/debruijn.pdf */
    static always_inline unsigned ntz(unsigned x) {
        static const unsigned char tz_table[32] =
        { 0,  1, 28,  2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17,  4, 8,
         31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18,  6, 11,  5, 10, 9};
        return tz_table[((uint32_t)((x & -x) * 0x077CB531u)) >> 27];
    }
    #endif
#endif

#include "./gx_error.h"

#define gx_sleep(...)           _GX_SLEEP(NARG(__VA_ARGS__), ##__VA_ARGS__)
#define _GX_SLEEP(N,...)        _GX_SLEEP_(N, ##__VA_ARGS__)
#define _GX_SLEEP_(N,...)       _GX_SLEEP_ ## N (__VA_ARGS__)
#define _GX_SLEEP_0()           _gx_sleep(1,0)
#define _GX_SLEEP_1(S)          _gx_sleep(S,0)
#define _GX_SLEEP_2(S,MS)       _gx_sleep(S, 1 ## MS ## 000000    - 1000000000)
#define _GX_SLEEP_3(S,MS,US)    _gx_sleep(S, 1 ## MS ## US ## 000 - 1000000000)
#define _GX_SLEEP_4(S,MS,US,NS) _gx_sleep(S, 1 ## MS ## US ## NS  - 1000000000)
static optional inline int _gx_sleep(time_t seconds, long nanoseconds) {
    int s;
    struct timespec ts;
    ts.tv_sec  = seconds;
    ts.tv_nsec = nanoseconds;
    for(;;) {
        if((s = nanosleep(&ts, &ts)) == -1) switch(errno) {
            case EINTR: break;
            default:    return(-1);
        }
        if(!s) break;
    }
    return 0;
}


#endif

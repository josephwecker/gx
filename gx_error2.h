/**
   @file      gx_error.h
   @brief     Lots of error-handling/logging to help unobscure C code.
   @author    Joseph A Wecker <joseph.wecker@gmail.com>
   @copyright
     Except where otherwise noted, Copyright (C) 2012 Joseph A Wecker

     MIT License

     Permission is hereby granted, free of charge, to any person obtaining a
     copy of this software and associated documentation files (the "Software"),
     to deal in the Software without restriction, including without limitation
     the rights to use, copy, modify, merge, publish, distribute, sublicense,
     and/or sell copies of the Software, and to permit persons to whom the
     Software is furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in
     all copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
     THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
     DEALINGS IN THE SOFTWARE.


   @details

    | etype | description                                                               |
    | ----- | ------------------------------------------------------------------------- |
    | esys  | Error if expression is -1         errno set                               |
    | emap  | Error if expression is MAP_FAILED errno set                               |
    | enz   | Error if expression is non-zero   value as errno                          |
    | ez    | Error if expression is zero       errno set                               |
    | enull | Error if expression is null       errno possibly set                      |
    | eio   | (NYI) stdio ferror style errors (doesn't trigger on feof)                 |
    | eavc  | (NYI) avconv-style errors- < 0 is error value, some w/ mapping to errno   |
*/
#ifndef GX_ERROR2_H
#define GX_ERROR2_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


esys = {strerror, };

// Overrides

/* TODO:
 *  h_errno
    gai_strerror,
    herror,
    hstrerror,
*/



#define if_esys(E)      if( _esys(E)  )
#define if_emap(E)      if( _emap(E)  )
#define if_enz(E)       if( _enz(E)   )
#define if_ez(E)        if( _ez(E)    )
#define if_enull(E)     if( _enull(E) )

#define switch_esys(E)  if_esys(E)  switch(errno)
#define switch_emap(E)  if_emap(E)  switch(errno)
#define switch_enz(E)   if_enz(E)   switch(errno)
#define switch_ez(E)    if_ez(E)    switch(errno)
#define switch_enull(E) if_enull(E) switch(errno)

#define _(E)            if_esys(E)
#define _M(E)           if_emap(E)
#define _E(E)           if_enz(E)
#define _Z(E)           if_ez(E)
#define _N(E)           if_enull(E)

// TODO: try sys_nerr + 10 or something for E_GX_RAISED
#define E_GX_RAISED 254
#define _eraise(RETVAL) {\
    _gx_error_cidx += 1;                               \
    if(_gx_error_cidx > GX_ERROR_BACKTRACE_SIZE - 1) { \
        _gx_error_cidx = GX_ERROR_BACKTRACE_SIZE - 1;  \
    }\
    errno = E_GX_RAISED; \
    return RETVAL;       \
}

#define _eclear() {\
    _gx_error_cidx = 0;                  \
    _gx_error_stack[0].error_id =        \
        _gx_error_stack[1].error_id = 0; \
    errno = 0;                           \
}

#define _eexit() {\
    /* TODO: log fatal */ gx_error_dump_all();\
    exit(errno);\
}


// ---------------------------------------------------------------------
// TODO: move all contained to gx.h when ready to make this include gx.h

#ifndef __SIZEOF_INT__
  #ifdef __INTMAX_MAX__
    #if (__INT_MAX__ == 0x7fffffff) || (__INT_MAX__ == 0xffffffff)
      #define __SIZEOF_INT__ 4
    #else
      //#warning Guessing the size of an integer is 2 bytes
      #define __SIZEOF_INT__ 2
    #endif
  #elif defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
    #define __SIZEOF_INT__ 4
  #else
    //#warning Guessing the size of an integer is 2 bytes
    #define __SIZEOF_INT__ 2
  #endif
#endif

#ifndef __SIZEOF_POINTER__
  //#warning Guessing the size of a pointer is 2 * __SIZEOF_INT__
  #define __SIZEOF_POINTER__ (2 * __SIZEOF_INT__)
#endif

#define _STR2(x)      #x
#define _STR(x)       _STR2(x)
#define __LINE_STR__  _STR(__LINE__)
#define _rare(X)      __builtin_expect(!!(X), 0)
#define _freq(X)      __builtin_expect(!!(X), 1)
#define _inline       __attribute__ ((__always_inline__))
#define _noinline     __attribute__ ((__noinline__))
/// A trick to get the number of args passed to a variadic macro
/// @author Laurent Deniau <laurent.deniau@cern.ch> (I think)
/// @todo   Abstract this back into gx.h and rename so it's generally usable
#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0
// ---------------------------------------------------------------------


#define _gx_mrk(EXPR_STR) \
    _gx_mark_err_do((errno ? errno : ( _e ? _e : EINVAL )), \
           __FILE__, __LINE__, __FUNCTION__, EXPR_STR)

#define _reset_e() errno=0; int _e=0; _gx_error_depth++
#define _run_e(E)  ({ int _result = _rare(E); _gx_error_depth--; \
        if(!_gx_error_depth && !_result) _eclear(); _result; })

#define _esys(E)   ({ _reset_e(); (_run_e(    (int)(E) == -1         )) ? _gx_mrk(#E) : 0; })
#define _emap(E)   ({ _reset_e(); (_run_e(    (int)(E) == MAP_FAILED )) ? _gx_mrk(#E) : 0; })
#define _enz(E)    ({ _reset_e(); (_run_e((_e=(int)(E))!= 0          )) ? _gx_mrk(#E) : 0; })
#define _ez(E)     ({ _reset_e(); (_run_e(         (E) == 0          )) ? _gx_mrk(#E) : 0; })
#define _enull(E)  ({ _reset_e(); (_run_e( (void *)(E) ==(void *)NULL)) ? _gx_mrk(#E) : 0; })
// TODO: _eio  - stdio ferror style errors (distinguish feof)
// TODO: _eavc - libavconv/ffmpeg style error codes


#define GX_ERROR_BACKTRACE_SIZE 5

typedef struct gx_error_rpt {
    int         error_id;
    const char *src_file;
    int         src_line;
    const char *src_func;
    const char *src_expr;
    int         chk_level;
} __attribute__((__packed__)) gx_error_rpt;
#define GX_ERROR_REPORT_SIZE ((__SIZEOF_INT__ * 3) + (__SIZEOF_POINTER__ * 3))
// TODO:
//   - error-class for lookup tables



// Guarantee these variables are put in the .common section so they are
// globally shared by the linker without us needing to initialize them
// anywhere.
asm ("\n .comm _gx_error_stack,"  _STR(GX_ERROR_REPORT_SIZE * GX_ERROR_BACKTRACE_SIZE) ",8 \n");
asm ("\n .comm _gx_error_cidx,"   _STR(__SIZEOF_INT__) ",8 \n");
asm ("\n .comm _gx_error_depth,"  _STR(__SIZEOF_INT__) ",8 \n");

gx_error_rpt _gx_error_stack[GX_ERROR_BACKTRACE_SIZE];
int          _gx_error_cidx;
int          _gx_error_depth;

// Purposefully don't inline because it will be rarely called (or, in any case,
// the code path for it not being called should be the optimized code path) and
// we don't want it to mess with the instruction pipeline / processing cache,
// even though the body is pretty minimal currently...
static _noinline int _gx_mark_err_do(int error_id, const char *file, int line,
        const char *function, const char *expr)
{
    _gx_error_stack[_gx_error_cidx].error_id  = error_id;
    _gx_error_stack[_gx_error_cidx].src_file  = file;
    _gx_error_stack[_gx_error_cidx].src_line  = line;
    _gx_error_stack[_gx_error_cidx].src_func  = function;
    _gx_error_stack[_gx_error_cidx].src_expr  = expr;
    _gx_error_stack[_gx_error_cidx].chk_level = _gx_error_depth;

    // If you ever want to change this so that it possibly filters out certain
    // errors you can return 0 and the earlier macros will think that no error
    // actually occurred.
    return 1;
}



/**
 * Log any errors. (should be automatic??)
 * SEVERITY
 * - 
 *
 */

//  #define _elog(SEVERITY, ...) _gx_elog(#SEVERITY, ##  ....
//  static _noinline void _gx_elog(const char *severity,  ....


static void gx_error_dump_all()
{
    int i;
    fprintf(stderr,"\n\n---------------- ERROR-DUMP --------------------\n");
    for(i = 0; i < GX_ERROR_BACKTRACE_SIZE; i++) {
        if(_gx_error_stack[i].error_id) {
            fprintf(stderr, "\nEntry %d:\n" "-------------------\n"
                            "  errno:     %d\n" "  src_file:  %s\n"
                            "  src_line:  %d\n" "  src_func:  %s\n"
                            "  src_expr:  %s\n" "  chk_level: %d\n",
                    i, _gx_error_stack[i].error_id,
                    _gx_error_stack[i].src_file, _gx_error_stack[i].src_line,
                    _gx_error_stack[i].src_func, _gx_error_stack[i].src_expr,
                    _gx_error_stack[i].chk_level);
        } else break;
    }
}


//
// - name
// - short_abbrev
// - check_statement
typedef struct gx_error_class {


} gx_error_class;

#endif

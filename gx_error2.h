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
*/

#ifndef GX_ERROR_H
#define GX_ERROR_H

/** 
 * Error Families.
 *
 *  | etype | description                                                               |
 *  | ----- | ------------------------------------------------------------------------- |
 *  | esys  | Error if expression is -1         errno set                               |
 *  | emap  | Error if expression is MAP_FAILED errno set                               |
 *  | enz   | Error if expression is non-zero   value as errno                          |
 *  | ez    | Error if expression is zero       errno set                               |
 *  | enull | Error if expression is null       errno possibly set                      |
 *  | eio   | (NYI) stdio ferror style errors (doesn't trigger on feof)                 |
 *  | eavc  | (NYI) avconv-style errors- < 0 is error value, some w/ mapping to errno   |
 */


#define if_esys(E)  if( _esys(E)  )
#define if_emap(E)  if( _emap(E)  )
#define if_enz(E)   if( _enz(E)   )
#define if_ez(E)    if( _ez(E)    )
#define if_enull(E) if( _enull(E) )

#define switch_esys(E)  if_esys(E)  switch(errno)
#define switch_emap(E)  if_emap(E)  switch(errno)
#define switch_enz(E)   if_enz(E)   switch(errno)
#define switch_ez(E)    if_ez(E)    switch(errno)
#define switch_enull(E) if_enull(E) switch(errno)

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


/// Stringifies the current line of code
/// @TODO: move to gx.h
#define _STR2(x)      #x
#define _STR(x)       _STR2(x)
#define __LINE_STR__  _STR(__LINE__)
#define _rare(X)      __builtin_expect(!!(X), 0)
#define _freq(X)      __builtin_expect(!!(X), 1)
#define _inline       __attribute__ ((__always_inline__))
#define _noinline     __attribute__ ((__noinline__))

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
typedef struct gx_error_report {
    int         error_id;
    const char *src_file;
    int         src_line;
    const char *src_func;
    const char *src_expr;
    int         chk_level;
} __attribute__((__packed__)) gx_error_report;
#define GX_ERROR_REPORT_SIZE ((__SIZEOF_INT__ * 3) + (__SIZEOF_POINTER__ * 3))

// Guarantee these variables are put in the .common section so they are
// globally shared by the linker without us needing to initialize them
// anywhere.
__asm__ ("\n .comm _gx_error_stack,"  _STR(GX_ERROR_REPORT_SIZE * GX_ERROR_BACKTRACE_SIZE) ",16 \n");
__asm__ ("\n .comm _gx_error_cidx,"   _STR(__SIZEOF_INT__) ",16 \n");
__asm__ ("\n .comm _gx_error_depth,"  _STR(__SIZEOF_INT__) ",16 \n");
gx_error_report _gx_error_stack[GX_ERROR_BACKTRACE_SIZE] = {0};
int             _gx_error_cidx                           = 0;
int             _gx_error_depth                          = 0;

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

#endif

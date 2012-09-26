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

    | check | abbrv | errtype | description                                                               |
    | ----- | ----- | ------- | ------------------------------------------------------------------------- |
    | esys  | _()   | syserr  | Error if expression is -1         errno set                               |
    | emap  | _M()  | syserr  | Error if expression is MAP_FAILED errno set                               |
    | enz   | _E()  | syserr  | Error if expression is non-zero   value as errno                          |
    | ez    | _Z()  | syserr  | Error if expression is zero       errno set                               |
    | enull | _N()  | syserr  | Error if expression is null       errno possibly set                      |
    | eio   |       | syserr  | (NYI) stdio ferror style errors (doesn't trigger on feof)                 |
    | eavc  |       | syserr  | (NYI) avconv-style errors- < 0 is error value, some w/ mapping to errno   |


    Actions
      - _ignore()        : ignore. possibly logs to unessential severity
      - _raise (RV)      : returns RV after saving the error on the error-stack
      - _clear ()        : clears the error-stack- does not log. happens automatically with other handlers
      - _abort  ()        : exits with error-number's value- automatically logs reason / severity
      - E_LOG   (SEV,...) : explicitly logs the error with a given severity & additional information
        The following are shorthand for logging the error with specific severities.
          - _emergency(...)
          - _alert    (...)
          - _critical (...)
          - _error    (...)
          - _warning  (...)
          - _notice   (...)
          - _info     (...)
          - _stat     (...)
          - _debug    (...)
          - E_UNKNOWN  (...)


   @todo
      - Separate esys lookup module with additional lookups from:
        - h_errno
        - gai_strerror,
        - herror,
        - hstrerror,
      - _eio  - stdio ferror style errors (distinguish feof)
      - _eavc - libavconv/ffmpeg style error codes
      - Silently abort (very early on) if runtime log-level is > severity
      - Append stack trace report when severity is high enough
*/

#ifndef GX_ERROR_H
#define GX_ERROR_H

#include "./gx_log.h"
#include "./gxe/gx_enum_lookups.h"
#include <sys/mman.h>

static optional void gx_error_dump_all();

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

#define gx_error_initialize(TAG)

#define _raise(RETVAL) {                                         \
    _gx_error_cidx += 1;                                         \
    if(_gx_error_cidx > GX_ERROR_BACKTRACE_SIZE - 1) {           \
        _gx_error_cidx = GX_ERROR_BACKTRACE_SIZE - 1;            \
    }                                                            \
    return RETVAL;                                               \
}

#define _ignore() {                                              \
    log_debug(K_msg, "Ignored");                                 \
    _clear();                                                    \
}

#define _clear() {                                               \
    _gx_error_cidx = 0;                                          \
    _gx_error_stack[0].error_number =                            \
        _gx_error_stack[1].error_number = 0;                     \
    errno = 0;                                                   \
}

#define _abort() {                                               \
    _critical();                                                \
    exit(errno);                                                 \
}

#define _goto(LBL) goto LBL

#define E_LOG(SEV, ...)  _gx_elog(SEV, #SEV, KV(__VA_ARGS__))
#define _emergency(...) E_LOG(SEV_EMERGENCY, ##__VA_ARGS__)
#define _alert(...)     E_LOG(SEV_ALERT,     ##__VA_ARGS__)
#define _critical(...)  E_LOG(SEV_CRITICAL,  ##__VA_ARGS__)
#define _error(...)     E_LOG(SEV_ERROR,     ##__VA_ARGS__)
#define _warning(...)   E_LOG(SEV_WARNING,   ##__VA_ARGS__)
#define _notice(...)    E_LOG(SEV_NOTICE,    ##__VA_ARGS__)
#define _info(...)      E_LOG(SEV_INFO,      ##__VA_ARGS__)
#define _stat(...)      E_LOG(SEV_STAT,      ##__VA_ARGS__)
#define _debug(...)     E_LOG(SEV_DEBUG,     ##__VA_ARGS__)
#define _uknown(...)    E_LOG(SEV_UNKNOWN,   ##__VA_ARGS__)


#define _gx_mrk(EXPR_STR)                                        \
    _gx_mark_err_do((errno ? errno : ( _e ? _e : EINVAL )),      \
           __FILE__, __LINE__, __FUNCTION__, EXPR_STR)
#define _reset_e() errno=0; int _e=0; _gx_error_depth++
#define _run_e(E)  ({ int _result = rare(E); _gx_error_depth--; \
        if(!_gx_error_depth && !_result) _clear(); _result; })

#define _esys(E)   ({ _reset_e(); (_run_e(    (int)(E) == -1         )) ? _gx_mrk(#E) : 0; })
#define _emap(E)   ({ _reset_e(); (_run_e(    (int)(E) == MAP_FAILED )) ? _gx_mrk(#E) : 0; })
#define _enz(E)    ({ _reset_e(); (_run_e((_e=(int)(E))!= 0          )) ? _gx_mrk(#E) : 0; })
#define _ez(E)     ({ _reset_e(); (_run_e(         (E) == 0          )) ? _gx_mrk(#E) : 0; })
#define _enull(E)  ({ _reset_e(); (_run_e( (void *)(E) ==(void *)NULL)) ? _gx_mrk(#E) : 0; })

typedef struct gx_error_lookup_info {
    char               error_label     [25];
    char               error_msg      [256];
    gx_severity        error_severity;
} gx_error_lookup_info;

// These are generated by the makefile
#include "./gxe/syserr.h"

typedef enum gx_error_family {
    ERRF_SYSERR
} gx_error_family;


#define GX_ERROR_BACKTRACE_SIZE 5

typedef struct gx_error_rpt {
    int         error_number;
    int         error_family;
    const char *src_file;
    int         src_line;
    const char *src_func;
    const char *src_expr;
    int         chk_level;
} __attribute__((__packed__)) gx_error_rpt;
#define GX_ERROR_REPORT_SIZE ((__SIZEOF_INT__ * 4) + (__SIZEOF_POINTER__ * 3))

// Guarantee these variables are put in the .common section so they are
// globally shared by the linker without us needing to initialize them
// anywhere.
//
/// @todo !!! abstract into macro & put in gx.h
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
static noinline int _gx_mark_err_do(int error_number, const char *file, int line,
        const char *function, const char *expr)
{
    _gx_error_stack[_gx_error_cidx].error_family = ERRF_SYSERR; /// Hardcoded for now until there are more
    _gx_error_stack[_gx_error_cidx].error_number = error_number;
    _gx_error_stack[_gx_error_cidx].src_file     = file;
    _gx_error_stack[_gx_error_cidx].src_line     = line;
    _gx_error_stack[_gx_error_cidx].src_func     = function;
    _gx_error_stack[_gx_error_cidx].src_expr     = expr;
    _gx_error_stack[_gx_error_cidx].chk_level    = _gx_error_depth;

    // If you ever want to change this so that it possibly filters out certain
    // errors you can return 0 and the earlier macros will think that no error
    // actually occurred.
    return 1;
}

/// Core Error expansion / logging
/// @note _gx_log call does $reset so be careful trying to use any values after that (although it'll still probably work...)

#define _EXPAND(IDX)                                                                                \
    K_type,            "syserr",                                                                    \
    K_name,            $("SYSERR_%s", syserr_info[_gx_error_stack[IDX].error_number].error_label),  \
    K_src_file,        _gx_error_stack[IDX].src_file,                                               \
    K_src_line,        $("%u", _gx_error_stack[IDX].src_line),                                      \
    K_src_function,    _gx_error_stack[IDX].src_func,                                               \
    K_src_expression,  _gx_error_stack[IDX].src_expr,                                               \
    K_err_severity,    $gx_severity(syserr_info[_gx_error_stack[IDX].error_number].error_severity), \
    K_err_family,      $gx_error_family(ERRF_SYSERR),                                               \
    K_err_number,      $("%u", _gx_error_stack[IDX].error_number),                                  \
    K_err_label,       syserr_info[_gx_error_stack[IDX].error_number].error_label,                  \
    K_err_msg,         syserr_info[_gx_error_stack[IDX].error_number].error_msg

static noinline void _gx_elog(gx_severity sev, char *ssev, int argc, ...)
{
    int     i;
    va_list argv;
    if(argc > 0) va_start(argv, argc);
    if(rare(_gx_error_stack[1].error_number)) {
        // Several errors to report, all "linked" to the last one
        //char *egrp = $("%u", cpu_ts);
        static char egroup[64];
        char *egrp = _gx_cpu_ts_str(egrp, cpu_ts);
        for(i=0; i < GX_ERROR_BACKTRACE_SIZE; i++) {
            if(_gx_error_stack[i].error_number) {
                char *edpth = $("%u", _gx_error_stack[i].chk_level);
                if(argc > 0) _gx_log(sev, ssev, argc, &argv, _EXPAND(i), K_err_depth, edpth, K_err_group, egrp);
                else         _gx_log(sev, ssev, 0,    NULL,  _EXPAND(i), K_err_depth, edpth, K_err_group, egrp);
            } else break;
        }
    } else if(freq(_gx_error_stack[0].error_number)) {
        if(argc > 0) _gx_log(sev, ssev, argc, &argv, _EXPAND(0));
        else         _gx_log(sev, ssev, 0,    NULL,  _EXPAND(0));
    } else {
        /// @todo do something clever here- there are no errors to report
    }
    if(argc > 0) {
        va_end(argv);
        $reset(); // Reset temporary string buffer for adhoc sprintfs ($(...))
    }
}
#undef _EXPAND


/// Primarily a debugging tool, orthogonal to the normal errors/logging. You can probably ignore it.
static optional void gx_error_dump_all()
{
    int i;
    fprintf(stderr,"\n\n---------------- ERROR-DUMP --------------------\n");
    for(i = 0; i < GX_ERROR_BACKTRACE_SIZE; i++) {
        if(_gx_error_stack[i].error_number) {
            fprintf(stderr, "\nEntry %d:\n" "-------------------\n"
                            "  errno:     %d\n" "  src_file:  %s\n"
                            "  src_line:  %d\n" "  src_func:  %s\n"
                            "  src_expr:  %s\n" "  chk_level: %d\n",
                    i, _gx_error_stack[i].error_number,
                    _gx_error_stack[i].src_file, _gx_error_stack[i].src_line,
                    _gx_error_stack[i].src_func, _gx_error_stack[i].src_expr,
                    _gx_error_stack[i].chk_level);
        } else break;
    }
}

#endif

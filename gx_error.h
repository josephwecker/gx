/**
 * @file      gx_error.h
 * @brief     Lots of error-handling/logging to help unobscure C code.
 *
 * Somewhere in global scope, set the following macro:
 *     gx_error_initialize(default_loglevel);
 * For example:
 *     gx_error_initialize(GX_DEBUG);
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
 *
 *
 * TODO:
 *  [ ] gx_elog allowed to accept another qualifier (similar to how perror is
 *      usually used)
 *  [ ] Configuration or maybe even runtime log-level detection.
 *  [ ] Configuration or env flag for colors
 *  [ ] Only emit colorcodes when stderr is going to a terminal
 *  [ ] Separate color terminal printing macros into gx_tty or something
 *  [ ] Send to pipe so that messages are atomic in multi-worker situations
 *  [ ] Macro wraps up macro specific stuff quickly and uses a function for
 *      the rest, reducing size and increasing speed due to instruction
 *      cache.
 *  [ ] Look up and display the error names instead of numbers (ala linux
 *      programming book)
 *  [ ] E_RAISE should propagate (possibly) some current-location information
 *      - link together so a problem can be dug into deeper.
 *  [ ] Some quick macros: D(...) and GOT_HERE for quick spot debugging
 *  [ ] Add __function__ to the data collected
 *  [ ] Indicate verbosity of the error - per message or runtime setting or
 *      compile setting.
 *  [ ] Ability to hook in other logging mechanisms
 *  [ ] (somehow) Better message alignment
 *  [ ] Contexts, ala libav/ffmpeg
 *  [ ] (possibly not necessary) - call stack info
 *  [ ] Fix info color- more green
 *
 *   compile-time-reporting-level
 *   runtime-reporting-level
 *   logging-mechanism/file
 *
 *    __FUNCTION__
 *    __LINE__
 *    __FILE__
 *    expression
 *    errorno
 *      actual-constant-name
 *      os-defined-message
 *    user error-message
 *
 *    severity
 *
 *    actions
 *      logging
 *      control-flow
 *
 *      (ideally have these as a block instead of the current weird syntax)
 *
 *
 *  - GX_ERROR_INITIALIZE(); - global macro for initializing global variables
 *    etc. so they can be declared extern in the header.
 *  - E_RAISE action should put the log message that would have been printed
 *    onto a special stack.
 *  
 */
#ifndef GX_ERROR_H
#define GX_ERROR_H

#include <gx/gx.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h> /* TODO: pipe logger for atomic msgs to stdout */


/// Place this somewhere in global scope
#define gx_error_initialize(default_loglevel)               \
    gx_error_report gx_error_recent_reports[3];             \
    int             gx_error_curr_report = 0;               \
    int             gx_error_loglevel    = default_loglevel;

/// For quick debugging "code reached here" points
#define XDBG X_LOG_DEBUG(NULL, "-- XDBG position reached --")

/// For system errors- detects -1 and uses errno
#define cX(expr, ctx) if( ({errno=0; (gx_unlikely((int)(expr) == -1)) ?        \
    ({_GX_X(ctx,errno,0,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))

/// Checks for null values
#define cXn(expr, ctx) if( ({errno=0; (gx_unlikely((void *)(expr) == NULL)) ?  \
    ({_GX_X(ctx,errno,1,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))

/// Check for mmap errors
#define cXm(expr, ctx) if( ({errno=0; (gx_unlikely((expr) == MAP_FAILED)) ?    \
    ({_GX_X(ctx,errno,2,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))

/// Checks for system errors, but drops you into a switch statement
#define cXs(expr, ctx) if( ({errno=0; (gx_unlikely((int)(expr) == -1)) ?       \
    ({_GX_X(ctx,errno,0,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;})) \
    switch(gx_error_recent_reports[gx_error_curr_report].sys_errno)

/// For system errors- detects -1 and uses errno
#define X(expr) if( ({errno=0; (gx_unlikely((int)(expr) == -1)) ?        \
    ({_GX_X(NULL,errno,0,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))

/// Checks for null values
#define Xn(expr) if( ({errno=0; (gx_unlikely((void *)(expr) == NULL)) ?  \
    ({_GX_X(NULL,errno,1,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))

/// Check for mmap errors
#define Xm(expr) if( ({errno=0; (gx_unlikely((expr) == MAP_FAILED)) ?    \
    ({_GX_X(NULL,errno,2,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))

/// For functions that return zero on success, and errno on error
#define Xz(expr) if( ({int _gx_tmp_errno=0; errno=0; (gx_unlikely((_gx_tmp_errno = (int)(expr)) != 0)) ? \
    ({errno=_gx_tmp_errno; _GX_X(NULL,_gx_tmp_errno,2,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;}))


/// Checks for system errors, but drops you into a switch statement
#define Xs(expr) if( ({errno=0; (gx_unlikely((int)(expr) == -1)) ?       \
    ({_GX_X(NULL,errno,0,NULL,__FILE__,__STR(__LINE__),__FUNCTION__,#expr);1;}):0;})) \
    switch(gx_error_recent_reports[gx_error_curr_report].sys_errno)

#define X_LOG_PANIC(msg, ...) \
    ({_GX_X(NULL,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_PANIC;1;})
#define X_LOG_FATAL(msg, ...) \
    ({_GX_X(NULL,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_FATAL;1;})
#define X_LOG_ERROR(msg, ...) \
    ({_GX_X(NULL,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_ERROR;1;})
#define X_LOG_WARN( msg, ...)  \
    ({_GX_X(NULL,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_WARN; 1;})
#define X_LOG_INFO( msg, ...)  \
    ({_GX_X(NULL,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_INFO; 1;})
#define X_LOG_DEBUG(msg, ...) \
    ({_GX_X(NULL,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_DEBUG;1;})

#define cX_LOG_PANIC(ctx, msg, ...) \
    ({_GX_X(ctx,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_PANIC;1;})
#define cX_LOG_FATAL(ctx, msg, ...) \
    ({_GX_X(ctx,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_FATAL;1;})
#define cX_LOG_ERROR(ctx, msg, ...) \
    ({_GX_X(ctx,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_ERROR;1;})
#define cX_LOG_WARN(ctx, msg, ...)  \
    ({_GX_X(ctx,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_WARN; 1;})
#define cX_LOG_INFO(ctx, msg, ...)  \
    ({_GX_X(ctx,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_INFO; 1;})
#define cX_LOG_DEBUG(ctx, msg, ...) \
    ({_GX_X(ctx,0,3,msg,__FILE__,__STR(__LINE__),__FUNCTION__,NULL,##__VA_ARGS__);X_DEBUG;1;})

#define X_PANIC _GX_ERROR_LOG(GX_PANIC) ///< Log last captured error at 'panic' severity
#define X_FATAL _GX_ERROR_LOG(GX_FATAL) ///< Log last captured error at 'fatal' severity
#define X_ERROR _GX_ERROR_LOG(GX_ERROR) ///< Log last captured error at 'erro' severity
#define X_WARN  _GX_ERROR_LOG(GX_WARN ) ///< Log last captured error at 'warn' severity
#define X_INFO  _GX_ERROR_LOG(GX_INFO ) ///< Log last captured error at 'info' severity
#define X_DEBUG _GX_ERROR_LOG(GX_DEBUG) ///< Log last captured error at 'debug' severity

#define X_RAISE(...) {                                     \
    gx_error_curr_report = (gx_error_curr_report + 1) % 3; \
    return __VA_ARGS__;                                    \
}

#define X_EXIT exit(errno)

//-----------------------------------------------------------------------------
 
#define GX_PANIC         0
#define GX_FATAL         1
#define GX_ERROR         2
#define GX_WARN          3
#define GX_INFO          4
#define GX_DEBUG         5

typedef struct gx_error_report {
    void *context;
    int   sys_errno;
    int   gx_errno;
    char  custom_msg[1024];
    char  filename[256];
    const char* linenum;
    char  function[256];
    char  expression[256];
} gx_error_report;

extern gx_error_report gx_error_recent_reports[3];
extern int             gx_error_curr_report;
extern int             gx_error_loglevel;

// Generate these with build-ename.sh
#if defined(__DARWIN__)
    static char *_gx_ename[] = {
        /*   0 */ "", 
        /*   1 */ "EPERM  ", "ENOENT ", "ESRCH  ", "EINTR  ", "EIO    ", "ENXIO  ", 
        /*   7 */ "E2BIG  ", "ENOEXEC", "EBADF  ", "ECHILD ", "EDEADLK", 
        /*  12 */ "ENOMEM ", "EACCES ", "EFAULT ", "ENOTBLK", "EBUSY  ", 
        /*  17 */ "EEXIST ", "EXDEV  ", "ENODEV ", "ENOTDIR", "EISDIR ", 
        /*  22 */ "EINVAL ", "ENFILE ", "EMFILE ", "ENOTTY ", "ETXTBSY", 
        /*  27 */ "EFBIG  ", "ENOSPC ", "ESPIPE ", "EROFS  ", "EMLINK ", "EPIPE  ", 
        /*  33 */ "EDOM   ", "ERANGE ", "EAGAIN ", "EINPROGRESS", 
        /*  37 */ "EALREADY", "ENOTSOCK", "EDESTADDRREQ", "EMSGSIZE", 
        /*  41 */ "EPROTOTYPE", "ENOPROTOOPT", "EPROTONOSUPPORT", 
        /*  44 */ "ESOCKTNOSUPPORT", "ENOTSUP", "EPFNOSUPPORT", 
        /*  47 */ "EAFNOSUPPORT", "EADDRINUSE", "EADDRNOTAVAIL", 
        /*  50 */ "ENETDOWN", "ENETUNREACH", "ENETRESET", "ECONNABORTED", 
        /*  54 */ "ECONNRESET", "ENOBUFS", "EISCONN", "ENOTCONN", 
        /*  58 */ "ESHUTDOWN", "ETOOMANYREFS", "ETIMEDOUT", "ECONNREFUSED", 
        /*  62 */ "ELOOP  ", "ENAMETOOLONG", "EHOSTDOWN", "EHOSTUNREACH", 
        /*  66 */ "ENOTEMPTY", "EPROCLIM", "EUSERS ", "EDQUOT ", "ESTALE ", 
        /*  71 */ "EREMOTE", "EBADRPC", "ERPCMISMATCH", "EPROGUNAVAIL", 
        /*  75 */ "EPROGMISMATCH", "EPROCUNAVAIL", "ENOLCK ", "ENOSYS ", 
        /*  79 */ "EFTYPE ", "EAUTH  ", "ENEEDAUTH", "EPWROFF", "EDEVERR", 
        /*  84 */ "EOVERFLOW", "EBADEXEC", "EBADARCH", "ESHLIBVERS", 
        /*  88 */ "EBADMACHO", "ECANCELED", "EIDRM  ", "ENOMSG ", "EILSEQ ", 
        /*  93 */ "ENOATTR", "EBADMSG", "EMULTIHOP", "ENODATA", "ENOLINK", 
        /*  98 */ "ENOSR  ", "ENOSTR ", "EPROTO ", "ETIME  ", "EOPNOTSUPP", 
        /* 103 */ "ELAST  ", "ENOPOLICY"
    };

  #define MAX_ENAME 103
#elif defined(__LINUX__)
    static char *_gx_ename[] = {
        /*   0 */ "", 
        /*   1 */ "EPERM  ",   "ENOENT ", "ESRCH  ", "EINTR  ", "EIO    ", "ENXIO  ", 
        /*   7 */ "E2BIG  ",   "ENOEXEC", "EBADF  ", "ECHILD ", 
        /*  11 */ "EAGAIN ",   "ENOMEM ", "EACCES ", "EFAULT ", 
        /*  15 */ "ENOTBLK",   "EBUSY  ", "EEXIST ", "EXDEV  ", "ENODEV ", 
        /*  20 */ "ENOTDIR",   "EISDIR ", "EINVAL ", "ENFILE ", "EMFILE ", 
        /*  25 */ "ENOTTY ",   "ETXTBSY", "EFBIG  ", "ENOSPC ", "ESPIPE ", 
        /*  30 */ "EROFS  ",   "EMLINK ", "EPIPE  ", "EDOM   ", "ERANGE ", 
        /*  35 */ "EDEADLK",   "ENAMETOOLONG",       "ENOLCK ", "ENOSYS ", 
        /*  39 */ "ENOTEMPTY", "ELOOP  ", "",        "ENOMSG ", "EIDRM  ", "ECHRNG ", 
        /*  45 */ "EL2NSYNC",  "EL3HLT ", "EL3RST ", "ELNRNG ", "EUNATCH", 
        /*  50 */ "ENOCSI ",   "EL2HLT ", "EBADE  ", "EBADR  ", "EXFULL ", "ENOANO ", 
        /*  56 */ "EBADRQC",   "EBADSLT", "", "EBFONT", "ENOSTR", "ENODATA", 
        /*  62 */ "ETIME  ",   "ENOSR  ", "ENONET ", "ENOPKG ", "EREMOTE", 
        /*  67 */ "ENOLINK",   "EADV   ", "ESRMNT ", "ECOMM  ", "EPROTO ", 
        /*  72 */ "EMULTIHOP", "EDOTDOT", "EBADMSG", "EOVERFLOW", 
        /*  76 */ "ENOTUNIQ",  "EBADFD ", "EREMCHG", "ELIBACC", "ELIBBAD", 
        /*  81 */ "ELIBSCN",   "ELIBMAX", "ELIBEXEC","EILSEQ ", "ERESTART", 
        /*  86 */ "ESTRPIPE",  "EUSERS ", "ENOTSOCK", "EDESTADDRREQ", 
        /*  90 */ "EMSGSIZE",  "EPROTOTYPE", "ENOPROTOOPT", 
        /*  93 */ "EPROTONOSUPPORT", "ESOCKTNOSUPPORT", 
        /*  95 */ "ENOTSUP",   "EPFNOSUPPORT", "EAFNOSUPPORT", 
        /*  98 */ "EADDRINUSE","EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH", 
        /* 102 */ "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS", 
        /* 106 */ "EISCONN",   "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS", 
        /* 110 */ "ETIMEDOUT", "ECONNREFUSED", "EHOSTDOWN", "EHOSTUNREACH", 
        /* 114 */ "EALREADY",  "EINPROGRESS", "ESTALE ", "EUCLEAN", 
        /* 118 */ "ENOTNAM",   "ENAVAIL", "EISNAM ", "EREMOTEIO", "EDQUOT ", 
        /* 123 */ "ENOMEDIUM", "EMEDIUMTYPE", "ECANCELED", "ENOKEY ", 
        /* 127 */ "EKEYEXPIRED","EKEYREVOKED", "EKEYREJECTED", 
        /* 130 */ "EOWNERDEAD","ENOTRECOVERABLE", "ERFKILL"
    };

  #define MAX_ENAME 132
#endif

/* Some predefined color escape sequences */
#define C_N               "\e[0m"
#define C_PANIC           "\e[38;5;198m"
#define C_FATAL           "\e[38;5;196m"
#define C_ERROR           "\e[38;5;160m"
#define C_WARN            "\e[38;5;202m"
#define C_INFO            "\e[38;5;118m"
#define C_DEBUG           "\e[38;5;226m"
#define C_UNKN            "\e[38;5;27m"
#define C_DELIM           "\e[38;5;236m"
#define C_BG              "\e[48;5;234m"
#define C_REF             "\e[38;5;240m"

#define _START   C_BG C_DELIM "["
#define _DLM(ch) C_BG C_DELIM ch C_REF   // delimiter shorthand for internal use
#define _FINISH  C_DELIM "]" C_N " "

//wrapper around stringifying a macro arg to avoid affecting the function
#define __STR2(x) #x
#define __STR(x) __STR2(x)

//a trick to get the number of args passed to a variadic macro
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

static char *_gx_eloglvl[] = {
    "panic",
    "fatal",
    "error",
    "warn",
    "info",
    "debug"
};

static char *_gx_errmsgs[] = {
    "System error.",
    "NULL where value is expected.",
    "Memory-mapping operation failed.",
    "--"
};

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static void _GX_X(
        void       *context,
        int         sys_errno,
        int         gx_errno,
        const char *custom_msg,
        const char *filename,
        const char *linenum,
        const char *function,
        const char *expression, ...) {
    va_list ap;
    va_start(ap, expression);
    gx_error_report *rp = &gx_error_recent_reports[gx_error_curr_report];
    memset(rp,0,sizeof(gx_error_report));
    rp->context   = context;
    rp->sys_errno = sys_errno;
    rp->gx_errno  = gx_errno;
    rp->linenum   = linenum;
    if(filename)   strncpy(rp->filename,   filename,   255);    else rp->filename[0]  =0;
    if(custom_msg) vsnprintf(rp->custom_msg,1023,custom_msg,ap);else rp->custom_msg[0]=0;
    if(function)   strncpy(rp->function,   function,   255);    else rp->function[0]  =0;
    if(expression) strncpy(rp->expression, expression, 255);    else rp->expression[0]=0;
    va_end(ap);
}

static GX_INLINE void _gx_get_basename(const char *fn, char *basename) {
    memset(basename, 0, 20);
    char *last = strrchr(fn, '/');
    if(last == NULL) strncpy(basename, fn, 19);
    else strncpy(basename, last+1, 19);
}

static GX_INLINE void _gx_get_expr_part(char *expr, char *exprpart) {
    memset(exprpart, 0, 20);
    char *start_at = strrchr(expr, '=');
    if(start_at == NULL) start_at = expr;
    else start_at ++;
    while(start_at[0] == ' ' || start_at[0] == '\t') start_at ++;
    char *fin_at = strchr(start_at, '(');
    if(fin_at == NULL) strncpy(exprpart, start_at, 19);
    else {
        int len = MIN(19, fin_at - start_at);
        strncpy(exprpart, start_at, len);
    }
}


#define LOG_PIPES 0
#define LOG_JSON  1
#define LOG_STDOUT 1
#define LOG_STDERR 2
#ifndef LOG_FORMAT 
    #define LOG_FORMAT LOG_PIPES
#endif

typedef struct logger {
    int handle;
    uint8_t format;
    const char *color;
} logger;

typedef struct log_format {
    char *begin, *kvformat, *delimiter, *end;
} log_format;

typedef struct kv {
    struct kv *_next, *_prev, *next;
    const char *key, *value;
} kv;

typedef struct context {
    struct context *_next, *_prev;
    kv* keyvalues;
} context;

// >> log(ctx, level, msg("hello %s", "jeff"), "key", "value")
// >> inner_log(ctx, level, 
//      PP_NARG("key", "value") + 6, 
//      msg("hello %s", "jeff"), "key", "value", "line", line, "function", function, "file", file)
// >> inner_log(ctx, level, 
//      PP_NARG("key", "value") + 6, 
//      "hello %s", "jeff", "key", "value", "line", line, "function", function, "file", file)
// >> inner_log(ctx, level, 
//      8, 
//      "hello %s", "jeff", "key", "value", "line", line, "function", function, "file", file)


#define LOG(ctx, level, message, ...)                                        \
    log_inner(ctx, level,                                                    \
        PP_NARG(__VA_ARGS__) + 8,                                            \
        message,                                                             \
        __VA_ARGS__,                                                         \
        "levl", _gx_eloglvl[level],                                          \
        "file", __FILE__,                                                    \
        "func", __FUNCTION__,                                                \
        "line", __STR(__LINE__)                                              \
    )                                                      

#define MSG(str, ...) str, __VA_ARGS__


static logger loggers[] = {
    /*panic*/ {.handle = LOG_STDERR, .format = LOG_FORMAT, .color = C_PANIC},
    /*fatal*/ {.handle = LOG_STDERR, .format = LOG_FORMAT, .color = C_FATAL},
    /*error*/ {.handle = LOG_STDERR, .format = LOG_FORMAT, .color = C_ERROR},
    /*warn*/  {.handle = LOG_STDERR, .format = LOG_FORMAT, .color = C_WARN},
    /*info*/  {.handle = LOG_STDERR, .format = LOG_FORMAT, .color = C_INFO},
    /*debug*/ {.handle = LOG_STDERR, .format = LOG_FORMAT, .color = C_DEBUG}
};

static log_format formats[] = {
    /* pipes */ {.begin = "",  .kvformat = "%s:%s",         .delimiter = "|",  .end = "" },
    /* json  */ {.begin = "{", .kvformat = "\"%s\":\"%s\"", .delimiter = ", ", .end = "}"}
};

#define LINE_MAX 1024
#define BUFFER_MAX 1024
#define MESSAGE_MAX 512
static GX_INLINE void log_inner(context *ctx, int level, size_t kv_argc, const char* printf_str, ...) {
    unsigned int n;
    char line[LINE_MAX] = "";
    char buffer[BUFFER_MAX] = "";
    char message[MESSAGE_MAX] = "";
    logger* logger = &loggers[level];

    va_list argp;
    va_start(argp, printf_str);
    //sort of sketchy--we expect here that vsnprintf calls va_arg(argp) the right number of times
    //but works here and is the way the implementation works from what i've seen
    vsnprintf(message, MESSAGE_MAX, printf_str, argp);
    snprintf(buffer, BUFFER_MAX, "%s%s%s", logger->color, message, C_N);
    strncpy(message, buffer, MESSAGE_MAX);
    strncat(line, formats[logger->format].begin, LINE_MAX);
    snprintf(buffer, BUFFER_MAX, formats[logger->format].kvformat, "mesg", message);
    strncat(line, buffer, LINE_MAX - strlen(line));
    for (n = 0; n < kv_argc; n += 2) {
        const char* key   = va_arg(argp, const char*);
        const char* value = va_arg(argp, const char*);
        strncat(line, formats[logger->format].delimiter, LINE_MAX - strlen(line));
        snprintf(buffer, BUFFER_MAX, formats[logger->format].kvformat, key, value);
        strncat(line, buffer, LINE_MAX - strlen(line));
    }
    va_end(argp);

    if (ctx != NULL) {
        kv *keyvalue = ctx->keyvalues;
        while (keyvalue != NULL) {
            snprintf(buffer, BUFFER_MAX, formats[logger->format].kvformat, keyvalue->key, keyvalue->value);
            strncat(line, buffer, LINE_MAX - strlen(line));
            keyvalue = keyvalue->next;
        }
    }
    strncat(line, formats[logger->format].end, LINE_MAX - strlen(line));

    flock(logger->handle, LOCK_EX);
    write(logger->handle, line, strlen(line));
    write(logger->handle, "\n", 1);
    flock(logger->handle, LOCK_UN);
}

static void _GX_ERROR_LOG(int severity) {
    if(gx_unlikely(gx_error_loglevel >= severity)) {
        gx_error_report *rp = &gx_error_recent_reports[gx_error_curr_report];
        char basename[20];
        char expr_part[20];
        char sys_errormsg[1024];
        //char final_logline[4096];
        char *fname;
        _gx_get_basename(rp->filename, basename);
        _gx_get_expr_part(rp->expression, expr_part);

        // Populate sys_errormsg
        if(rp->sys_errno)          strerror_r(rp->sys_errno, sys_errormsg, 1023);
        else if(rp->custom_msg[0]) strncpy(sys_errormsg, rp->custom_msg, 1023);
        else                       strncpy(sys_errormsg, _gx_errmsgs[rp->gx_errno], 255);

        int len_fun = strlen(rp->function);
        int len_exp = strlen(expr_part);
        int full_flen = len_fun + len_exp;

        if(len_exp > 0 && len_fun > 0) {
            strncat(rp->function, "/", 255 - len_fun - 1);
            full_flen += 1;
        }
        strncat(rp->function, expr_part, 255 - len_fun - 2);

        if(full_flen <= 17) fname = rp->function;
        else {
            fname = rp->function + full_flen - 17;
            fname[2] = fname[3] = '.';
            fname[1] = rp->function[1];
            fname[0] = rp->function[0];
        }

        if (rp->sys_errno != 0) {
            log_inner(NULL, severity, 10, sys_errormsg, 
                "levl", _gx_eloglvl[severity],
                "name", _gx_ename[rp->sys_errno], 
                "file", basename, 
                "line", rp->linenum,
                "func", fname);
        } else {
            log_inner(NULL, severity, 8, sys_errormsg, 
                "levl", _gx_eloglvl[severity],
                "file", basename, 
                "line", rp->linenum,
                "func", fname);
        }
    }
}
#endif

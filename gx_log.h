/**
   @file      gx_log.h
   @brief     Quick logging abstractions- used in conjunction usually with gx_error.h
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

## Critical Concerns
     Primary purposes for logging (with some overlap):
       - Product resource usage & performance analysis
       - Product usage analysis
       - State communications & auditing (retroactive investigation)
       - Defect containment/identification/investigation (app & system)
       - Development inspection & investigation


## Commonly Reported Fields

     Common fields for log reports / items. Fields that are automatically
     calculated / included are marked with `*`. Fields that are automatically
     included via some lookup mechanisms are marked with `^`. Scope isn't set
     in stone- any key/value pair can be added / overridden.

     | Field           | Scope  | Description                           | example(s)                    |
     | --------------- | ------ | ------------------------------------- | ----------------------------- |
     | type            | all    | Adhoc namespace for log items         | broadcast_state, system_state |
     | severity        | all    | naive severity from app perspective   | warning                       |
     | name            | all    | Single identifier for log item        | stream_up, syserr_eacces, ... |
     | msg             | log    | Brief message / description / note    | publishing went live.         |
     | report          | log    | Multiline formatted report or data    | payload hexdump: %n 0x1289... |

     | time*           | all    | UTC iso8601 formatted time of report  | 2012-09-17T09:49Z             |
     | ticks*          | all    | Current CPU ticks                     | 28912                         |
     | host*           | all    |                                       | dev6.justin.tv                |
     | program*        | all    |                                       | imbibe                        |
     | version*        | all    |                                       | v0.0.5+build.22.c             |
     | pid*            | all    |                                       | 123                           |
     | ppid*           | all    |                                       | 5                             |
     | tid*            | all    | if available, thread id               | 891                           |
     | src_file*       | all    |                                       | imb_rtmp.c                    |
     | src_line*       | all    |                                       | 52                            |
     | src_function*   | all    |                                       | handle_connection             |

     | err_family^     | error  | What kind of error                    | syserror, gxerror             |
     | err_label^      | error  | The constant used for the error       | EFAULT, EACCES, ...           |
     | err_msg^        | error  | Looked up string description of err   | "Permission Denied."          |
     | err_severity^   | error  | intrinsic risk of an error            | critical                      |
     | result          | all    | impact / severity / recovery-action...| session_killed, msg_skipped...|

     | err_depth*      | error  | # in sequence of related errors       | 2                             |
     | err_group*      | error  | guid that associates several reports  | w839u21                       |
     | err_number^     | error  | Depends on error class- usually errno | 17                            |
     | src_expression* | error  | Expression that triggered error       | open(filename, "r+")          |
     | err_stack*      | error  | multiline msg w/ stack trace info     | (nyi)                         |

     | (ad-hoc pairs)  | all    | Misc adhoc key/value pair assoc. data | desired_file=somefile.txt     |


## Log Message Severity.

  RFC 5424 defines eight severity levels described below (notes from wikipedia
  plus some additional notes, clarification, and customizations). I've given
  each level the following attributes:
    - A brief description
    - (Likely) Scope:  How big of a problem is infered or explicitly noted by
      a message of this level- how many users / systems, etc.
    - (Likely) Persistence:  How likely the error or similar errors are to
      repeat themselves. Keep in mind these are orthogonal to the scope but
      together with the scope and desired action inform the severity. Some
      common values:
        - System-permanent: it will not ever fix itself unless there is manual
          intervention at the system level. e.g., required protocol not
          implemented on / available on current OS.
        - System-transient: It is a system-level problem (like resources or
          network connection) that may resolve itself without manual
          intervention. e.g., out of memory
        - App-permanent: Something in the way the application is coded and
          to a lesser degree it's current runtime context guarantees the error
          will not go away without manual intervention (and/or code changes).
          e.g., a segfault
        - App-transient: May heal itself / stop repeating at the process
          level. Usually pretty easy to code correct behavior. e.g., EINTR.
    - (Desired) Action:  What kind of action this severity usually calls for.


  -- scope -----------------\
  -- persistence ------------>  severity/urgency -> notification-action
  -- intervention-needed ---/


  -------------------------------------------------------------
  0. EMERGENCY: Whole system disruption or serious destabilization
  -------------------------------------------------------------
     Scope:       Potentially all application users and more. May indicate
                  problems that affect larger systems.
     Persistence: System-permanent or sometimes system-transient
     Action:      Manual intervention almost certainly required. At this level
                  usually notify all tech staff on call.

  -------------------------------------------------------------
  1. ALERT:     Full App disruption or serious destabilization
  -------------------------------------------------------------
     Scope:       Most or all of the application users / potential users. App
                  cannot recover.
     Persistence: System-transient, app-permanent, some system-permanent
     Action:      Fast manual intervention and sometimes code changes
                  probably required. Therefore notify staff that can fix the
                  problem.

  -------------------------------------------------------------
  2. CRITICAL:  Critical conditions.
  -------------------------------------------------------------
     Scope:       Either many users disrupted or destabilized such that they
                  may soon be disrupted. e.g., Worker process dies along with
                  all the sessions it was handling... Also usually includes
                  failure of redundant systems- i.e., backup ISP connection
                  goes down.
     Persistence: Partially or fully recoverable but indicates possible
                  problem escalation and/or needs manual intervention.
     Action:      Should be corrected immediately to avoid Alerts and
                  Emergencies.

  -------------------------------------------------------------
  3. ERROR:     Non-urgent failures. Verified defect hits.
  -------------------------------------------------------------
     Scope:       Session-level and application defined disruptions affecting
                  individual users.
     Persistence: Session-transient. App and system fully recover but bug will
                  remain.
     Action:      Relayed to developers and/or admins and resolved in a given
                  timeframe.

  -------------------------------------------------------------
  4. WARNING:   Warning conditions.
  -------------------------------------------------------------
     Scope:       Usually unknown- an indicator of instability and potential
                  problem escalation. Unrecognized usage / code paths that
                  haven't been fully developed, etc. Failed assertions, etc.
     Persistence: Often session-transient. These are dangerous because if
                  warnings never escalate they start to get ignored even
                  though rarely seen warnings can be very urgent.
     Action:      Relayed to developers and/or admins and resolved in a given
                  timeframe.

  -------------------------------------------------------------
  5. NOTICE:   Normal but significant condition.
  -------------------------------------------------------------
     Scope:       Usually unknown in advance. The events are unusual but not
                  directly destabilizing or disruptive. Usually they concern
                  app-specific usage and user behavior assumptions. May
                  indicate that different optimization decisions should be
                  made etc.
     Persistence: Any
     Action:      Email-speed notifications or reports of aggregated totals
                  for further analysis. No immediate action required.

  -------------------------------------------------------------
  6. INFO/STAT: Informational messages.
  -------------------------------------------------------------
     Scope:       Normal operational messages - may be harvested for
                  reporting, measuring throughput, etc.
     Persistence: N/A
     Action:      Record, aggregate, report, etc. No immediate action
                  required.

  -------------------------------------------------------------
  7. DEBUG:    Debug-level messages.
  -------------------------------------------------------------
     Scope:       Info useful to developers for debugging the application, not
                  useful during operations.
     Persistence: Development context
     Action:      Developer specific

*/
#ifndef GX_LOG_H
#define GX_LOG_H

#include "./gx.h"
#include "./gx_string.h"
#include "./gxe/gx_enum_lookups.h"
#include <time.h>
#include <sys/uio.h>

/// @todo  Finish populating comments from table above

typedef enum gx_log_standard_keys {
    K_type=0,           ///< Adhoc namespace for log items. e.g., broadcast_state, system_state, ...
    K_severity,         ///< Declared severity from app.    e.g., SEV_WARNING
    K_name,             ///< Tag / name for grouping logs.  e.g., stream_up, syserr_eacces, ...

    K_msg,              ///< Brief app message / description / note. e.g., "publisher went live."
    K_report,           ///< Multiline formatted report / data
    K_result,           ///< App message indicating risk mitigation action taken etc.

    K_src_file,
    K_src_line,
    K_src_function,
    K_src_expression,

    K_err_severity,
    K_err_family,
    K_err_number,
    K_err_label,
    K_err_msg,

    K_err_depth,
    K_err_group,
    K_err_stack,

    K_sys_time,
    K_sys_ticks,

    K_sys_program,
    K_sys_version,

    K_sys_cpuid,
    K_sys_pid,
    K_sys_ppid,
    K_sys_tid,

    K_net_host,
    K_net_bound_ip,
    K_net_bound_port,
    K_net_peer_ip,
    K_net_peer_port,
    K_net_peer_state
} gx_log_standard_keys;
#define K_END_STANDARD (K_net_peer_state)

typedef enum gx_severity {
    SEV_EMERGENCY,   ///< Whole system disruption or serious destabilization.
    SEV_ALERT,       ///< Full application disruption or serious destabilization.
    SEV_CRITICAL,    ///< Process-level disruption or destabilization needing intervention.
    SEV_ERROR,       ///< Session-level disruptions. Failures. Defects.
    SEV_UNKNOWN,     ///< Unknown severity. (Usually for native syserr severities)
    SEV_WARNING,     ///< Potential instability needing attention.
    SEV_NOTICE,      ///< Normal but significant condition needing some monitoring- high priority stat.
    SEV_STAT,        ///< Information being gathered for specific analytics or monitoring
    SEV_INFO,        ///< General informational messages/reports possibly useful for analysis.
    SEV_DEBUG        ///< Assistance for development/debugging but not useful during operations.
} gx_severity;

#define gx_log_set_key_lookup(FUN)                                             \
    static const char * (*_gx_log_keystr)(int) = FUN

#define gx_log_update_sysinfo()    do{                                         \
    $Sreset(_gx_log_sysinfo);                                                  \
    _gx_log_update_pids();                                                     \
    _gx_log_update_cpuid();                                                    \
    _gx_log_update_host();                                                     \
} while(0)

#define gx_log_set_prog(PROG,VER)  do{                                         \
    gx_log_set_program(PROG);                                                  \
    gx_log_set_version(VER);                                                   \
} while(0)

#define gx_log_set_program(PROG)   gx_log_set(K_sys_program, PROG)
#define gx_log_set_version(VER)    gx_log_set(K_sys_version, VER)
#define gx_log_set(KEY, VALUE)     _gx_log_set_inner(KEY,    VALUE)


/// Some static variables that hold state for some system key/value entries
static const char * (*_gx_log_keystr)(int);
static uint16_t       _gx_log_time_len   = 3;
static char           _gx_log_time[256]  = {0};
static unsigned int   _gx_log_last_tick  = 0;
static gx_strbuf      _gx_log_sysinfo    = {{0},NULL};

static char _GX_NULLSTRING[] = "";

#define kv_main_head_t uint32_t                        ///< Type of full-message header (gives length of msg)
#define kv_head_t      uint16_t                        ///< Type of length headers before strings
#define _iov_base      typeofm(struct iovec, iov_base) ///< Usually 'void *' or 'char *'
#define _iov_size      typeofm(struct iovec, iov_len ) ///< Usually 'size_t' or 'int'

/// Key-value structure, set up a bit redundantly so that it already
/// encapsulates the output format in a layout ready for scatter-io.
typedef struct _gx_kv {
    _iov_base key_head_base;     ///< For protocol, size(dat) + 1 (null-term) + 2 (size-size)
    _iov_size key_head_size;     ///< 2 (size-size)
    _iov_base key_data_base;     ///< Point to payload for key, plus null-term byte
    _iov_size key_data_size;     ///< Size of key_data_base including null-term byte.

    _iov_base val_head_base;
    _iov_size val_head_size;
    _iov_base val_data_base;
    _iov_size val_data_size;
} packed _gx_kv;

/// gen/build-gx_log_table.rb reads in the standard_keys enum above and uses
/// the information to declare / define the following:
/// @param ADHOC_OFFSET         (defined) index of first adhoc kv pair
/// @param KV_ENTRIES           (defined) Total # of kv pairs
/// @param _key_sizes_master    kv_head_t array with default key sizes
/// @param _val_sizes_master    kv_head_t array with default value sizes
/// @param msg_tab_master       used to clear/default the msg_iov.msg_tab below
#include "./gxe/gx_log_table.h"

static kv_head_t _key_sizes[KV_ENTRIES];     ///< Key header + payload sizes
static kv_head_t _val_sizes[KV_ENTRIES];     ///< Val header + payload sizes
static unsigned int curr_adhoc_offset = ADHOC_OFFSET; ///< Changes as "semi-permanent" k/v pairs are added/removed

typedef struct kv_msg_iov {
    _iov_base  main_head_base;
    _iov_size  main_head_size;
    _gx_kv     msg_tab[KV_ENTRIES];
    _gx_kv     main_tail;
} packed kv_msg_iov;

typedef struct gx_logger {
    int            enabled;
    void         (*logger_function)(gx_severity, kv_msg_iov *);
    gx_severity    min_severity;
} gx_logger;

#define GX_NUM_STD_LOGGERS 3
static gx_logger gx_loggers[GX_NUM_STD_LOGGERS];

/// Standard loggers... don't really need this more configurable at the moment...
#include "./gxe/log_stderr.h"
#include "./gxe/log_syslogd.h"
#include "./gxe/log_mq.h"

static gx_logger gx_loggers[3] = {
    {1, &log_stderr,  SEV_DEBUG},
//    {1, &log_syslogd, SEV_CRITICAL},
//    {1, &log_mq,      SEV_DEBUG}
};


/// Number of iov-elements in a "flattened" kv_msg_iov:
/// ((message-table-entries + main-tail) * iovs-per-_gx_kv) + main-head
/// If you really wanted to you could also divide sizeof(kv_msg_iov) by sizeof(struct iovec)
#define KV_IOV_COUNT ((KV_ENTRIES + 1) * 4) + 1
/// msg_iov static variable as an array of iovecs
#define MSG_AS_IOV(MSG)  ((struct iovec *)&(MSG))

static const kv_head_t kv_head_empty = (kv_head_t)(1 + sizeof(kv_head_t));
static kv_main_head_t  kv_main_head  = 0;

static kv_msg_iov msg_iov = {
    .main_head_base = &kv_main_head,
    .main_head_size = sizeof(kv_main_head),
    .main_tail      = {(_iov_base)&kv_head_empty, sizeof(kv_head_empty),
                       (_iov_base)_GX_NULLSTRING, 0x01,
                       (_iov_base)&kv_head_empty, sizeof(kv_head_empty),
                       (_iov_base)_GX_NULLSTRING, 0x01}
};

/// Change any NULL pointers into values that point to _GX_NULLSTRING
#define _STR_NULL(P) ({                                                        \
    typeof(P) _p = (P);                                                        \
    (void *)_p == (void *)NULL ? _GX_NULLSTRING : _p;                          \
    })


/// @note Format always has bodies with at least one byte (a NULL) and
///       therefore size of at least 1 + sizeof(size-header-type)

/// Macros for setting value in kv pair.
///   - ...VAL:    string + default-key
///   - ...VAL_O:  string only (need to set key yourself)
///   - ...VAL_L:  string/buffer with length (minus null-term) known + default-key
///   - ...VAL_LO: implement it if you need it
#define KV_SET_VAL(IDX, STR)            KV_SET_VAL_O   (IDX, STR);             \
                                        KV_DEFAULT_KEY (IDX)
#define KV_SET_VAL_O(IDX, STR)          KV_SET_PART_STR(IDX, val, STR)
#define KV_SET_VAL_L(IDX, BUF, LEN)     KV_SET_PART    (IDX, val, BUF, LEN);   \
                                        KV_DEFAULT_KEY (IDX)

/// Macros for setting the key in the kv pair.
///   - ...KEY:    set it, assumes a string. (for adhoc keys usually)
///   - ...DEFAULT_KEY: sets iovec sizes, leaving (default) key values in place
#define KV_SET_KEY(IDX, STR)            KV_SET_PART_STR(IDX, key, STR)
#define KV_DEFAULT_KEY(IDX)             do {                                   \
    msg_iov.msg_tab[(IDX)].key_head_size = sizeof(kv_head_t);                  \
    msg_iov.msg_tab[(IDX)].key_data_size =                                     \
            (_iov_size)(_key_sizes_master[IDX] - sizeof(kv_head_t));           \
}while(0)


#define KV_SET_PART_STR(IDX, PART, S)   do {                                   \
    size_t _len=strlen(S);                                                     \
    KV_SET_PART(IDX, PART, S, _len);                                           \
}while(0)
#define KV_SET_PART(IDX, PART, BODY, LEN) do {                                 \
    msg_iov.msg_tab[(IDX)].PART ## _head_base = &(_ ## PART ## _sizes[IDX]);   \
    _ ## PART ## _sizes[IDX] = (kv_head_t)(LEN + 1 + sizeof(kv_head_t));       \
    msg_iov.msg_tab[(IDX)].PART ## _head_size = sizeof(kv_head_t);             \
    msg_iov.msg_tab[(IDX)].PART ## _data_base = (_iov_base)(_STR_NULL(BODY));  \
    msg_iov.msg_tab[(IDX)].PART ## _data_size = LEN + 1;                       \
}while(0)

#define _gx_log_set_inner(KEY, VALUE)     do {                                 \
    unsigned int  _idx   = (KEY);                                              \
    char         *_val   = (VALUE);                                            \
    size_t        _vsize = strlen(_val) + 1;                                   \
    if(rare(_idx > curr_adhoc_offset)) {                                       \
        /* SORRY NOT YET IMPLEMENTED                                    */     \
        /* curr_adhoc_offset ++;                                        */     \
        /* put key-head-base, key-data-base,                            */     \
        /* and _key_sizes_master for key_data_size... but which index?? */     \
        break;                                                                 \
    }                                                                          \
    _val_sizes_master[_idx]                 =                                  \
        (kv_head_t)(_vsize + sizeof(kv_head_t));                               \
    msg_tab_master[_idx].key_head_size      =                                  \
    msg_tab_master[_idx].val_head_size      = sizeof(kv_head_t);               \
    msg_tab_master[_idx].key_data_size      =                                  \
        (_iov_size)(_key_sizes_master[_idx] - sizeof(kv_head_t));              \
    msg_tab_master[_idx].val_data_size      = (_iov_size)_vsize;               \
    msg_tab_master[_idx].val_data_base      = (_iov_base)_val;                 \
} while(0)


// Forward declarations
//static inline   void  gx_writev_buf      (char *dst, const struct iovec *iov, int iovcnt);
static noinline void _gx_log_inner       (gx_severity severity, char *severity_str,
                                           int vparam_count, va_list *vparams, int argc, ...);
static inline   void _gx_log_dispatch    (gx_severity severity, kv_msg_iov *msg);
static inline   void _gx_log_update_host ();
static inline   void _gx_log_update_pids ();
static inline   void _gx_log_update_cpuid();
static inline   void gx_hexdump(void *buf, size_t len, int more);


static inline void _gx_log_update_cpuid() {
    /// Waiting for an OS/CPU portable enencumbered fast apic-id finder
}

static inline void _gx_log_update_pids() {
    gx_log_set(K_sys_pid,  $S(_gx_log_sysinfo, "%u", getpid ()));
    gx_log_set(K_sys_ppid, $S(_gx_log_sysinfo, "%u", getppid()));
}

static inline void _gx_log_update_host() {
    /// @todo use sysctl on mac osx for better hostid
    gx_log_set(K_net_host, $S(_gx_log_sysinfo, "0x%08lx", gethostid()));
}

/// Main logging functionality.
/// @note gx.h's KV(...) macro needs to continue to ensure that there is a value for every key
/// @todo error handling if not enough key/value pairs (simply unused conditional below)
#define _gx_log(SEV, SSEV, VPCOUNT, VPARAMS, ...)                              \
    _gx_log_inner(SEV, SSEV, VPCOUNT, VPARAMS, KV(__VA_ARGS__))

/// For now these are mostly for backwards compatibility with the old X_LOG_* macros
#define _gx_log_msg(SEV,...) _gx_log(SEV, #SEV, 0, NULL, K_msg, $(__VA_ARGS__))
#define log_emergency(...)   _gx_log_msg(SEV_EMERGENCY, __VA_ARGS__)
#define log_alert(...)       _gx_log_msg(SEV_ALERT,     __VA_ARGS__)
#define log_critical(...)    _gx_log_msg(SEV_CRITICAL,  __VA_ARGS__)
#define log_error(...)       _gx_log_msg(SEV_ERROR,     __VA_ARGS__)
#define log_warning(...)     _gx_log_msg(SEV_WARNING,   __VA_ARGS__)
#define log_notice(...)      _gx_log_msg(SEV_NOTICE,    __VA_ARGS__)
#define log_info(...)        _gx_log_msg(SEV_INFO,      __VA_ARGS__)
#define log_stat(...)        _gx_log_msg(SEV_STAT,      __VA_ARGS__)
#define log_debug(...)       _gx_log_msg(SEV_DEBUG,     __VA_ARGS__)
#define log_unknown(...)     _gx_log_msg(SEV_UNKNOWN,   __VA_ARGS__)

#define _VA_ARG_TO_TBL(VALIST) {                                               \
    unsigned int  _kv_key = va_arg((VALIST), unsigned int);                    \
    char         *_kv_val = va_arg((VALIST), char *);                          \
    if(freq(_kv_key < curr_adhoc_offset)) {                                    \
        KV_SET_VAL (_kv_key, _kv_val);                                         \
    } else if(freq(adhoc_idx < KV_ENTRIES)) {                                  \
        KV_SET_VAL_O(adhoc_idx, _kv_val);                                      \
        if(_gx_log_keystr) KV_SET_KEY(adhoc_idx, _gx_log_keystr(_kv_key));     \
        else               KV_SET_KEY(adhoc_idx, $("custom_%u", _kv_key));     \
        adhoc_idx ++;                                                          \
    }                                                                          \
}

typedef union _ctick {
    char ctick_buf[9];
    struct {
        char     empty_head;
        uint64_t ctick_data;
    } packed;
} _ctick;

static char ctick_base64[64];

static inline char *_gx_cpu_ts_str(char *dest, uint64_t ts) {
    _ctick ctick_transformer;
    ctick_transformer.empty_head = 0;
    ctick_transformer.ctick_data = bswap64(ts);
    gx_base64_urlencode_m3(ctick_transformer.ctick_buf, 9, dest);
    char *p = dest;
    while(*p == '0') p++;
    return p;
}

static noinline void _gx_log_inner(gx_severity severity, char *severity_str,
        int vparam_count, va_list *vparams, int argc, ...)
{
    va_list argv;
    int     i;

    memcpy(&msg_iov.msg_tab, &msg_tab_master, sizeofm(kv_msg_iov,msg_tab)); // yes it's fastest

    int adhoc_idx = curr_adhoc_offset;

    // Usually "expanded" values

    //gx_hexdump(&(msg_iov.msg_tab), sizeof(msg_iov.msg_tab), 0);
    if(argc > 0) {
        va_start(argv, argc);
        for(i = 0; i < argc; i+=2) _VA_ARG_TO_TBL(argv);
    }
    //gx_hexdump(&(msg_iov.msg_tab), sizeof(msg_iov.msg_tab), 0);

    // Passed in at the highest level generally- highest precedence
    if(vparam_count > 0) for(i = 0; i < vparam_count; i+=2) _VA_ARG_TO_TBL(*vparams);

    // Absolute highest precedence
    KV_SET_VAL(K_severity, severity_str);

    if(freq(msg_iov.msg_tab[K_sys_time].val_data_size <= 3)) {
        uint64_t curr_tick = cpu_ts;
        if(!_gx_log_last_tick || abs(curr_tick - _gx_log_last_tick) > 250000000) {
            time_t now = time(NULL);
            struct tm now_tm;
            gmtime_r(&now, &now_tm);
            _gx_log_time_len = strftime(_gx_log_time, sizeof(_gx_log_time)-1,
                    "%Y-%m-%dT%H:%M:%SZ", &now_tm);
            _gx_log_last_tick = curr_tick;
        }
        KV_SET_VAL_L(K_sys_time,  &(_gx_log_time[0]), _gx_log_time_len);
        KV_SET_VAL  (K_sys_ticks, _gx_cpu_ts_str(ctick_base64, curr_tick));
    }

    kv_main_head = 0;
    for(i = 0; i < KV_IOV_COUNT; i++) kv_main_head += (kv_main_head_t)(MSG_AS_IOV(msg_iov)[i].iov_len);

    _gx_log_dispatch(severity, &msg_iov);
    $reset();
#ifdef DEBUG_LOGGING
    fprintf(stderr, "\n-----------------------------------------------------------------------------\n");
    int row_num = 0;
    for(i = 0; i < KV_ENTRIES; i++) {
        _gx_kv *kv = &(msg_iov.msg_tab[i]);
        if(kv->val_data_size > 0) {
            fprintf(stderr, "|%3d|%3d "
                            "|%zu> 0x%04x(=%2u) |%2zu>   %-17s "
                            "|%zu> 0x%04x(=%2u) |%2zu>   %-25s\n",
                    row_num, i,
                    kv->key_head_size, *((kv_head_t *)kv->key_head_base),*((kv_head_t *)kv->key_head_base),
                    kv->key_data_size, (char *)kv->key_data_base,
                    kv->val_head_size, *((kv_head_t *)kv->val_head_base),*((kv_head_t *)kv->val_head_base),
                    kv->val_data_size, (char *)kv->val_data_base);
            row_num ++;
        }
    }
    fprintf(stderr, "\n");
#endif

    if(argc > 0) va_end(argv);
}
#undef _VA_ARG_TO_TBL


static inline void _gx_log_dispatch(gx_severity severity, kv_msg_iov *msg) {
    //ssize_t actual_len = writev(STDERR_FILENO, MSG_IOV_IOV, KV_IOV_COUNT);
    int i;
    for(i = 0; i < GX_NUM_STD_LOGGERS; i ++) {
        if(freq(gx_loggers[i].enabled)) {
            if(gx_loggers[i].min_severity >= severity) (gx_loggers[i].logger_function)(severity, msg);
        }
    }
}

/*
static inline size_t  gx_writev_buf (char *dst, size_t maxsize, const struct iovec *iov, int iovcnt) {
    int    i;
    size_t copied;
    for(i = 0; i < iovcnt; i++) {
        size_t len = iovec[i].iov_len;
        if(rare(len > (maxsize - copied - 1)) return copied;
    }

}
*/

/// @todo use normal logging and K_report, fix name
static inline void gx_hexdump(void *buf, size_t len, int more) {
    size_t i=0, tch; int val, grp, outsz=0, begin_col;
    size_t begin_line;
    uint8_t *bf = (uint8_t *)buf;
    static const char *utf8_ascii[] = {
        "␀", "␁", "␂", "␃", "␄", "␅", "␆", "␇", "␈", "␉", "␊", "␋", "␌", "␍",
        "␎", "␏", "␐", "␑", "␒", "␓", "␔", "␕", "␖", "␗", "␘", "␙", "␚", "␛",
        "␜", "␝", "␞", "␟", "␡"};
    flockfile(stderr);
    fprintf(stderr, "     ");
    if(len == 0) {fprintf(stderr, "\\__/\n"); return;}
    while(1) {
        begin_line = i;
        begin_col  = outsz;
        for(grp=0; grp<3; grp++) {
            fprintf(stderr, "| "); outsz += 2;
            for(val=0; val<4; val++) {
                fprintf(stderr, "%02X ", bf[i]); outsz += 3;
                i++;
                if(i>=len) break;
            }
            if(i>=len) break;
        }
        fprintf(stderr, "| "); outsz += 1;
        if(i>=len) break;
        for(tch = outsz - begin_col; tch < 43; tch++) fprintf(stderr, " ");
        for(tch = begin_line; tch < i; tch++) {
            if(bf[tch] < 0x20)       fprintf(stderr, "%s", utf8_ascii[bf[tch]]);
            else if(bf[tch] <= 0x7E) fprintf(stderr, "%c", bf[tch]);
            else                     fprintf(stderr, "·");
        }
        fprintf(stderr, "\n     ");
    }
    if(more) { fprintf(stderr,"... ... ..."); outsz += 11; }
    for(tch = outsz - begin_col; tch < 41; tch++) fprintf(stderr, " ");
    fprintf(stderr, "| ");
    for(tch = begin_line; tch < i; tch++) {
        if(bf[tch] < 0x20)       fprintf(stderr, "%s", utf8_ascii[bf[tch]]);
        else if(bf[tch] <= 0x7E) fprintf(stderr, "%c", bf[tch]);
        else                     fprintf(stderr, "·");
    }
    fprintf(stderr,"\n     \\");
    for(val=0; val<min(outsz, ((3 * 4 /* 1group */) + 2 /* front-bar+spc */) * 3 + 1) - 2; val++) fprintf(stderr,"_");
    fprintf(stderr,"/\n");
    funlockfile(stderr);
}


#endif

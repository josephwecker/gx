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
#include "./gxe/gx_enum_lookups.h"
#include <time.h>
#include <sys/uio.h>


#define GX_LOG_MAX_KEY_LEN 64
#define GX_LOG_MAX_VAL_LEN 255
#define GX_LOG_MAX_RPT_LEN 4096


static const char * (*gx_log_keystring_callback)(int);
#define GX_LOG_SET_FKEYSTR(FUN) static const char * (*gx_log_keystring_callback)(int) = FUN


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

    K_sys_cpuid,
    K_sys_time,
    K_sys_ticks,
    K_sys_host,
    K_sys_program,
    K_sys_version,
    K_sys_pid,
    K_sys_ppid,
    K_sys_tid
} gx_log_standard_keys;
#define K_END_ADHOC (K_sys_tid)

typedef enum gx_severity {
    SEV_EMERGENCY,   ///< Whole system disruption or serious destabilization.
    SEV_ALERT,       ///< Full application disruption or serious destabilization.
    SEV_CRITICAL,    ///< Process-level disruption or destabilization needing intervention.
    SEV_ERROR,       ///< Session-level disruptions. Failures. Defects.
    SEV_WARNING,     ///< Potential instability needing attention.
    SEV_NOTICE,      ///< Normal but significant condition needing some monitoring- high priority stat.
    SEV_STAT,        ///< Information being gathered for specific analytics or monitoring
    SEV_INFO,        ///< General informational messages/reports possibly useful for analysis.
    SEV_DEBUG,       ///< Assistance for development/debugging but not useful during operations.
    SEV_UNKNOWN
} gx_severity;

#define kv_head_t uint16_t                        ///< Type of length headers before strings
#define kv_base_t typeofm(struct iovec, iov_base) ///< Usually 'void *' or 'char *'
#define kv_size_t typeofm(struct iovec, iov_len ) ///< Usually 'size_t' or 'int'


/// Key-value structure, set up a bit redundantly so that it already
/// encapsulates the output format in a layout ready for scatter-io.
typedef struct _gx_kv {
    kv_base_t key_head_base;     ///< For protocol, size(dat) + 1 (null-term) + 2 (size-size)
    kv_size_t key_head_size;     ///< 2 (size-size)
    kv_base_t key_data_base;     ///< Point to payload for key, plus null-term byte
    kv_size_t key_data_size;     ///< Size of key_data_base including null-term byte.

    kv_base_t val_head_base;
    kv_size_t val_head_size;
    kv_base_t val_data_base;
    kv_size_t val_data_size;
} __packed _gx_kv;

/// Change any NULL pointers into values that point to _GX_NULLSTRING
#define _STR_NULL(P) ({ typeof(P) _p = (P); (void *)_p == (void *)NULL ? _GX_NULLSTRING : _p; })
static char _GX_NULLSTRING[] = "";


/// @note Format always has bodies with at least one byte (a NULL) and
///       therefore size of at least 1 + sizeof(size-header-type)
#define _SET_KV_VAL2(IDX,BUF,LEN)        _SET_KV_PART    (IDX, val, BUF, LEN)
#define _SET_KV_KEY2(IDX,BUF,LEN)        _SET_KV_PART    (IDX, key, BUF, LEN)
#define _SET_KV_VAL(IDX,STR)             _SET_KV_PART_STR(IDX, val, STR)
#define _SET_KV_KEY(IDX,STR)             _SET_KV_PART_STR(IDX, key, STR)
#define _SET_KV_PART_STR(IDX,PART,S)     do {                                  \
    size_t _len=strlen(S);                                                     \
    _SET_KV_PART(IDX,PART,S,_len);                                             \
}while(0)

#define _ENABLE_KEY(IDX)                 do {                                  \
    msg_iov.msg_tab[(IDX)].key_head_size = sizeof(kv_head_t);                  \
    msg_iov.msg_tab[(IDX)].key_data_size =                                     \
            (kv_head_t)(_key_sizes_master[IDX] - sizeof(kv_head_t));           \
}while(0)

#define _SET_KV_PART(IDX,PART,BODY,SIZE) do {                                  \
    msg_iov.msg_tab[(IDX)].PART ## _head_base = &(_ ## PART ## _sizes[IDX]);   \
    _ ## PART ## _sizes[IDX] = (kv_head_t)(SIZE + 1 + sizeof(kv_head_t));      \
    msg_iov.msg_tab[(IDX)].PART ## _head_size = sizeof(kv_head_t);             \
    msg_iov.msg_tab[(IDX)].PART ## _data_base = (kv_base_t)(_STR_NULL(BODY));  \
    msg_iov.msg_tab[(IDX)].PART ## _data_size = SIZE + 1;                      \
}while(0)


static uint16_t      _gx_log_time_len  = 3;
static char          _gx_log_time[256] = {0};
static unsigned int  _gx_log_last_tick = 0;
// host
// program
// version

/// gen/build-gx_log_table.rb reads in the standard_keys enum above and uses
/// the information to declare / define the following:
/// @param ADHOC_OFFSET         (defined) index of first adhoc kv pair
/// @param KV_ENTRIES           (defined) Total # of kv pairs
/// @param _key_sizes_master    kv_head_t array with default key sizes
/// @param _val_sizes_master    kv_head_t array with default value sizes
/// @param msg_tab_master       used to clear/default the msg_iov.msg_tab below
#include "./gxe/gx_log_table.h"

static kv_head_t _key_sizes[KV_ENTRIES];
static kv_head_t _val_sizes[KV_ENTRIES];

typedef struct kv_msg_iov {
    struct {
        kv_base_t main_head_base;
        kv_size_t main_head_size;
    };
    _gx_kv msg_tab[KV_ENTRIES];
} __packed kv_msg_iov;
static kv_msg_iov msg_iov;


/// Main logging functionality.
/// @note gx.h's KV(...) macro needs to continue to ensure that there is a value for every key
#define _gx_log(SEV, SSEV, VPCOUNT, VPARAMS, ...)                              \
    _gx_log_inner(SEV, SSEV, VPCOUNT, VPARAMS, KV(__VA_ARGS__))

#define _VA_ARG_TO_TBL(VALIST) {                                               \
    unsigned int  _kv_key = va_arg((VALIST), unsigned int);                    \
    char         *_kv_val = va_arg((VALIST), char *);                          \
    if(_freq(_kv_key < ADHOC_OFFSET)) {                                        \
        _SET_KV_VAL (_kv_key, _kv_val);                                        \
        _ENABLE_KEY (_kv_key);                                                 \
    } else if(_freq(adhoc_idx < KV_ENTRIES)) {                                 \
        _SET_KV_VAL(adhoc_idx, _kv_val);                                       \
        /* TODO: set key w/ lookup function */                                 \
        adhoc_idx ++;                                                          \
    }                                                                          \
}

static _noinline void _gx_log_inner(gx_severity severity, char *severity_str,
        int vparam_count, va_list *vparams, int argc, ...)
{
    int i;

    memcpy(&msg_iov.msg_tab, &msg_tab_master, sizeofm(kv_msg_iov,msg_tab)); // yes it's fastest

    int adhoc_idx = ADHOC_OFFSET;

    // Usually "expanded" values
    if(argc > 0) {
        va_list argv;
        va_start(argv, argc);
        for(i = 0; i < argc; i+=2) _VA_ARG_TO_TBL(argv);
        va_end(argv);
    }

    // Passed in at the highest level generally- highest precedence
    if(vparam_count > 0) for(i = 0; i < vparam_count; i+=2) _VA_ARG_TO_TBL(*vparams);

    // Absolute highest precedence
    _SET_KV_VAL(K_severity, severity_str);


    if(_freq(msg_iov.msg_tab[K_sys_time].val_data_size <= 3)) {
        uint64_t curr_tick = GX_CPU_TS;
        if(!_gx_log_last_tick || abs(curr_tick - _gx_log_last_tick) > 250000000) {
            time_t now = time(NULL);
            struct tm now_tm;
            gmtime_r(&now, &now_tm);
            _gx_log_time_len = strftime(_gx_log_time, sizeof(_gx_log_time)-1,
                    "%Y-%m-%dT%H:%M:%SZ", &now_tm);
            _gx_log_last_tick = curr_tick;
        }
        _SET_KV_VAL2(K_sys_time, &(_gx_log_time[0]), _gx_log_time_len);
        char *ctick_string = $("%llu", (long long int)curr_tick);
        _SET_KV_VAL (K_sys_ticks, ctick_string);
    }

    //writev(STDERR_FILENO, (struct iovec *)&msg_iov, (KV_ENTRIES)*4 + 1);

#ifdef DEBUG_LOGGING
    fprintf(stderr, "\n-----------------------------------------------------------------------------\n");
    int row_num = 0;
    for(i = 0; i < KV_ENTRIES; i++) {
        _gx_kv *kv = &(msg_iov.msg_tab[i]);
        if(kv->val_data_size > 3) {
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
}

#undef _VA_ARG_TO_TBL

#endif

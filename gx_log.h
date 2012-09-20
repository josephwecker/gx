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

#define GX_LOG_MAX_KEY_LEN 64
#define GX_LOG_MAX_VAL_LEN 255
#define GX_LOG_MAX_RPT_LEN 4096


static const char * (*gx_log_keystring_callback)(int);
#define GX_LOG_SET_FKEYSTR(FUN) static const char * (*gx_log_keystring_callback)(int) = FUN


/// @todo  Finish populating comments from table above

#define K_START_STD 128
#define K_START_ADHOC 256

typedef enum gx_log_standard_keys {
    K_type=K_START_STD, ///< Adhoc namespace for log items. e.g., broadcast_state, system_state, ...
    K_severity,         ///< Declared severity from app.    e.g., SEV_WARNING
    K_name,             ///< Tag / name for grouping logs.  e.g., stream_up, syserr_eacces, ...

    K_msg,           ///< Brief app message / description / note. e.g., "publisher went live."
    K_report,        ///< Multiline formatted report / data
    K_result,        ///< App message indicating risk mitigation action taken etc.

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
    K_sys_host,
    K_sys_program,
    K_sys_version,
    K_sys_pid,
    K_sys_ppid,
    K_sys_tid
} gx_log_standard_keys;

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


/// Pretty much the main things that get passed by value in C, but to build an
/// array of them.
typedef union gx_log_val {
    int            v_int;
    void          *v_ptr;
    double         v_dbl;
    char          *v_str;
    long int       v_long_int;
    long long int  v_long_long_int;
} gx_log_val;

typedef struct _gx_log_skv {
    //uint16_t 

} _gx_log_skv;

// TODO:
//   - high-level logging macros
//   - runtime loglevel mechanism
//   - macro for automatic argc/argv adhoc parameters for _gx_log_inner
//   - memoized timer to minimize system calls
// TODO:
//   - determine core-logger destinations- early abort if no logger wants it
//   - fold in app, process, and time parameters
//   - dispatch 

#define _gx_log(VPCOUNT, VPARAMS, APARAMS, ...) _gx_log_inner(VPCOUNT, VPARAMS, APARAMS, KV(__VA_ARGS__))
/// Main logging functionality.
/// vparam_count/vparams have highest precedence (they are user overrides)
/// haven't decided yet if aparams are necessary at all,

static _noinline void _gx_log_inner(int vparam_count, va_list *vparams, gx_log_val aparams[], int argc, ...)
{
    // - loop through each param
    //   - populate array of std-keys, giving string
    //   - populate fixed size array (with incrementing pointer) of adhoc-keys
    //
}


typedef struct gx_logentry_constants {
    char      host     [1024];
    char      version  [256];
    char      pid      [16];
    char      ppid     [16];
    char      utc_iso8601_time[64];  ///< Update with its own timer
} gx_log_semiconstant;

typedef struct gx_logentry_common {
    char      cputick  [64];
    char     *tag;
    char     *msg;
    char      report   [1024];

} gx_log_common;





#endif

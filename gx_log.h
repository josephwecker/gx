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
     Primary purposes for logging (with some overlap):
       - Product resource usage & performance analysis
       - Product usage analysis
       - State communications & auditing (retroactive investigation)
       - Defect containment/identification/investigation (app & system)
       - Development inspection & investigation


     Common fields for log reports / items. Fields that are automatically
     calculated / included are marked with `*`. Fields that are automatically
     included via some lookup mechanisms are marked with `^`. Scope isn't set
     in stone- any key/value pair can be added / overridden.

     | Field           | Scope  | Description                           | example(s)                    |
     | --------------- | ------ | ------------------------------------- | ----------------------------- |
     | type            | all    | Adhoc namespace for log items         | broadcast_state, system_state |
     | name            | all    | Single identifier for log item        | stream_up, syserr_eacces, ... |
     | brief           | log    | Brief message / description / note    | publishing went live.         |
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

     | severity        | all    | naive severity from app perspective   | warning                       |
     | err_label^      | error  | The constant used for the error       | EFAULT, EACCES, ...           |
     | err_brief^      | error  | Looked up string description of err   | "Permission Denied."          |
     | err_severity^   | error  | intrinsic risk of an error            | critical                      |
     | result          | all    | impact / severity / recovery-action...| session_killed, msg_skipped...|

     | err_depth*      | error  | # in sequence of related errors       | 2                             |
     | err_group*      | error  | guid that associates several reports  | w839u21                       |
     | err_number^     | error  | Depends on error class- usually errno | 17                            |
     | err_family^     | error  | What kind of error                    | syserror, gxerror             |
     | src_expression* | error  | Expression that triggered error       | open(filename, "r+")          |
     | err_stack*      | error  | multiline msg w/ stack trace info     | (nyi)                         |

     | (ad-hoc pairs)  | all    | Misc adhoc key/value pair assoc. data | desired_file=somefile.txt     |


*/



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







// specify formatters for different kinds of reports

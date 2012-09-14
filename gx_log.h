



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

#ifndef _GXE_LOG_STDERR_H
#define _GXE_LOG_STDERR_H
/**
   @file      gxe/log_stderr.h
   @brief     Most basic logger - stderr output, with and without a tty.
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
        verbosity
        terminal-width
        severity
        type




    | src_file       | notice |
    | src_line       | error  |
    | src_function   | error  |
    | src_expression | error  |

    | severity      |
    | ------------- |
    | SEV_EMERGENCY |
    | SEV_ALERT     |
    | SEV_CRITICAL  |
    | SEV_ERROR     |
    | SEV_UNKNOWN   |
    | SEV_WARNING   |
    | SEV_NOTICE    |
    | SEV_STAT      |
    | SEV_INFO      |
    | SEV_DEBUG     |


   @todo Lock stderr, but only on mac

*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

static int log_stderr_verified = 0;
static int log_stderr_tty      = 0;
static int log_stderr_8c       = 0;
static int log_stderr_256c     = 0;

static int _gx_log_maxwidths[K_END_STANDARD] = {0};

static inline int is_fd_open(int fd)
{
    return fcntl(fd, F_GETFL) != 1 || errno != EBADF;
}

static inline int check_stderr()
{
    log_stderr_verified = 1;
    // Verify that stderr is open and see if it is a tty or not. e.g., setup
    if(is_fd_open(STDERR_FILENO)) {
        if( (log_stderr_tty = isatty(STDERR_FILENO)) ) {
            char *term;
            if( (term = getenv("TERM")) ) {
                if(strcasestr(term, "xterm") || strcasestr(term, "color")) {
                    log_stderr_8c = 1;
                    if(strstr(term, "256")) log_stderr_256c = 1;
                }
            }
        }
    } else {
        // Probably daemonized- stderr is not even open. Disable this logger.
        gx_loggers[0].enabled = 0;
        return 1;
    }
    return 0;
}

static struct iovec out_iov[100] = {{NULL,0}};
static int    iov_out_count      = 0;

#define CN           "\e[0m"

#define C0       "\e[38;5;198m" /* emergency */
#define C1       "\e[38;5;197m" /* alert */
#define C2       "\e[38;5;196m" /* critical */
#define C3       "\e[38;5;160m" /* error */
#define C4       "\e[38;5;161m" /* unknown */
#define C5       "\e[38;5;202m" /* warning */
#define C6       "\e[38;5;214m" /* notice */
#define C7       "\e[38;5;118m" /* stat */
#define C8       "\e[38;5;106m" /* info */
#define C9       "\e[38;5;226m" /* debug */
#define CW       "\e[38;5;231m" /* white */
#define C_UNKN        "\e[38;5;27m"
#define C_D           "\e[38;5;239m"
#define C_BG          "\e[48;5;234m"
#define C_REF         "\e[38;5;240m"


static char *sev_labels[] = {
    "emergency", "alert  ", "critical",  "error  ", "unknown", "warning",
    "notice ",   "stat   ", "info   ",   "debug  "
};
static char *sev_highlight[] = {C0, C1, C2, C3, C4, C5, C6, C7, C8, C9};


#define padby(N)           scatn(" ", 1, N)
#define sep()              scatc(C_D " | " C_N)
#define scatn(BUF, LEN, N) if(N > 0){                                          \
    size_t _i;                                                                 \
    for(_i=0; _i<N; _i++) scat(BUF, LEN);                                      \
}
#define scatsdp(KEY, PAD) do{ \
    char *_str = DT(KEY); \
    ssize_t _len = SZ(KEY) - 1; \
    if(_len > 0) { \
        scat(_str, _len); \
        padby(PAD - _len); \
        sep(); \
    } \
} while(0)
#define scatsd(KEY) do{ \
    char *_str = DT(KEY); \
    ssize_t _len = SZ(KEY) - 1; \
    if(_len > 0) { \
        scat(_str, _len); \
        sep(); \
    } \
} while(0)
#define scats(STR)       {char *_str=(STR); scat(_str, strlen(_str));}
#define scatc(CONST_STR) scat(CONST_STR, sizeof(CONST_STR) - 1)
#define scat(BUF,LEN) do {                                                     \
    if((LEN) > 0) {                                                            \
        out_iov[iov_out_count].iov_len  = (LEN);                               \
        out_iov[iov_out_count].iov_base = (typeof(out_iov[0].iov_base))(BUF);  \
        iov_out_count ++;                                                      \
    }                                                                          \
} while(0)

#define DT(KEY)    (msg->msg_tab[KEY].val_data_base)
#define SZ(KEY)    ((size_t)(msg->msg_tab[KEY].val_data_size))
#define ISSET(KEY) (SZ(KEY) > 0)

static inline void log_stderr(gx_severity severity, kv_msg_iov *msg)
{
    ssize_t len, len2, pad, i;
    if(rare(!log_stderr_verified) && check_stderr()) return;
    memset(out_iov, 0, sizeof(out_iov));
    iov_out_count = 0;

    // Time + cpu-ticks
    scat   (DT(K_sys_time) + 11, SZ(K_sys_time ) - 13);
    scatc  (C_D ":" C_N);
    len =  7 - SZ(K_sys_ticks);
    scat   (DT(K_sys_ticks) - len, len2 = SZ(K_sys_ticks) - 1 + len);
    padby  (6 - len2);
    sep    ();

    // Severity
    scats  (sev_highlight[severity]);
    scats  (sev_labels[severity]);
    sep    ();

    // Groupings
    if(rare(ISSET(K_err_group))) {
        scatc(C_D "[" C_N ); scats(DT(K_err_group)); scatc(C_D ":" C_N);
        scats(DT(K_err_depth)); scatc(C_D "] " C_N);
    }

    // Error info
    scatsdp(K_err_label,10);
    scatc(C_D);
    scat (DT(K_err_severity), SZ(K_err_severity));
    scatc(": " C_N);
    scatsd (K_err_msg);

    // User Message
    scatsd (K_msg);
    scat   ("\n",1);


    //fprintf(stderr, "OK, got this far. %d\n", iov_out_count);
    writev(STDERR_FILENO, out_iov, iov_out_count);
}

#undef DT
#undef SZ
#undef ISSET


#endif

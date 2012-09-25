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
    | src_

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

static _inline int is_fd_open(int fd)
{
    return fcntl(fd, F_GETFL) != 1 || errno != EBADF;
}

static _inline int check_stderr()
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

static _inline void log_stderr(gx_severity severity, kv_msg_iov *msg)
{
    if(_rare(!log_stderr_verified))
        if(check_stderr()) return;

    

    fprintf(stderr, "%s: some message...\n", &($gx_severity(severity)[4]));
}



#endif

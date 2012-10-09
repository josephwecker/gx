#ifndef _GX_MQUEUE_H
#define _GX_MQUEUE_H

/**
   @file      gx_mqueue.h
   @brief     Some extensions to posix message queues (like scatter io) and SysV fallback for OSX
   @author    Joseph A Wecker <joseph.wecker@gmail.com>
   @author    Achille Roussel <achille.roussel@gmail.com>
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

*/

#include "gx.h"
#include <fcntl.h>
#ifdef __LINUX__
# include <mqueue.h>
#else
# include <unistd.h>
# include <string.h>
# include <stdio.h>
#endif

// The BSD implementation uses a named pipe for now. In order to implement a
// similar message boundary system as the one provided by the Linux message
// queues GX writes the size of the message before the message body.

#ifndef GX_MQUEUE_MSG_COUNT
#define GX_MQUEUE_MSG_COUNT 10
#endif

#ifndef GX_MQUEUE_MSG_SIZE
#define GX_MQUEUE_MSG_SIZE 8192
#endif

// ???
static char _gx_msg_stage[8192] = {0};

static int gx_mq_open(const char *name, int oflag, int mode) {
    int mqfd;

#if __LINUX__
    struct mq_attr attr;

    if(oflags & O_RDWR) {
        errno = EINVAL;
        return -1;
    }

    attr.mq_flags   = 0;
    attr.mq_maxmsg  = GX_MQUEUE_MSG_COUNT;
    attr.mq_maxsize = GX_MQUEUE_MSG_SIZE;
    attr.mq_curmsg  = 0;
    _ (mqfd = mq_open(name, oflags, mode, &attr)) _raise(-1);

#else
    char path[256] = { 0 };

    // Mirrors the Linux behavior, if the name format is invalid return ENOENT.
    if((name[0] != '/') || (name[1] == '\0') || (strchr(name, '/') != NULL)) {
        errno = ENOENT;
        return -1;
    }

    snprintf(path, "/tmp%s", name);

    if(oflags & O_CREAT) {
        _ (mkfifo(path, mode)) _raise(-1);
    }

    _ (mqfd = open(path, oflags & ~(O_CREAT), mode)) _raise(-1);

#endif
    return mqfd;
}

static int gx_mq_recv(int mqfd, void *buffer, int nbytes, unsigned *prio) {
    int n;

#if __LINUX__
    _ (n = mq_receive(mqfd, (char *)buffer, nbytes, prio)) _raise(-1);

#else
    int a = 0;
    int c = 0;

    (void)prio; // Priorities aren't supported on BSD systems.

    n = 0;
    do {
        _ (c = read(mqfd, ((char *)&a) + n, sizeof(a) - n)) {
	    if(errno == EINTR) {
	        continue;
	    }

	    if(errno != EAGAIN) {
	        _raise(-1);
	    } else if(n == 0) {
	        // We couldn't read any data yet, the pipe is probably empty,
	        // simply report EAGAIN to the caller.
	        return -1;
	    } else {
	        continue;
	    }
        }
        n += c;
    } while(n != sizeof(a));

    n = 0;
    do {
        _ (c = read(mqfd, ((char *)buffer) + n, a - n)) {
	    if(errno == EINTR) {
	        continue;
	    }

	    if(errno != EAGAIN) {
	        _raise(-1);
	    } else {
	        // No data were available yet but we're expecting 'a' bytes so
	        // we'll retry again because more data should be available soon.
	        continue;
	    }
        }
        n += c;
    } while(n != a);

#endif
    return n;
}

static int gx_mq_send(int mqfd, const void *buffer, int nbytes, unsigned prio) {
    int n;

#if __LINUX__
    _ (n = mq_send(mqfd, (const char *)buffer, nbytes, prio)) _raise(-1);

#else
    int c;

    // We build a the message which is intended to be sent to the pipe, this is
    // a little stack and time consuming but makes the sending operation easier,
    // it could be optimized in the future.
    char msg[GX_MQUEUE_MSG_SIZE + sizeof(nbytes)];
    memmove(msg, &nbytes, sizeof(nbytes));
    memmove(msg + sizeof(nbytes), buffer, nbytes);

    (void)prio; // Priorities aren't supported on BSD systems.

    nbytes += sizeof(nbytes);
    do {
      _ (c = write(mqfd, msg + n, nbytes - n)) {
	  if(errno == EINTR) {
	      continue;
	  }

	  if((errno == EINTR) && (n == 0)) {
	      // No data could be sent yet, we return -1 and errno is set to
	      // EAGAIN so if the file descriptor was associated with a kqueue
	      // object it will be notified once more data can be sent to the
	      // pipe.
	      return -1;
	  }
      }
      n += c;
    } while(n != nbytes);

#endif
    return n;
}

static int gx_mq_close(int mqfd) {
#if __LINUX__
    _ (mq_close(mqfd)) _raise(-1);
#else
    _ (close(mqfd)) _raise(-1);
#endif
    return 0;
}

static int gx_mq_unlink(const char *name) {
#if __LINUX__
    _ (mq_unlink(name)) _raise(-1);
#else
    char path[256] = { 0 };

    if((name[0] != '/') || (name[1] == '\0') || (strchr(name, '/') != NULL)) {
        errno = ENOENT;
        return -1;
    }

    snprintf(path, sizeof(path), "/tmp%s", name);
    _ (unlink(path)) _raise(-1);
#endif
    return 0;
}

#endif

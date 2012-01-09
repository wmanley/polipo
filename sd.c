/*
Copyright (c) 2011 YouView TV Ltd. <william.manley@youview.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "polipo.h"
#include <errno.h>

#if defined(__linux__)
static int using_linux = 1;
#else
static int using_linux = 0;
#endif

FdEventHandlerPtr
create_listener_sd(int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
                   void *data)
{
    int done;
    int rc;
    int fd = SD_LISTEN_FDS_START;
    int mask;

    /* Some operating systems define but don't support SO_REUSEADDR to check
       that listen has been called.  Notably this includes Mac OS X.  We're
       also not interested in checking that the family is AF_INET because a
       private http cache may actually want to listen on an AF_UNIX socket. */
    rc = sd_is_socket(fd, 0, SOCK_STREAM, using_linux ? 1 : -1);
    if(rc <= 0) {
        do_log_error(L_ERROR, errno, "Passed file descriptor is not a valid socket");
        done = (*handler)(-errno, NULL, NULL);
        assert(done);
        errno = rc < 0 ? -rc : ENOTSOCK;
        goto fail;
    }

    mask = fcntl(fd, F_GETFL, 0);
    if (mask < 0 || fcntl(fd, F_SETFL, mask | O_NONBLOCK) < 0) {
        do_log_error(L_ERROR, errno, "Failed to set socket in non-blocking mode");
        errno = EBADF;
        goto fail;
    }

    return schedule_accept(fd, handler, data);
fail:
    CLOSE(fd);
    return NULL;
};

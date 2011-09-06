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

FdEventHandlerPtr
create_listener_sd(int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
                   void *data)
{
    int done;
    int one;
    int rc;
    int fd = SD_LISTEN_FDS_START;
    int mask;

    rc = sd_is_socket(fd, AF_INET, SOCK_STREAM, 1);
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

    socklen_t optlen = sizeof(one);
    rc = getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, &optlen);
    if(rc < 0) do_log_error(L_WARN, errno, "Failed to check SO_REUSEADDR on socket");
    if(one != 1) do_log_error(L_WARN, EINVAL, "Socket option SO_REUSEADDR is not set");

    return schedule_accept(fd, handler, data);
fail:
    CLOSE(fd);
    return NULL;
};

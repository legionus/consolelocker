/*
  Copyright (C) 2018  Alexey Gladkov <legion@altlinux.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "xmalloc.h"
#include "sockets.h"
#include "communication.h"

struct cmd_resp {
	int status;
	ssize_t msglen;
};

long int __attribute__((format(printf, 3, 4)))
send_command_response(int conn, int retcode, const char *fmt, ...)
{
	va_list ap;

	long int rc        = 0;
	struct msghdr msg  = {};
	struct iovec iov   = {};
	struct cmd_resp rs = {};

	rs.status = retcode;

	if (fmt && *fmt) {
		va_start(ap, fmt);

		if ((rs.msglen = vsnprintf(NULL, 0, fmt, ap)) < 0) {
			err("unable to calculate message size");
			va_end(ap);
			return -1;
		}

		rs.msglen++;
		va_end(ap);
	}

	iov.iov_base = &rs;
	iov.iov_len  = sizeof(rs);

	msg.msg_iov    = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	if (TEMP_FAILURE_RETRY(sendmsg(conn, &msg, MSG_NOSIGNAL)) < 0) {
		/* The client left without waiting for an answer */
		if (errno == EPIPE)
			return 0;

		err("sendmsg: %m");
		return -1;
	}

	if (!rs.msglen)
		return 0;

	iov.iov_base = xmalloc((size_t)rs.msglen);
	iov.iov_len  = (size_t)rs.msglen;

	va_start(ap, fmt);
	vsnprintf(iov.iov_base, iov.iov_len, fmt, ap);
	va_end(ap);

	errno = 0;
	if (TEMP_FAILURE_RETRY(sendmsg(conn, &msg, MSG_NOSIGNAL)) < 0 && errno != EPIPE) {
		err("sendmsg: %m");
		rc = -1;
	}

	free(iov.iov_base);

	return rc;
}

int recv_command_response(int conn, cmd_status_t *retcode, char **m)
{
	struct cmd_resp rs = {};

	if (xrecvmsg(conn, &rs, sizeof(rs)) < 0)
		return -1;

	if (retcode)
		*retcode = rs.status;

	if (!m || rs.msglen <= 0)
		return 0;

	char *x = xcalloc(1UL, (size_t)rs.msglen);
	if (xrecvmsg(conn, x, (uint64_t)rs.msglen) < 0)
		return -1;

	*m = x;
	return 0;
}

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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "sockets.h"

int unix_listen(const char *dir_name, const char *file_name)
{
	struct sockaddr_un sun;

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	snprintf(sun.sun_path, sizeof sun.sun_path, "%s/%s", dir_name, file_name);

	if (unlink(sun.sun_path) && errno != ENOENT) {
		err("unlink: %s: %m", sun.sun_path);
		return -1;
	}

	if (mkdir(dir_name, 0700) && errno != EEXIST) {
		err("mkdir: %s: %m", dir_name);
		return -1;
	}

	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
		err("socket AF_UNIX: %m");
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&sun, (socklen_t)sizeof sun)) {
		err("bind: %s: %m", sun.sun_path);
		(void)close(fd);
		return -1;
	}

	if (listen(fd, 16) < 0) {
		err("listen: %s: %m", sun.sun_path);
		(void)close(fd);
		return -1;
	}

	return fd;
}

int unix_connect(const char *dir_name, const char *file_name)
{
	struct sockaddr_un sun;
	int conn;

	if ((conn = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
		err("socket AF_UNIX: %m");
		return -1;
	}

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	snprintf(sun.sun_path, sizeof sun.sun_path, "%s/%s", dir_name, file_name);

	if (connect(conn, (const struct sockaddr *)&sun, sizeof(struct sockaddr_un)) < 0) {
		err("connect: %s: %m", sun.sun_path);
		return -1;
	}

	return conn;
}

int get_peercred(int fd, pid_t *pid, uid_t *uid, gid_t *gid)
{
	struct ucred uc;
	socklen_t len = sizeof(struct ucred);

	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &uc, &len) < 0) {
		err("getsockopt(SO_PEERCRED): %m");
		return -1;
	}

	if (pid)
		*pid = uc.pid;

	if (uid)
		*uid = uc.uid;

	if (gid)
		*gid = uc.gid;

	return 0;
}

int set_recv_timeout(int fd, int secs)
{
	struct timeval tv = {};

	tv.tv_sec = secs;

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0) {
		err("setsockopt(SO_RCVTIMEO): %m");
		return -1;
	}
	return 0;
}

int xsendmsg(int conn, void *data, uint64_t len)
{
	ssize_t n;

	struct msghdr msg = {};
	struct iovec iov  = {};

	iov.iov_base = data;
	iov.iov_len  = len;

	msg.msg_iov    = &iov;
	msg.msg_iovlen = 1;

	if ((n = TEMP_FAILURE_RETRY(sendmsg(conn, &msg, 0))) != (ssize_t)len) {
		if (n < 0)
			err("recvmsg: %m");
		else if (n)
			err("recvmsg: expected size %u, got %u", (unsigned)len, (unsigned)n);
		else
			err("recvmsg: unexpected EOF");
		return -1;
	}

	return 0;
}

int xrecvmsg(int conn, void *data, uint64_t len)
{
	ssize_t n;

	struct msghdr msg = {};
	struct iovec iov  = {};

	iov.iov_base = data;
	iov.iov_len  = len;

	msg.msg_name   = NULL;
	msg.msg_iov    = &iov;
	msg.msg_iovlen = 1;

	if ((n = TEMP_FAILURE_RETRY(recvmsg(conn, &msg, MSG_WAITALL))) != (ssize_t)len) {
		if (n < 0)
			err("recvmsg: %m");
		else if (n)
			err("recvmsg: expected size %u, got %u", (unsigned)len, (unsigned)n);
		else
			err("recvmsg: unexpected EOF");
		return -1;
	}

	return 0;
}

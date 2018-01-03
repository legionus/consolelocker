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
#include <sys/epoll.h>

#include <string.h>
#include <unistd.h>

#include "epoll.h"
#include "logging.h"

int epollin_add(int fd_ep, int fd)
{
	struct epoll_event ev;

	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events  = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd, &ev) < 0) {
		err("epoll_ctl: %m");
		return -1;
	}

	return 0;
}

void epollin_remove(int fd_ep, int fd)
{
	if (fd < 0)
		return;
	epoll_ctl(fd_ep, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

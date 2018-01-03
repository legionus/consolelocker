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
#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include <stdint.h>

typedef enum {
	CMD_NONE = 0,
	CMD_LOCK,
} cmd_t;

typedef enum {
	CMD_STATUS_DONE = 0,
	CMD_STATUS_FAILED,
} cmd_status_t;

struct cmd {
	cmd_t type;
	uint64_t datalen;
};

long int send_command_response(int conn, int retcode, const char *fmt, ...);
int recv_command_response(int conn, cmd_status_t *retcode, char **m);

#endif /* _COMMUNICATION_H_ */

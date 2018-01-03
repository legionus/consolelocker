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

#include <unistd.h>

#include "logging.h"
#include "sockets.h"
#include "communication.h"

int main(void)
{
	int conn;
	cmd_status_t status;
	char *msg      = NULL;
	struct cmd hdr = {};

	if ((conn = unix_connect(SOCKETDIR, PROJECT)) < 0)
		return EXIT_FAILURE;

	hdr.type    = CMD_LOCK;
	hdr.datalen = 0;

	if (xsendmsg(conn, &hdr, sizeof(hdr)) < 0)
		return -1;

	if (recv_command_response(conn, &status, &msg) < 0) {
		free(msg);
		return -1;
	}

	close(conn);

	if (msg && *msg) {
		err("%s", msg);
		free(msg);
	}

	return status == CMD_STATUS_FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
}

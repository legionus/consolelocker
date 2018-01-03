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
#include <stdio.h>

#include "logging.h"
#include "sysrq.h"

static const char sysrq_path[] = "/proc/sys/kernel/sysrq";

int get_sysrq(void)
{
	FILE *fd;
	int cur_sysrq = 0;

	if (!(fd = fopen(sysrq_path, "r"))) {
		err("fopen: %s: %m", sysrq_path);
		return -1;
	}

	if (fscanf(fd, "%d", &cur_sysrq) != 1) {
		err("fscanf: %s: %m", sysrq_path);
		cur_sysrq = -1;
	}

	if ((fclose(fd)) != 0) {
		err("fclose: %s: %m", sysrq_path);
		cur_sysrq = -1;
	}

	return cur_sysrq;
}

int set_sysrq(int new_sysrq)
{
	FILE *fd;

	if (!(fd = fopen(sysrq_path, "w"))) {
		err("fopen: %s: %m", sysrq_path);
		return -1;
	}

	if (!fprintf(fd, "%d\n", new_sysrq)) {
		err("fprintf: %s: %m", sysrq_path);
		fclose(fd);
		return -1;
	}
	fflush(fd);

	if ((fclose(fd)) != 0) {
		err("fclose: %s: %m", sysrq_path);
		return -1;
	}
	return 1;
}

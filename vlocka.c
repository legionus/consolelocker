/*
  Copyright (C) 2006-2018  Alexey Gladkov <legion@altlinux.org>

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
#include <sys/wait.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <error.h>

static void
my_error_print_progname(void)
{
	fprintf(stderr, "%s: ", program_invocation_short_name);
}

static inline void
clear(void)
{
	puts("\033[H""\033[J");
}

static unsigned int
str2int(const char *value)
{
	char   *p = 0;
	unsigned long n;

	if (!*value)
		error(EXIT_FAILURE, 0, "invalid value: %s", value);

	n = strtoul(value, &p, 10);

	if (!p || *p || n > UINT_MAX)
		error(EXIT_FAILURE, 0, "invalid value: %s", value);

	return (unsigned int) n;
}

int
main(int argc, char ** argv)
{
	int noclear = 0;
	struct passwd *pw = NULL;

	error_print_progname = my_error_print_progname;

	if (argc < 2)
		error(EXIT_FAILURE, 0, "Usage: %s <uid>", program_invocation_short_name);

	if (getuid())
		error(EXIT_FAILURE, 0, "must be root");

	if (strlen(argv[1]) > 0) {
		uid_t uid = str2int(argv[1]);

		pw = getpwuid(uid);
		if (!pw) {
			error(EXIT_SUCCESS, errno, "getpwnam");
			error(EXIT_SUCCESS, 0, "rollback to root");
			noclear = 1;
		}
	}

	if (!pw && !(pw = getpwuid(0)))
		error(EXIT_FAILURE, errno, "getpwnam");

	if (clearenv() < 0)
		error(EXIT_FAILURE, errno, "clearenv");

	if (setenv("USER", pw->pw_name, 1) < 0)
		error(EXIT_FAILURE, errno, "setenv: %s", "USER");

	if (setenv("LOGNAME", pw->pw_name, 1) < 0)
		error(EXIT_FAILURE, errno, "setenv: %s", "LOGNAME");

	if (setenv("HOME", pw->pw_dir, 1) < 0)
		error(EXIT_FAILURE, errno, "setenv: %s", "HOME");

	if (setenv("PATH", "/bin:/usr/bin", 1) < 0)
		error(EXIT_FAILURE, errno, "setenv: %s", "PATH");

	if (setenv("TERM", "linux", 1) < 0)
		error(EXIT_FAILURE, errno, "setenv: %s", "TERM");

	if ((setgid(pw->pw_gid)) < 0)
		error(EXIT_FAILURE, errno, "setgid");

	if ((setuid(pw->pw_uid)) < 0)
		error(EXIT_FAILURE, errno, "setuid");

	if (!noclear)
		clear();

	if ((execl("/usr/bin/vlock", "vlocka", "-a", (char *) NULL)) < 0)
		error(EXIT_FAILURE, errno, "execl");

	clear();

	return EXIT_SUCCESS;
}

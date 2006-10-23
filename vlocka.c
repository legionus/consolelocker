#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <error.h>

extern const char *__progname;

static void
clear(void) {
	int retval;
	pid_t pid;

	if ((pid = fork()) == -1)
		error(EXIT_FAILURE, errno, "fork");

	if (pid == 0) {
		if ((execl("/usr/bin/clear", "clear", (char *) NULL)) == -1)
			error(EXIT_FAILURE, errno, "execl");
	} else {
		if ((retval = wait(NULL)) == -1)
			error(EXIT_FAILURE, errno, "wait");
	}
}

int
main(int argc, char ** argv) {
	const char *user = NULL, *term = NULL;
	struct passwd *pw = NULL;
	
	if (argc < 2)
		error(EXIT_FAILURE, 0, "Usage: %s <username>", __progname);

	user = argv[1];
	term = getenv("TERM");

	if (getuid())
		error(EXIT_FAILURE, 0, "must be root");

	if ((pw = getpwnam(user)) == NULL)
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

	if (setenv("TERM", term, 1) < 0)
		error(EXIT_FAILURE, errno, "setenv: %s", "TERM");

	if ((setgid(pw->pw_gid)) == -1)
		error(EXIT_FAILURE, errno, "setgid");

	if ((setuid(pw->pw_uid)) == -1)
		error(EXIT_FAILURE, errno, "setuid");

	clear();

	if ((execl("/usr/bin/vlock", "vlocka", "-a", (char *) NULL)) == -1)
		error(EXIT_FAILURE, errno, "execl");
}

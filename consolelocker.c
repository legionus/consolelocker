/* -*- mode: Shell-script; tab-width: 8; fill-column: 70; -*- */
/***** BEGIN LICENSE BLOCK ***** 
 * Copyright (C) 2006 Alexey Gladkov <legion@altlinux.org> 
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ***** END LICENSE BLOCK ******/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>

#include "pidfile.h"

#define setprogname(pn)
extern const char *__progname;

enum states {
    STATE_READY,    // program ready for commands.
    STATE_PASSIVE,  // program not listen any commands.
    STATE_LOCKED,   // program was lock console.
};

const char fifo_path[] = "/var/run/consolelocker.fifo";
const char consolelock_path[] = "/var/run/console/console.lock";
const char pidfile_path[] = "/var/run/consolelocker.pid";

volatile pid_t lock_process;
volatile enum states state;
volatile int goto_finish;

const uid_t def_fifo_owner = 0;
const gid_t def_fifo_group = 0;

char *consoleowner = NULL;
uid_t fifo_owner;
gid_t fifo_group;

static void __attribute__ ((noreturn))
print_help(int ret)  {
	printf("Usage: %s [OPTIONS]\n", __progname);
	printf("\n");
	printf("%s is a program to lock sessions on the Linux console\n", __progname);
	printf("and virtual consoles. After startup it open FIFO to read\n");
	printf("events from console user.\n");
	printf("This program is simple wrapper for vlock(1).\n");
	printf("\n");
	printf("Options:\n");
	printf("  -D, --nodaemon   When this option is specified sshd will\n");
	printf("                   not detach and does not become a daemon.\n");
	printf("  -V, --version    print program version and exit.\n");
	printf("  -h, --help       output a brief help message.\n");
	printf("\n");
	exit(ret);
}

static void __attribute__ ((noreturn))
print_version(void) {
        printf("%s version %s\n"
               "\nCopyright (C) 2006  Alexey Gladkov <legion@altlinux.org>\n"
               "\nThis is free software; see the source for copying conditions.\n"
               "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
               "\nWritten by Alexey Gladkov <legion@altlinux.org>\n",
               __progname, PROJECT_VERSION);
        exit(EXIT_SUCCESS);
}

static void
sigchld_handler(int sig __attribute__((__unused__))) {
	int status, retval;

	if ((retval = waitpid(lock_process, &status, WNOHANG|WUNTRACED)) == -1)
		error(EXIT_FAILURE, errno, "waitpid");

	if (retval == 0)
		return;
		
	if (state)
		state = STATE_READY;

	if (lock_process)
		lock_process = -1;
}

static void
exit_handler(int sig __attribute__((__unused__))) {
	goto_finish = 1;
}

static void
readgroup(const char *filename) {
	FILE *fd;
	struct passwd *pw = NULL;
	size_t len = 0;
	
	if ((fd = fopen(filename, "r")) == NULL)
		error(EXIT_FAILURE, errno, "fopen");

	if (consoleowner)
    		free(consoleowner);

	if ((getline(&consoleowner, &len, fd) == -1))
		error(EXIT_FAILURE, 0, "getline: unable read username");

	if ((pw = getpwnam(consoleowner)) != NULL) {
		fifo_group = pw->pw_gid;
	} else
		error(EXIT_FAILURE, errno, "getpwnam");

	if ((fclose(fd)) != 0)
		error(EXIT_FAILURE, errno, "fclose");
}

static pid_t
authorize(void) {
	pid_t pid;

	if ((pid = fork()) == -1)
		error(EXIT_FAILURE, errno, "fork");
	if (pid == 0) {
		if ((execl("/usr/bin/openvt", 
			"openvt", "-s", "-l", "-w", "--", 
				"su", "-l", "-c", "clear; vlock -a", consoleowner, 
			(char *) NULL)) == -1)
			error(EXIT_FAILURE, errno, "execl");
	}
	return pid;
}

static int
exist(const char *filename) {
	if (access(filename, R_OK) == -1) {
		if (errno == ENOENT)
			return 0;
		error(EXIT_FAILURE, errno, "access");
	}
	return 1;
}

static int
daemonize(void) {
	int reopen_fifo = 1, fd = -1, rc = EXIT_SUCCESS;
	fd_set rfds;
	struct timeval tv;

	/* Create pidfile */
	if ((write_pid(pidfile_path)) == 0)
		error(EXIT_FAILURE, 0, "write_pid failed");

	/* Register SIGCHLD handler */
	signal(SIGCHLD, sigchld_handler);

	/* Register SIGTERM handler */
	signal(SIGTERM, exit_handler);

	/* Block other signals */
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	goto_finish = 0;
	lock_process = -1;
	state = STATE_PASSIVE;

#define xassert_loop(x, y) if (x == -1) { \
	error(0, errno, y); \
	rc = EXIT_FAILURE; \
	break; \
}

	while (goto_finish == 0) {
		char c;
		int retval;

		if (!exist(consolelock_path)) {
			if (state == STATE_LOCKED && lock_process > 0) {
#if 0
				/* Kill vlock process, if nobody logged in. */
				xassert_loop ((kill( -lock_process, SIGKILL)), "kill");
#endif
				lock_process = -1;
			}
			
			xassert_loop((chown(fifo_path, def_fifo_owner, def_fifo_group)), "chown");
			state = STATE_PASSIVE;
		} else if (state == STATE_PASSIVE) {
			readgroup(consolelock_path);
			
			xassert_loop((chown(fifo_path, fifo_owner, fifo_group)), "chown");
			state = STATE_READY;
		}

		/* Open the FIFO */
		if (reopen_fifo == 1) {
			xassert_loop((fd != -1 && (close(fd)) == -1), "close");
			xassert_loop((fd = open(fifo_path, O_RDONLY | O_NONBLOCK)), "open");
			reopen_fifo = 0;
		}

		/* Watch to see when it has input. */
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
      
		/* Wait up to three seconds. */
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		
		retval = -1;
		errno = EINTR;
		while (retval < 0 && errno == EINTR)
			retval = select((fd + 1), &rfds, NULL, NULL, &tv);
		
		xassert_loop(retval, "select");
		
		if (retval == 0)
			continue;
	
		while ((retval = read(fd, &c, sizeof(c))) == 1) {
			if (state != STATE_READY)
				continue;
			state = STATE_LOCKED;
			lock_process = authorize();
		}
		xassert_loop(retval, "read");
		reopen_fifo = 1;
	}

	if (consoleowner)
    		free(consoleowner);

	if ((chown(fifo_path, def_fifo_owner, def_fifo_group)) == -1)
		error(EXIT_FAILURE, errno, "chown");

	if ((remove_pid(pidfile_path)) == -1)
		error(EXIT_FAILURE, errno, "remove_pid");

	return rc;
}

int
main(int argc, char ** argv) {
	int c, nodaemon = 0;
	mode_t prev_umask;

	static struct option long_options[] = {
		{"nodaemon", 0, 0, 'D'},
		{"help", 0, 0, 'h'},
		{"version", 0, 0, 'V'},
		{0, 0, 0, 0}
	};

	setprogname(argv[0]);
	while (1) {
		int option_index = 0;
		c = getopt_long (argc, argv, "DhV", long_options, &option_index);
		if (c == -1)
    			break;
    
		switch (c) {
			case 'D': nodaemon = 1; break;
			case 'V': print_version(); break;
			default:
			case 'h': print_help(EXIT_SUCCESS); break;
		}
	}
	
	if (getuid())
		error(EXIT_FAILURE, 0, "must be root");

	if (check_pid(pidfile_path))
		error(EXIT_FAILURE, 0, "pidfile already exist");

	/* Remove old FIFO */
	if (exist(fifo_path) && (unlink(fifo_path)) == -1)
		error(EXIT_FAILURE, errno, "unlink");

	/* Create the FIFO */
	prev_umask = umask(046);
	if ((mkfifo(fifo_path, 0620)) == -1)
		error(EXIT_FAILURE, errno, "mkfifo");
	umask(prev_umask);

	/* Change owner/group to default values */
	if ((chown(fifo_path, def_fifo_owner, def_fifo_group)) == -1)
		error(EXIT_FAILURE, errno, "chown");

	if (nodaemon == 0 && daemon(0, 0) == -1)
		error(EXIT_FAILURE, errno, "daemon");
	
	return daemonize();
}
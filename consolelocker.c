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

#include <sys/param.h> /* MAXPATHLEN */
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <error.h>

#include "logging.h"
#include "pidfile.h"
#include "epoll.h"
#include "sockets.h"
#include "sysrq.h"
#include "communication.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum states {
	STATE_READY,  // program ready for commands.
	STATE_LOCKED, // program was lock console.
};

static const char pidfile_path[] = "/var/run/consolelocker.pid";
static int finish_server         = 0;
static int sysrq                 = 0;

static enum states state;
static pid_t vlock_pid;

static struct option long_options[] = {
	{ "loglevel", required_argument, 0, 'l' },
	{ "pidfile", required_argument, 0, 'p' },
	{ "group", required_argument, 0, 'g' },
	{ "daemonize", no_argument, 0, 'f' },
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },
	{ 0, 0, 0, 0 }
};

static void __attribute__((noreturn))
print_help(int ret)
{
	printf("Usage: %s [OPTIONS]\n\n"
	       "This is a program to lock sessions on the Linux console\n"
	       "and virtual consoles. After startup it open FIFO to read\n"
	       "events from console user.\n"
	       "This program is simple wrapper for vlock(1).\n\n"
	       "Options:\n"
	       "  -g, --group=NAME     make socket group writable\n"
	       "  -p, --pidfile=FILE   pidfile location;\n"
	       "  -l, --loglevel=LVL   set logging level;\n"
	       "  -f, --foreground     stay in the foreground;\n"
	       "  -V, --version        print program version and exit.\n"
	       "  -h, --help           output a brief help message.\n\n",
	       program_invocation_short_name);
	exit(ret);
}

static void __attribute__((noreturn))
print_version(void)
{
	printf("%s version %s\n\n"
	       "Copyright (C) 2006-2018  Alexey Gladkov <legion@altlinux.org>\n\n"
	       "This is free software; see the source for copying conditions.\n"
	       "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
	       "Written by Alexey Gladkov <legion@altlinux.org>\n",
	       program_invocation_short_name, PROJECT_VERSION);
	exit(EXIT_SUCCESS);
}

static void
my_error_print_progname(void)
{
	fprintf(stderr, "%s: ", program_invocation_short_name);
}

static int
handle_signal(uint32_t signo)
{
	pid_t pid;
	int status;

	switch (signo) {
		case SIGINT:
		case SIGTERM:
			finish_server = 1;
			break;

		case SIGCHLD:
			if ((pid = waitpid(-1, &status, 0)) < 0) {
				err("waitpid: %m");
				return -1;
			}

			if (pid == vlock_pid) {
				state     = STATE_READY;
				vlock_pid = 0;
				set_sysrq(sysrq);
				info("console unlocked");
			}

			break;

		case SIGHUP:
			break;
	}
	return 0;
}

static pid_t
consolelock(uid_t uid)
{
	char s_uid[256];
	pid_t pid;

	if ((pid = fork()) != 0) {
		if (pid < 0) {
			err("fork: %m");
			return -1;
		}
		return pid;
	}

	snprintf(s_uid, sizeof(s_uid), "%d", uid);

	info("console is locked by the %s", s_uid);

	if ((execl("/bin/openvt", "openvt", "-s", "-l", "-w", "--", VLOCK_WRAPPER, s_uid, (char *) NULL)) < 0)
		fatal("execl: %m");

	return -1;
}

static int
process_request(int conn)
{
	int rc         = 0;
	struct cmd hdr = {};

	uid_t uid;
	gid_t gid;

	if (get_peercred(conn, NULL, &uid, &gid) < 0)
		return -1;

	if (xrecvmsg(conn, &hdr, sizeof(hdr)) < 0)
		return -1;

	switch (hdr.type) {
		case CMD_LOCK:
			if (state == STATE_LOCKED) {
				dbg("console already locked");
				send_command_response(conn, CMD_STATUS_FAILED, "console already locked");
				break;
			}
			dbg("request to lock console from uid=%d", uid);
			state = STATE_LOCKED;

			sysrq     = get_sysrq();
			vlock_pid = consolelock(uid);

			send_command_response(conn, CMD_STATUS_DONE, NULL);
			break;
		default:
			err("unknown command");
			send_command_response(conn, CMD_STATUS_FAILED, "command unknown");
			rc = -1;
	}

	return rc;
}

int main(int argc, char **argv)
{
	sigset_t mask;
	mode_t m;

	int rc = EXIT_FAILURE;
	int i, daemonize = 1;
	int loglevel = -1;

	const char *pidfile = NULL;
	const char *grpname = NULL;

	int fd_ep     = -1;
	int fd_signal = -1;
	int fd_conn   = -1;

	error_print_progname = my_error_print_progname;

	while ((i = getopt_long(argc, argv, "fhVg:p:l:", long_options, NULL)) != -1) {
		switch (i) {
			case 'g':
				grpname = optarg;
				break;
			case 'p':
				pidfile = optarg;
				break;
			case 'l':
				loglevel = logging_level(optarg);
				break;
			case 'f':
				daemonize = 0;
				break;
			case 'V':
				print_version();
				break;
			default:
			case 'h':
				print_help(EXIT_SUCCESS);
				break;
		}
	}

	if (!pidfile)
		pidfile = pidfile_path;

	if (loglevel < 0)
		loglevel = logging_level("info");

	umask(022);

	if (pidfile && check_pid(pidfile))
		error(EXIT_FAILURE, 0, "already running");

	if (daemonize && daemon(0, 0) < 0)
		error(EXIT_FAILURE, errno, "daemon");

	logging_init(loglevel, !daemonize);

	if (pidfile && !write_pid(pidfile))
		goto exit;

	m = umask(017);

	if ((fd_conn = unix_listen(SOCKETDIR, PROJECT)) < 0)
		goto exit;

	dbg("listen unix socket: %s/%s", SOCKETDIR, PROJECT);

	if (grpname != NULL) {
		struct group *gr;
		char socketpath[MAXPATHLEN];

		snprintf(socketpath, sizeof(socketpath), "%s/%s", SOCKETDIR, PROJECT);

		if (!(gr = getgrnam(grpname))) {
			err("getgrnam: %m");
			goto exit;
		}

		if (chown(socketpath, (uid_t)-1, gr->gr_gid) < 0) {
			err("chown: %s: %m", socketpath);
			goto exit;
		}

		dbg("unix socket group: %s/%s: %s(%d)", SOCKETDIR, PROJECT, gr->gr_name, gr->gr_gid);
	}

	umask(m);

	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	sigdelset(&mask, SIGABRT);
	sigdelset(&mask, SIGSEGV);

	if ((fd_ep = epoll_create1(EPOLL_CLOEXEC)) < 0) {
		err("epoll_create1: %m");
		goto exit;
	}

	if ((fd_signal = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC)) < 0) {
		err("signalfd: %m");
		goto exit;
	}

	if (epollin_add(fd_ep, fd_signal) < 0 || epollin_add(fd_ep, fd_conn) < 0)
		goto exit;

	state = STATE_READY;

	dbg("waiting for connections");
	while (!finish_server) {
		struct epoll_event ev[42];
		int fdcount;
		ssize_t size;

		errno = 0;
		if ((fdcount = epoll_wait(fd_ep, ev, ARRAY_SIZE(ev), 1000)) < 0) {
			if (errno == EINTR)
				continue;
			err("epoll_wait: %m");
			break;
		}

		for (i = 0; i < fdcount; i++) {
			if (!(ev[i].events & EPOLLIN)) {
				continue;

			} else if (ev[i].data.fd == fd_signal) {
				struct signalfd_siginfo fdsi;

				size = TEMP_FAILURE_RETRY(read(fd_signal, &fdsi, sizeof(struct signalfd_siginfo)));
				if (size != sizeof(struct signalfd_siginfo)) {
					err("unable to read signal info");
					continue;
				}

				dbg("got signal: %d", fdsi.ssi_signo);
				handle_signal(fdsi.ssi_signo);

			} else if (ev[i].data.fd == fd_conn) {
				int conn;

				dbg("got new connection");
				if ((conn = accept4(fd_conn, NULL, 0, SOCK_CLOEXEC)) < 0) {
					err("accept4: %m");
					continue;
				}

				if (set_recv_timeout(conn, 3) < 0) {
					close(conn);
					continue;
				}

				process_request(conn);

				close(conn);
			}
		}
	}
	if (finish_server)
		rc = EXIT_SUCCESS;
exit:
	dbg("stopped listening rc=%d", rc);
	if (fd_ep >= 0) {
		epollin_remove(fd_ep, fd_signal);
		epollin_remove(fd_ep, fd_conn);
		close(fd_ep);
	}

	if (pidfile)
		remove_pid(pidfile);

	logging_close();
	return rc;
}

PROJECT = consolelocker
VERSION = 0.0.1

MAN8PAGES = consolelocker.8

bindir          = /usr/bin
sbindir         = /usr/sbin
helperdir       = /usr/libexec/$(PROJECT)
socketdir       = /var/run
sysconfdir      = /etc
initdir         = /etc/rc.d/init.d
systemd_unitdir = /lib/systemd/system

DESTDIR =

CC        = gcc
MKDIR_P   = mkdir -p
INSTALL   = install
HELP2MAN8 = help2man -N -s8

WARNINGS = -Wall -Wextra -W -Wshadow -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wconversion -Waggregate-return -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn \
	-Wmissing-format-attribute -Wredundant-decls -Wdisabled-optimization 

CPPFLAGS = -std=gnu99 $(WARNINGS) -D_GNU_SOURCE \
	-DPROJECT=\"$(PROJECT)\" -DPROJECT_VERSION=\"$(VERSION)\" \
	-DVLOCK_WRAPPER=\"$(helperdir)/vlocka\" \
	-DSOCKETDIR=\"$(socketdir)\"

CFLAGS = -I. -pipe -O2

HEADERS = communication.h epoll.h logging.h pidfile.h sockets.h sysrq.h xmalloc.h

client_SRC = consolelock.c communication.c sockets.c logging.c xmalloc.c
client_OBJ = $(client_SRC:.c=.o)
client_DEP = $(client_SRC:.c=.d)

server_SRC = pidfile.c consolelocker.c communication.c sockets.c logging.c epoll.c sysrq.c xmalloc.c
server_OBJ = $(server_SRC:.c=.o)
server_DEP = $(server_SRC:.c=.d)

vlocka_SRC = vlocka.c
vlocka_OBJ = $(vlocka_SRC:.c=.o)
vlocka_DEP = $(vlocka_SRC:.c=.d)

DEP = $(client_DEP) $(server_DEP) $(vlocka_DEP)

TARGETS = consolelock consolelocker vlocka $(MAN8PAGES)
WRAPPERS = vlocka consolelock.pam-console

.PHONY:	all install clean indent

all: $(TARGETS)

consolelock: $(client_OBJ)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

consolelocker: $(server_OBJ)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

vlocka: $(vlocka_OBJ)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

install: all
	$(MKDIR_P) -m755 $(DESTDIR)$(sysconfdir)/cron.d
	$(INSTALL) -p -m644 consolelocker.cronjob $(DESTDIR)$(sysconfdir)/cron.d/consolelocker
	$(MKDIR_P) -m755 $(DESTDIR)$(sysconfdir)/sysconfig
	$(INSTALL) -p -m644 consolelocker.sysconf $(DESTDIR)$(sysconfdir)/sysconfig/consolelocker
	$(MKDIR_P) -m755 $(DESTDIR)$(initdir)
	$(INSTALL) -p -m755 consolelocker.sysvinit $(DESTDIR)$(initdir)/consolelocker
	$(MKDIR_P) -m755 $(DESTDIR)$(systemd_unitdir)
	$(INSTALL) -p -m644 consolelocker.service $(DESTDIR)$(systemd_unitdir)/consolelocker.service
	$(MKDIR_P) -m755 $(DESTDIR)$(bindir)
	$(INSTALL) -p -m755 consolelock $(DESTDIR)$(bindir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(sbindir)
	$(INSTALL) -p -m700 consolelocker $(DESTDIR)$(sbindir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(helperdir)
	$(INSTALL) -p -m700 $(WRAPPERS) $(DESTDIR)$(helperdir)/

clean:
	$(RM) $(TARGETS) $(DEP) $(client_OBJ) $(server_OBJ) $(vlocka_OBJ) core *~

indent:
	clang-format -style=file -i $(HEADERS) $(client_SRC) $(server_SRC)

%.8: % %.8.inc Makefile
	$(HELP2MAN8) -i $@.inc ./$< >$@

ifneq ($(MAKECMDGOALS),indent)
ifneq ($(MAKECMDGOALS),clean)

%.d:	%.c Makefile
	@echo Making dependences for $<
	@$(CC) -MM $(CPPFLAGS) $< |sed -e 's,\($*\)\.o[ :]*,\1.o $@: Makefile ,g' >$@

ifneq ($(DEP),)
-include $(DEP)
endif

endif # clean
endif # indent

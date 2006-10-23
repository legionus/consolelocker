PROJECT = consolelocker
VERSION = 0.0.1
WRAPPERS = vlocka
MAN8PAGES = $(PROJECT).8
TARGETS = $(PROJECT) $(WRAPPERS) $(MAN8PAGES)

sbindir = /usr/sbin
helperdir = /usr/libexec/consolelocker
DESTDIR =

CC = gcc
MKDIR_P = mkdir -p
INSTALL = install
HELP2MAN8 = help2man -N -s8

WARNINGS = -Wall -Wextra -Werror -W -Wshadow -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wconversion -Waggregate-return -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn \
	-Wmissing-format-attribute -Wredundant-decls -Wdisabled-optimization 

CPPFLAGS = -std=gnu99 $(WARNINGS) -D_GNU_SOURCE -DPROJECT_VERSION=\"$(VERSION)\" \
	-DVLOCK_WRAPPER=\"$(helperdir)/vlocka\"

CFLAGS = -I. -pipe -O2

SRC = pidfile.c consolelocker.c
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

.PHONY:	all install clean

all: $(TARGETS)

$(PROJECT): $(OBJ)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

install: all
	$(MKDIR_P) -m755 $(DESTDIR)$(sbindir)
	$(INSTALL) -p -m700 $(PROJECT) $(DESTDIR)$(sbindir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(helperdir)
	$(INSTALL) -p -m700 $(WRAPPERS) $(DESTDIR)$(helperdir)/

clean:
	$(RM) $(TARGETS) $(DEP) $(OBJ) core *~

%.8: % %.8.inc Makefile
	$(HELP2MAN8) -i $@.inc ./$< >$@

ifneq ($(MAKECMDGOALS),clean)
%.d:	%.c Makefile
	@echo Making dependences for $<
	@$(CC) -MM $(CPPFLAGS) $< |sed -e 's,\($*\)\.o[ :]*,\1.o $@: Makefile ,g' >$@
endif

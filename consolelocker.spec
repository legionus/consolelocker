Name: consolelocker
Version: 0.0.1
Release: alt1

Summary: Daemon for lock console terminal and virtual consoles.
License: GPL
Group: Terminals
Packager: Alexey Gladkov <legion@altlinux.org>

Source: %name-%version.tar

Requires: vlock console-vt-tools

# Automatically added by buildreq on Fri Oct 20 2006 (-bi)
BuildRequires: help2man

%description
This package contains a daemon for lock console terminal and virtual consoles.

%prep
%setup -q

%build
%make_build

%install
%make_install install DESTDIR=%buildroot
%__mkdir_p %buildroot/%_bindir
%__install -D -m755 consolelock %buildroot/%_bindir/
%__install -D -m644 %name.cronjob %buildroot/%_sysconfdir/cron.d/%name
%__install -D -m755 %name.service %buildroot/%_initrddir/%name

%files
%_bindir/*
%_sbindir/*
%_initrddir/*
%config(noreplace) %_sysconfdir/cron.d/%name

%changelog
* Tue Oct 10 2006 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt1
- Initial revision.

Name: consolelocker
Version: 0.0.2
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

%post
if [ $1 -eq 1 ]; then
	/sbin/chkconfig --add %name
fi

%preun
if [ $1 -eq 0 ]; then
	/sbin/chkconfig --del %name
fi

%files
%_bindir/*
%_sbindir/*
%_initrddir/*
%_usr/libexec/%name
%config(noreplace) %_sysconfdir/cron.d/%name

%changelog
* Wed Feb 03 2016 Alexey Gladkov <legion@altlinux.ru> 0.0.2-alt1
- Fix build warnings.

* Wed Sep 05 2012 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt6
- Fix build.

* Fri Jul 17 2009 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt5
- Fix build.

* Sun Jun 22 2008 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt4
- Fix openvt path.

* Mon Oct 23 2006 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt3
- Add wrapper for vlock.
- consolelocker.service: Change start and stop priority levels.

* Sun Oct 22 2006 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt2
- Add block sysrq.
- Fix copyright.

* Tue Oct 10 2006 Alexey Gladkov <legion@altlinux.ru> 0.0.1-alt1
- Initial revision.

.\" Copyright (C) 2006  Alexey Gladkov <legion@altlinux.org>
.\"
.\" This file is free software; you can redistribute it and/or modify
.\" it under the terms of the GNU General Public License as published by
.\" the Free Software Foundation; either version 2 of the License, or
.\" (at your option) any later version.
.\" 
.\" This program is distributed in the hope that it will be useful,
.\" but WITHOUT ANY WARRANTY; without even the implied warranty of
.\" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
.\" GNU General Public License for more details.
.\" 
.\" You should have received a copy of the GNU General Public License
.\" along with this program; if not, write to the Free Software
.\" Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

[NAME]
\fBconsolelocker\fR \- virtual console lock daemon

[FILES]
.TP
.I /var/run/consolelocker.fifo
.TP
\fI/var/run/consolelocker.pid
.TP
\fI/var/run/console.lock

[SEE ALSO]
.BR vlock (1),
.BR pam_console (8).
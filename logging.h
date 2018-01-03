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
#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <syslog.h>
#include <stdlib.h>

void logging_init(int, int);
void logging_close(void);
int logging_level(const char *lvl);

void message(int priority, const char *fmt, ...);

#define fatal(format, arg...)                             \
	do {                                              \
		message(LOG_CRIT,                         \
		        "%s(%d): %s: " format,            \
		        __FILE__, __LINE__, __FUNCTION__, \
		        ##arg);                           \
		exit(EXIT_FAILURE);                       \
	} while (0)

#define err(format, arg...)                               \
	do {                                              \
		message(LOG_ERR,                          \
		        "%s(%d): %s: " format,            \
		        __FILE__, __LINE__, __FUNCTION__, \
		        ##arg);                           \
	} while (0)

#define info(format, arg...)                              \
	do {                                              \
		message(LOG_INFO,                         \
		        "%s(%d): %s: " format,            \
		        __FILE__, __LINE__, __FUNCTION__, \
		        ##arg);                           \
	} while (0)

#define dbg(format, arg...)                               \
	do {                                              \
		message(LOG_DEBUG,                        \
		        "%s(%d): %s: " format,            \
		        __FILE__, __LINE__, __FUNCTION__, \
		        ##arg);                           \
	} while (0)

#endif /* _LOGGING_H_ */

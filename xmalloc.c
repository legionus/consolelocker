/*
  Copyright (C) 2002-2007  Dmitry V. Levin <ldv@altlinux.org>

  Dynamic memory allocation with error checking.

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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <limits.h>

#include "xmalloc.h"

void *
xmalloc(size_t size)
{
	void *r = malloc(size);

	if (!r)
		error(EXIT_FAILURE, errno, "malloc: allocating %lu bytes",
		      (unsigned long)size);
	return r;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *r = calloc(nmemb, size);

	if (!r)
		error(EXIT_FAILURE, errno, "calloc: allocating %lu*%lu bytes",
		      (unsigned long)nmemb, (unsigned long)size);
	return r;
}

void *
xrealloc(void *ptr, size_t nmemb, size_t elem_size)
{
	if (nmemb && ULONG_MAX / nmemb < elem_size)
		error(EXIT_FAILURE, 0, "realloc: nmemb*size > ULONG_MAX");

	size_t size = nmemb * elem_size;
	void *r     = realloc(ptr, size);

	if (!r)
		error(EXIT_FAILURE, errno, "realloc: allocating %lu*%lu bytes",
		      (unsigned long)nmemb, (unsigned long)elem_size);
	return r;
}

char *
xstrdup(const char *s)
{
	size_t len = strlen(s);
	char *r    = xmalloc(len + 1);

	memcpy(r, s, len + 1);
	return r;
}

char *
xasprintf(char **ptr, const char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	if (vasprintf(ptr, fmt, arg) < 0)
		error(EXIT_FAILURE, errno, "vasprintf");
	va_end(arg);

	return *ptr;
}


/*
 * this file is part of distexec
 *
 * Copyright (C) 2016-2017 Konrad Lother <k@hiddenbox.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include "distexec/error.h"
#include "distexec/logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* holds the error message */
static struct _libdistexec_error *last_error = NULL;

EXPORT int libdistexec_error(int code, unsigned char _abort, const char *file,
			     const char *func, unsigned int line, char *format,
			     ...)
{
	struct _libdistexec_error *e;
	va_list list;
	char buf[ERROR_MAX_LEN];

	va_start(list, format);
	vsprintf(buf, format, list);
	va_end(list);

	e = malloc(sizeof(struct _libdistexec_error));
	e->code = code;
	e->file = strdup(file);
	e->func = strdup(func);
	e->line = line;
	e->message = strdup(buf);

	last_error = e;

	if (1 == _abort) {
		libdistexec_error_dump(e);
		exit(e->code);
	}
	return e->code;
}

EXPORT char *libdistexec_error_format(libdistexec_error_t e)
{
	int bufsize = 1024;
	char buf[bufsize];
	sprintf(buf, "%s:%s:%u: %s: %s (%i)", e.file, e.func, e.line, e.message,
		libdistexec_error_str(e.code), e.code);
	return strdup(buf);
}

EXPORT void libdistexec_error_dump(libdistexec_error_t * e)
{
	char *format = libdistexec_error_format(*e);
	fprintf(stderr, "%s\n", format);
	fflush(stderr);
	free(format);
}

EXPORT char *libdistexec_error_str(int code)
{
#ifdef _WIN32
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM |
		      FORMAT_MESSAGE_IGNORE_INSERTS,
		      NULL,
		      code,
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		      (LPTSTR) & lpMsgBuf, 0, NULL);
	return lpMsgBuf;
#else
	return strerror(code);
#endif
}

EXPORT bool libdistexec_error_exist()
{
	return ((NULL != last_error) ? true : false);
}

EXPORT libdistexec_error_t *libdistexec_error_last()
{
	return (libdistexec_error_exist()? last_error : NULL);
}

EXPORT int libdistexec_error_release(libdistexec_error_t * e)
{
	struct _libdistexec_error *_e;

	if (NULL == e && libdistexec_error_exist())
		_e = (struct _libdistexec_error *)last_error;
	else if (NULL != e)
		_e = (struct _libdistexec_error *)e;
	else
		return -1;

	free(_e->file);
	free(_e->func);
	free(_e->message);
	free(_e);
	return 0;
}

EXPORT int libdistexec_errno()
{
#ifdef _WIN32
	return GetLastError();
#else
	return errno;
#endif
}

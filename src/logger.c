/*
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

#include "distexec/logger.h"
#include "distexec/error.h"
#include "distexec/util.h"
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <libgen.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef _WIN32

#else

#endif

static char *default_format = LOGGER_FORMAT_DEFAULT;
static char *default_format_date = LOGGER_FORMAT_DATE;
static char *default_format_time = LOGGER_FORMAT_TIME;

static loglevel_t default_level =
    LINFO | LWARNING | LERROR | LFATAL | LCRITICAL;

static libdistexec_logger_t core_logger = {
	.name = NULL,
	.out = NULL,
	.err = NULL,
	.level = LINFO,
	.format = LOGGER_FORMAT_DEFAULT,
	.format_date = LOGGER_FORMAT_DATE,
	.format_time = LOGGER_FORMAT_TIME,
};

static libdistexec_list_t *loggers = NULL;

static char *_get_strftime(time_t * rawtime, char *format, size_t len);

static char *strlevel(loglevel_t level)
{
	switch (level) {
	case LINFO:
		return "INFO";
	case LWARNING:
		return "WARN";
	case LERROR:
		return "ERROR";
	case LCRITICAL:
		return "CRITICAL";
	case LFATAL:
		return "FATAL";
	case LDEBUG:
		return "DEBUG";
	default:
		return "UNKNOWN";
	}
}

EXPORT int libdistexec_logger_init(FILE * out, FILE * err)
{
	libdistexec_list_init(loggers, libdistexec_logger_t *, 0);
	libdistexec_logger_new(&core_logger, "core");
	core_logger.out = out;
	core_logger.err = err;
	return 0;
}

EXPORT void
libdistexec_logger(libdistexec_logger_t logger, loglevel_t level,
		   const char *filepath, const char *func, unsigned int line,
		   char *format, ...)
{
#ifdef LOGGER_DISABLE
	return;
#else

#ifndef LOGGER_DEBUG
	if (logger.level == LDEBUG)
		return;
#endif
	if ((logger.level != LALL) && (0 == (logger.level & level)))
		return;

	char mbuf[LOGGER_MAX_LEN];

	va_list list;
	va_start(list, format);
	vsnprintf(mbuf, LOGGER_MAX_LEN, format, list);
	va_end(list);

	// see libdistexec_logger.h enum. Everything below LWARNING should go to err out
	FILE *out = NULL;
	if (level < LWARNING) {
		out = logger.err;
	} else {
		out = logger.out;
	}

	if (NULL == out)
		goto exit;

	char *p;
	char fbuf[LOGGER_MAX_LEN] = { '\0' };
	size_t findex = 0;
	char *tmp;
	time_t rawtime;
	time(&rawtime);
	int msec;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	msec = lrint(tv.tv_usec / 1000.0);
	if (msec >= 1000)
		msec -= 1000;

#define REPL_TIME(format,len) \
	tmp = _get_strftime(&rawtime, format, len); \
	strcat(fbuf, tmp); \
	findex += strlen(tmp); \
	free(tmp); \
	tmp = NULL;

#define REPL_NUM(format, num) \
	tmp = calloc(sizeof(char), 32); \
	sprintf(tmp, format, num); \
	strcat(fbuf, tmp);\
	findex += strlen(tmp); \
	free(tmp); \
	tmp = NULL;

#define REPL_STR(str) \
	strcat(fbuf, str); \
	findex += strlen(str);

	/* parse log format */
	for (p = logger.format; *p != '\0'; p++) {
		if (*p == '%') {
			p++;
		} else {
			fbuf[findex++] = *p;
			continue;
		}

		switch (*p) {
		case '%':
			fbuf[findex++] = '%';
			break;
		case 'D':
			REPL_TIME(logger.format_date, 32);
			break;
		case 'T':
			REPL_TIME(logger.format_time, 32);
			break;
		case 'X':
			REPL_NUM("%03d", msec);
			break;
		case 'N':
			REPL_STR(logger.name);
			break;
		case 'L':
			REPL_STR(strlevel(level));
			break;
		case 'f':
			REPL_STR(filepath);
			break;
		case 'm':
			REPL_STR(func);
			break;
		case 'l':
			REPL_NUM("%u", line);
			break;
		case 'M':
			REPL_STR(mbuf);
			break;
		default:
			fprintf(stderr, "Log format error: %%%c - %s\n", *p,
				logger.format);
			fflush(stderr);
			exit(EXIT_FAILURE);
		}
	}

	fprintf(out, "%s\n", fbuf);
	fflush(out);
 exit:
	return;
#endif
}

static char *_get_strftime(time_t * rawtime, char *format, size_t len)
{
	struct tm timeinfo;
	char buf[len];
	//timeinfo =
#ifdef _WIN32
	localtime_s(&timeinfo, rawtime);
#else
	localtime_r(rawtime, &timeinfo);
#endif
	strftime(buf, len, format, &timeinfo);
	return strdup(buf);
}

EXPORT int libdistexec_logger_new(libdistexec_logger_t * logger,
				  const char *name)
{
	logger->name = strdup(name);
	logger->out = core_logger.out;
	logger->err = core_logger.err;
	logger->format = default_format;
	logger->format_date = default_format_date;
	logger->format_time = default_format_time;
	logger->level = default_level;
	libdistexec_list_add(loggers, libdistexec_logger_t *, logger);
	return 0;
}

EXPORT int libdistexec_logger_set_format(char *format)
{
	default_format = format;
	core_logger.format = format;
	return 0;
}

EXPORT int libdistexec_logger_set_format_date(char *format)
{
	default_format_date = format;
	core_logger.format_date = format;
	return 0;
}

EXPORT int libdistexec_logger_set_format_time(char *format)
{
	default_format_time = format;
	core_logger.format_time = format;
	return 0;
}

EXPORT void libdistexec_logger_set_level(loglevel_t level)
{
	default_level = level;
	core_logger.level = level;
}

EXPORT libdistexec_logger_t *libdistexec_logger_core()
{
	return &core_logger;
}

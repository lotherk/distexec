
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





#ifndef DISTEXEC_LOGGER_H
#define DISTEXEC_LOGGER_H
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "distexec/export.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LFATAL = 1,
	LCRITICAL = 2,
	LERROR = 4,
	LWARNING = 8,
	LINFO = 16,
	LDEBUG = 32,
	LALL = 64
} loglevel_t;


typedef struct {
	char *name;
	char *format;
	char *format_date;
	char *format_time;
	loglevel_t level;
	FILE *out;
	FILE *err;
} libdistexec_logger_t;




#ifdef LOGGER_DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

#define LOG_CORE(...) if (DEBUG) { fprintf(stderr,  __VA_ARGS__); fflush(stderr); }

#define LOGGER_MAX_LEN 4096

#if !defined(MACRO_LOGGER)
#define MACRO_LOGGER *libdistexec_logger_core()
#endif

#if !defined(__FILENAME__)
#define __FILENAME__ __FILE__
#endif

#define LOGGER(LEVEL, ...) libdistexec_logger(MACRO_LOGGER, LEVEL, \
				     __FILENAME__, \
				     __func__, \
				     __LINE__, \
				     __VA_ARGS__)

#define LOG_INFO(...)       LOGGER(LINFO, __VA_ARGS__)
#define LOG_WARN(...)       LOGGER(LWARNING, __VA_ARGS__)
#define LOG_ERROR(...)      LOGGER(LERROR, __VA_ARGS__)
#define LOG_CRITICAL(...)   LOGGER(LCRITICAL, __VA_ARGS__)
#define LOG_FATAL(...)      LOGGER(LFATAL, __VA_ARGS__)
#define LOG_DEBUG(...)      LOGGER(LDEBUG, __VA_ARGS__)

EXPORT void libdistexec_logger(libdistexec_logger_t logger, loglevel_t level, const char *filepath, const char *func,
	  unsigned int line, char *fmt, ...);

EXPORT void libdistexec_logger_set_level(loglevel_t level);
EXPORT int libdistexec_logger_set_format(char *format);
EXPORT int libdistexec_logger_set_format_date(char *format);
EXPORT int libdistexec_logger_set_format_time(char *format);
EXPORT int libdistexec_logger_new(libdistexec_logger_t *logger, const char *name);
EXPORT int libdistexec_logger_init(FILE * out, FILE * err);
EXPORT libdistexec_logger_t *libdistexec_logger_core();

#ifdef __cplusplus
}
#endif

#endif

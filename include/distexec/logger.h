
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
    unsigned char initialized;
	char *name;
	char *format;
	char *format_date;
	char *format_time;
	loglevel_t level;
	FILE *out;
	FILE *err;
} logger_t;

#define LOGGER_MAX_LEN 4096

#if !defined(__FILENAME__)
#define __FILENAME__ __FILE__
#endif

#define LOGGER(logger, level, ...) log(logger, level, 0,\
				     __FILENAME__, \
				     __func__, \
				     __LINE__, \
				     __VA_ARGS__)

#define LOG_INFO(logger, ...)       LOGGER(logger, LINFO, __VA_ARGS__)
#define LOG_WARN(logger, ...)       LOGGER(logger, LWARNING, __VA_ARGS__)
#define LOG_ERROR(logger, ...)      LOGGER(logger, LERROR, __VA_ARGS__)
#define LOG_CRITICAL(logger, ...)   LOGGER(logger, LCRITICAL, __VA_ARGS__)
#define LOG_FATAL(logger, ...)      LOGGER(logger, LFATAL, __VA_ARGS__)
#define LOG_DEBUG(logger, ...)      LOGGER(logger, LDEBUG, __VA_ARGS__)

void log(logger_t logger, loglevel_t level, int severity, const char *filepath, const char *func,
	  unsigned int line, char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif

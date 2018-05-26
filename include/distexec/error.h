
/*
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


/**
 * @brief	pioe error handling
 *
 * More detailed description for pioe's error handling.
 *
 * @date	11/11/2016
 * @file	error.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_ERROR_H
#define DISTEXEC_ERROR_H

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "distexec/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
__declspec(dllimport) __cdecl extern int errno;
#else
extern int errno;
#endif

#define ERROR_MAX_LEN 1024

/**
 * @def LIBDISTEXEC_ERROR(status, ...)
 * Sets a new core error
 */
#define LIBDISTEXEC_ERROR(code, ...) \
	libdistexec_error(code, 0, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

/**
 * @def LIBDISTEXEC_ABORT(status, ...)
 * Creates an libdistexec_error_t, dumps it using libdistexec_error_dump() and aborts the program.
 */
#define LIBDISTEXEC_ABORT(status, ...) \
	libdistexec_error(status, 1, __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

struct _libdistexec_error {
	int code;
	char *file;
	char *func;
	unsigned int line;
	char *errorstr;
	char *message;
};

typedef const struct _libdistexec_error libdistexec_error_t;

/**
 * @brief Create a new error
 *
 * Allocates memory for a new libdistexec_error_t struct. Should not be called directly.
 *
 * @param code an integer representing the error code
 * @param file the file the error occured in
 * @param func the function the error occured in
 * @param line the line number of the file
 * @param format the error message
 * @param ... don't know what to write here
 * @return the allocated error struct.
 * @see libdistexec_error_release(libdistexec_error_t *e)
 * @see LIBDISTEXEC_ERROR(status, ...)
 */
EXPORT int libdistexec_error(int code, unsigned char _abort, const char *file,
				   const char *func, unsigned int line,
				   char *format, ...);

EXPORT int libdistexec_errno();

EXPORT char* libdistexec_error_str(int code);

EXPORT libdistexec_error_t *libdistexec_error_last();

/**
 * @brief Check if a "core" error has been set.
 *
 * @return true if an error is set, false if not
 */
EXPORT bool libdistexec_error_exist();

/**
 * @brief Releases an libdistexec_error_t (free() etc.)
 *
 * If the provided argument is NULL the "core" error will be released.
 *
 * @param e the libdistexec_error_t* to release or NULL to release the "core" error.
 */

EXPORT int libdistexec_error_release(libdistexec_error_t * e);

/**
 * @brief Dumps an libdistexec_error_t to the console
 *
 * @param e the libdistexec_error_t* to dump
 */
EXPORT void libdistexec_error_dump(libdistexec_error_t * e);

EXPORT char *libdistexec_error_format(libdistexec_error_t e);

#ifdef __cplusplus
}
#endif
#endif

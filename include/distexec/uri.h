
/* uri.h: ADD DESCRIPTION HERE
 *
 * Copyright (C) 2017 Konrad Lother <konrad@hiddenbox.org>
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
 * @brief	Add brief description!
 *
 * More detailed description
 *
 * @date	02/15/2017
 * @file	uri.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_URI_H
#define DISTEXEC_URI_H

#include <stdlib.h>
#include "distexec/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define URI_MAX_PARAMS 64

struct uri_param {
	char *key;
	char *value;
};

typedef struct uri {
	char *scheme;
	char *hostname;
	char *path;
	char *port;
	struct uri_param **get_params;
	size_t get_params_s;
} libdistexec_uri_t;

EXPORT libdistexec_uri_t *libdistexec_uri_new();

EXPORT int libdistexec_uri_free(libdistexec_uri_t *uri);

EXPORT int libdistexec_uri_parse(const char *url, libdistexec_uri_t *uri);

EXPORT int libdistexec_uri_add_get_param(libdistexec_uri_t *uri, const char *key, const char *value);

EXPORT int libdistexec_uri_set_get_param(libdistexec_uri_t *uri, const char *key, const char *value);

EXPORT char *libdistexec_uri_build(libdistexec_uri_t *uri);

EXPORT char * libdistexec_uri_encode(const char *v);

EXPORT char *libdistexec_uri_decode(const char *v);
EXPORT char *libdistexec_uri_get_param(libdistexec_uri_t *uri, const char *key);
EXPORT struct uri_param *libdistexec_uri_get_uri_param(libdistexec_uri_t *uri, const char *key);

EXPORT libdistexec_uri_t *libdistexec_uri_copy(libdistexec_uri_t *src);
#ifdef __cplusplus
}
#endif
#endif

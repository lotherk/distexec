
/* uri.c: ADD DESCRIPTION HERE
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "distexec/uri.h"
#include "distexec/util.h"
#include "distexec/error.h"
#include "distexec/logger.h"

static char *char_map[] = {
	"!", "%21",
	"#", "%23",
	"$", "%24",
	"&", "%26",
	"'", "%27",
	"(", "%28",
	")", "%29",
	"*", "%2A",
	"+", "%2B",
	",", "%2C",
	"/", "%2F",
	":", "%3A",
	";", "%3B",
	"=", "%3D",
	"?", "%3F",
	"@", "%40",
	"[", "%5B",
	"]", "%5D",
	"\n", "%0A",
	" ", "%20",
	"\"", "%22",
	"%", "%25",
	"-", "%2D",
	".", "%2E",
	"<", "%3C",
	">", "%3E",
	"\\", "%5C",
	"^", "%5E",
	"_", "%5F",
	"`", "%60",
	"{", "%7B",
	"|", "%7C",
	"}", "%7D",
	"~", "%7E",
};

static struct uri_param *uri_param_new()
{
	struct uri_param *p = malloc(sizeof(struct uri_param));
	p->key = NULL;
	p->value = NULL;
	return p;
}

EXPORT libdistexec_uri_t *libdistexec_uri_new()
{
	struct uri *p = malloc(sizeof(struct uri));

	if (NULL == p)
		return NULL;

	p->scheme = NULL;
	p->hostname = NULL;
	p->path = NULL;
	p->port = NULL;
	p->get_params = NULL;
	p->get_params_s = 0;
	return p;
}

EXPORT int libdistexec_uri_free(libdistexec_uri_t * uri)
{
	if (uri->get_params != NULL) {
		int i;
		for (i = 0; i < uri->get_params_s; i++) {
			struct uri_param *p = uri->get_params[i];
			free(p->key);
			free(p->value);
			free(p);
		}
		free(uri->get_params);
	}
	free(uri);

	return 0;
}

EXPORT int libdistexec_uri_set_get_param(libdistexec_uri_t * uri,
					 const char *key, const char *value)
{
	if (NULL == key)
		return -1;

	if (NULL == value)
		return -2;

	struct uri_param *p = libdistexec_uri_get_uri_param(uri, key);

	if (NULL == p)
		return libdistexec_uri_add_get_param(uri, key, value);

	if (NULL != p->value)
		free(p->value);

	p->value = strdup(value);

	return 0;
}

EXPORT int libdistexec_uri_add_get_param(libdistexec_uri_t * uri,
					 const char *key, const char *value)
{
	if (uri->get_params_s >= URI_MAX_PARAMS)
		return -1;

	if (NULL == uri)
		return -2;

	if (NULL == key)
		return -3;

	if (NULL == uri->get_params) {
		uri->get_params =
		    calloc(URI_MAX_PARAMS, sizeof(struct uri_param *));

	}
/*		uri->get_params = calloc(1, sizeof(struct uri_param *));
	} else {
		uri->get_params =
		    realloc(uri->get_params, uri->get_params_s + 1);
	}
TODO: FIXME! */

	if (NULL == uri->get_params)
		return -4;

	struct uri_param *p = uri_param_new();
	if (NULL == p)
		return -5;

	p->key = strdup(key);
	if (NULL != value)
		p->value = strdup(value);

	uri->get_params[uri->get_params_s++] = p;
	return 0;
}

EXPORT char *libdistexec_uri_build(libdistexec_uri_t * uri)
{
	size_t bufsize = 1024;
	char *buf = calloc(bufsize + 1, sizeof(char));
#define ADD_TO_URI(s) \
	if (s != NULL) { \
		if (bufsize < (strlen(buf) + strlen(s))) { \
			bufsize = strlen(buf) + strlen(s) + 1 + 1; \
			char *tmp; \
			tmp = realloc(buf, bufsize); \
			if (NULL == tmp) { \
				LIBDISTEXEC_ABORT(-1, "realloc failed"); \
			} \
			buf = tmp; \
		} \
		strcat(buf, s); \
	}

	ADD_TO_URI(uri->scheme);
	ADD_TO_URI("://");
	ADD_TO_URI(uri->hostname);

	if (NULL != uri->port) {
		ADD_TO_URI(":");
		ADD_TO_URI(uri->port);
	}

	ADD_TO_URI("/");
	ADD_TO_URI(uri->path);
	if (uri->get_params_s > 0) {
		ADD_TO_URI("?");
		int i;
		for (i = 0; i < uri->get_params_s; i++) {
			struct uri_param *p = uri->get_params[i];
			ADD_TO_URI(p->key);
			if (NULL != p->value) {
				ADD_TO_URI("=");
				char *nvalue = libdistexec_uri_encode(p->value);
				ADD_TO_URI(nvalue);
				free(nvalue);
			}
			if (i < uri->get_params_s - 1)
				ADD_TO_URI("&");
		}
	}
#undef ADD_TO_URI

	return buf;

}

EXPORT int libdistexec_uri_parse(const char *url, libdistexec_uri_t * uri)
{
	size_t usize = 0;
	size_t uindex = 0;
	char *scheme = NULL;
	char *hostname = NULL;
	char *path = NULL;
	char *port = NULL;
	char *p = NULL;
	int i;
	/*
	 *  [scheme]://[hostname]:[port]/[path]?[query]
	 */
	p = (char *)url;
	usize = strlen(p);
	// scan until first : occurance
	for (i = 0; i < usize; i++) {
		if ((*p++) == ':') {
			if (i + 2 > usize)
				return -1;

			if (*(p++) != '/' && *(p++) != '/')
				return -2;

			scheme = calloc(i + 1, sizeof(char));
			memcpy(scheme, url, i);
			uindex += i + 3;
			(void)*(p++);
			break;
		}
	}

	// if : occurs, we have a port
	// if / occurs, we MAY have a path
	// if neither : nor / occurs, we just have the hostname
	for (i = 0; *p != '\0'; i++) {
		if ((*p) == ':') {
			// we have a port
			hostname = calloc(i + 1, sizeof(char));
			memcpy(hostname, url + uindex, i);
			uindex += (i + 1);
			int c;
			for (c = 0; *p != '\0'; c++)
				if (*(p++) == '/')
					break;
			i += c;
			port = calloc(c + 1, sizeof(char));
			memcpy(port, url + uindex, c - 1);
			uindex += c;
			break;
		} else if ((*p) == '/') {
			// we may have a path
			hostname = calloc(i + 1, sizeof(char) + 1);
			memcpy(hostname, url + uindex, i);
			uindex += (i + 1);
			(void)*(p++);
			break;
		}

		(void)*(p++);
	}
	// if ? occurs, we have params
	for (i = 0; *p != '\0'; i++) {
		if ((*p) == '?') {
			path = calloc(i + 1, sizeof(char));
			memcpy(path, url + uindex, i);
			uindex += (i + 1);
			(void)*(p++);
			break;
		}
		(void)*(p++);
	}
	if (NULL == path && strlen(p) == 0) {
		path = calloc(i + 1, sizeof(char));
		memcpy(path, url + uindex, i);
		uindex += (i + 1);
	}
	// parse params, if something left in p
	if (strlen(p) > 0) {
		char *tmp;
		char *saveptr = NULL;
		tmp = strtok_r(strdup(p), "&", &saveptr);	// why strdup?
		do {
			char *sptr;
			char *key = strtok_r(tmp, "=", &sptr);

			if (NULL == key) {
				// garbage?!
			}
			char *value = strtok_r(NULL, "\0", &sptr);

			if (NULL != value) {
				value = libdistexec_uri_decode(value);
				libdistexec_uri_add_get_param(uri, key, value);
				free(value);
			} else
				libdistexec_uri_add_get_param(uri, key, NULL);

		} while ((tmp = strtok_r(NULL, "&", &saveptr)) != NULL);
	}

	uri->hostname = hostname;
	uri->scheme = scheme;
	uri->path = path;
	uri->port = port;

	return 0;
}

EXPORT libdistexec_uri_t *libdistexec_uri_copy(libdistexec_uri_t * src)
{
#define _CP(key) \
	if (src->key != NULL) { \
		copy->key = strdup(src->key); \
	} else { \
		LIBDISTEXEC_ABORT(-1, "is null!"); \
	}

	libdistexec_uri_t *copy = libdistexec_uri_new();
	_CP(scheme);
	_CP(hostname);
	_CP(port);
	_CP(path);

	// copy params
	int i;
	for (i = 0; i < src->get_params_s; i++)
		libdistexec_uri_add_get_param(copy, src->get_params[i]->key,
					      src->get_params[i]->value);
	return copy;
}

EXPORT char *libdistexec_uri_encode(const char *v)
{
	char *p = (char *)v;

	size_t cms = ARRAY_SIZE(char_map);
	if ((cms % 2) != 0)
		LIBDISTEXEC_ABORT(-1, "cms % 2 != 0");

	size_t buflen = 512;
	char *buf = calloc(buflen + 1, sizeof(char));
	size_t bufs = 0;

	while (*p != '\0') {
		int found = 0;
		int i;
		for (i = 0; i < cms; i += 2) {
			if (*p == *(char_map[i])) {
				int x;
				char *str = char_map[i + 1];
				for (x = 0; x < strlen(str); x++)
					buf[bufs++] = str[x];
				found = 1;
				break;
			}
		}

		if (found == 0)
			buf[bufs++] = *p;

		(void)*(p++);

	}
	return buf;
}

EXPORT char *libdistexec_uri_decode(const char *v)
{
	char *p = (char *)v;

	size_t cms = ARRAY_SIZE(char_map);
	if ((cms % 2) != 0)
		LIBDISTEXEC_ABORT(-1, "cms % 2 != 0");

	size_t buflen = 512;
	char *buf = calloc(buflen + 1, sizeof(char));
	size_t bufs = 0;
	do {
		int found = 0;
		if (*p == '%') {
			char search[4];
			search[0] = *(p);
			search[1] = *(p + 1);
			search[2] = *(p + 2);
			search[3] = '\0';	// not sure if needed, I'm confused.

			int i;
			for (i = 0; i < cms; i += 2) {
				if (strcmp(search, char_map[i + 1]) == 0) {
					buf[bufs++] = *(char_map[i]);
					p += 2;
					found = 1;
					break;
				}
			}

		}
		if (found == 0)
			buf[bufs++] = *p;
	} while (*(p++) != '\0');
	return buf;
}

EXPORT char *libdistexec_uri_get_param(libdistexec_uri_t * uri, const char *key)
{
	struct uri_param *p = libdistexec_uri_get_uri_param(uri, key);
	if (NULL == p)
		return NULL;

	return p->value;
}

EXPORT struct uri_param *libdistexec_uri_get_uri_param(libdistexec_uri_t * uri,
						       const char *key)
{
	if (NULL == uri->get_params)
		return NULL;
	int i;
	for (i = 0; i < uri->get_params_s; i++) {
		struct uri_param *p = uri->get_params[i];
		if (strcmp(p->key, key) == 0)
			return p;
	}

	return NULL;
}

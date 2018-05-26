
/* exec.c: exec backend and frontend for distexec
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

#ifdef _WIN32
#else
#include <poll.h>
#endif
#include "distexec/export.h"
#include "distexec/error.h"
#include "distexec/plugin.h"
#include "distexec/logger.h"
#include "distexec/util.h"
#include "distexec/popen3.h"

#include <stdio.h>
#undef MACRO_LOGGER
#define MACRO_LOGGER logger

static libdistexec_logger_t logger;

static char buf_stdout[8192];
static char buf_stderr[8192];

static struct _backend_config {
	const char *program;
	unsigned int timeout;
} backend_config;

static struct _frontend_config {
	const char *program;
	unsigned int timeout;
} frontend_config;

static const char *backend_config_values[] = {
	"program",
	"timeout",
};

static const char *frontend_config_values[] = {
	"program",
	"timeout",
};

static int fetch(const char *filter, libdistexec_node_t *** result);
static int exec(libdistexec_node_t * node, const char *command);

static int set_frontend_config_value(const char *key, const char *value)
{

	if (strcmp(key, "program") == 0)
		frontend_config.program = strdup(value);
	else if (strcmp(key, "timeout") == 0)
		sscanf(value, "%u", &(frontend_config.timeout));
	else
		LIBDISTEXEC_ABORT(-1, "unknown config option %s", key);

	return 0;
}

static int set_backend_config_value(const char *key, const char *value)
{
	if (strcmp(key, "program") == 0) {
		backend_config.program = strdup(value);
	} else {
		LIBDISTEXEC_ABORT(-1, "unknown config option %s", key);
	}
	return 0;
}

EXPORT int load()
{
	if (backend_register
	    ("exec", NULL, fetch, set_backend_config_value,
	     backend_config_values, ARRAY_SIZE(backend_config_values))) {
		// could not register backend, handle error.
	} else {

		// set default values
		backend_config.timeout = 1000 * 10;
	}

	if (frontend_register
	    ("exec", NULL, exec, set_frontend_config_value,
	     frontend_config_values, ARRAY_SIZE(frontend_config_values))) {
		// could not register backend, handle error.
		frontend_config.timeout = 1000 * 10;
	} else {

		// set default values
		frontend_config.timeout = 1000 * 10;
	}

	return 0;
}

static int exec(libdistexec_node_t * node, const char *command)
{
	LOG_DEBUG("Exec!");
	return 0;
}

EXPORT int init()
{
	libdistexec_logger_new(&logger, "exec");
	LOG_DEBUG("INIT!!!!");
	return 0;
}

static int fetch(const char *filter, libdistexec_node_t *** result)
{
	if (NULL == backend_config.program)
		LIBDISTEXEC_ABORT(-1,
				  "backend_config.program must not be NULL");

	int pipes[3];
	char *const arg[] = { NULL };
	int res;
#ifndef _WIN32
	struct pollfd ufds[2];

	printf("errno: %i\n", libdistexec_errno());
	LOG_DEBUG("Running command: %s", backend_config.program);
	res = libdistexec_popen3(pipes, backend_config.program, arg);
	printf("errno: %i\n", libdistexec_errno());
	printf("res: %i\n", res);

/*	FILE *out = fdopen(pipes[1], "r");
	FILE *err = fdopen(pipes[2], "r"); */

	ufds[0].fd = pipes[1];
	ufds[0].events = POLLIN | POLLPRI;

	ufds[1].fd = pipes[2];
	ufds[1].events = POLLIN | POLLPRI;

	/* wait for data */
	printf("Polling for %u microseconds\n", backend_config.timeout);
	int rv = poll(ufds, 2, backend_config.timeout);
	printf("rv: %i\n", rv);

	if (rv == -1)
		LIBDISTEXEC_ABORT(-1, "error in poll()");
	else if (rv == 0)
		LIBDISTEXEC_ABORT(-1, "timeout, no data");
	else {
		// check stdout
		if (ufds[0].revents & POLLPRI)
			LIBDISTEXEC_ABORT(-1, "stdout out-of-band data");
		else if (ufds[0].revents & POLLIN) {

		}

	}

	char outbuf[1024];

	exit(0);
#endif
	return 0;
}

EXPORT int unload()
{
	return 0;
}

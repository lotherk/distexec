
/* file.c: file backend for distexec
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

#include "distexec/export.h"
#include "distexec/error.h"
#include "distexec/plugin.h"
#include "distexec/logger.h"
#include "distexec/util.h"

#include <stdio.h>
#include <ctype.h>

#undef MACRO_LOGGER
#define MACRO_LOGGER logger

static libdistexec_logger_t logger;

static char *file = NULL;

static const char *config_values[] = {
	"file",
};

static int fetch(const char *filter, libdistexec_node_t *** result);

static int set_config_value(const char *key, const char *value)
{
	if (strcmp(key, "file") == 0) {
		file = malloc(sizeof(char) * strlen(value) + 1);
		strcpy(file, value);
	} else {
		LIBDISTEXEC_ABORT(-1, "unknown config option %s", key);
	}
	return 0;
}

EXPORT int load()
{
	if (backend_register
	    ("file", NULL, fetch, set_config_value, config_values,
	     ARRAY_SIZE(config_values))) {
		// could not register backend, handle error.
	}

	return 0;
}

static int fetch(const char *filter, libdistexec_node_t *** result)
{
	libdistexec_logger_new(&logger, "file");

	FILE *fp;
	char line[1024];
	size_t total = 1024;
	libdistexec_node_t **node_buf =
	    malloc(total * sizeof(libdistexec_node_t *));

	fp = fopen(file, "r");
	if (NULL == fp) {
		LIBDISTEXEC_ABORT(-1, "error opening file");
	}
	int i = 0;
	while (fgets(line, 1024, fp) != NULL) {

		char *p = line;

		/* drop whitespaces */
		while (isspace(*p))
			(void)*(p++);

		/* drop comments */
		if (*p == '#')
			continue;

		/* drop empty string */
		if (strlen(p) == 0)
			continue;
		char *saveptr;
		char *tmp;
		tmp = strtok_r(p, " #\n", &saveptr);

		libdistexec_node_t *node = libdistexec_node();
		node->hostname = strdup(tmp);
		node_buf[i++] = node;
	}
	fclose(fp);
	*result = node_buf;
	return i;
}

EXPORT int unload()
{
	free(file);
	return 0;
}

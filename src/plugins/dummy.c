
/* ssh.c: ADD DESCRIPTION HERE
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
#include "distexec/plugin.h"
#include "distexec/node.h"
#include "distexec/distexec.h"
#include "distexec/logger.h"
#include "distexec/util.h"

#undef MACRO_LOGGER
#define MACRO_LOGGER logger

static libdistexec_logger_t logger;
static int collect(const char *filter);
static int collect_init();
static int collect_terminate();
static int collect_set_config(const char *key, const char *value);
static const char *collect_config_values[] = {
	"option1",
	"dummy2",
};

static int exec(libdistexec_node_t * node, const char *command);
static int exec_init();
static int exec_terminate();
static int exec_set_config(const char *key, const char *value);
static const char *exec_config_values[] = {
	"option1",
	"dummy2",
};

static int collect_set_config(const char *key, const char *value)
{
	LOG_DEBUG("Set config key '%s' to '%s'", key, value);
	return 0;
}

static int collect_init()
{
	LOG_DEBUG("init!");
	return 0;
}

static int collect_terminate()
{
	LOG_DEBUG("terminate!");
	return 0;
}

static int collect(const char *filter)
{
	LOG_DEBUG("collect nodes with filter '%s'", filter);
	int i;
	for (i = 0; i < 10; i++) {
		LOG_DEBUG("add node_%d", i);
		libdistexec_node_t *node = libdistexec_node_new();
		char buf[2000] = { '\0' };
		sprintf(buf, "server-%d", i);
		node->hostname = strdup(buf);
		libdistexec_node_add(node);
	}
	return 0;
}

static int exec_set_config(const char *key, const char *value)
{
	LOG_DEBUG("Set config key '%s' to '%s'", key, value);
	return 0;
}

static int exec_init()
{
	LOG_DEBUG("init!");
	return 0;
}

static int exec_terminate()
{
	LOG_DEBUG("terminate!");
	return 0;
}

static int exec(libdistexec_node_t * node, const char *command)
{
	LOG_DEBUG("Running '%s' on %s", command, node->hostname);
	return 0;
}

EXPORT int load()
{

	int rc;
	libdistexec_logger_new(&logger, "dummy");

	rc = libdistexec_register_callback_collect("dummy",
						   collect, collect_init,
						   collect_terminate,
						   collect_set_config,
						   collect_config_values,
						   ARRAY_SIZE
						   (collect_config_values));

	if (rc)
		LIBDISTEXEC_ABORT(-1, "could not register callback collect: %d",
				  rc);

	rc = libdistexec_register_callback_execute("dummy",
						   exec, exec_init,
						   exec_terminate,
						   exec_set_config,
						   exec_config_values,
						   ARRAY_SIZE
						   (exec_config_values));

	if (rc)
		LIBDISTEXEC_ABORT(-1, "could not register callback execute: %d",
				  rc);

	return 0;
}

EXPORT int unload()
{
	LOG_DEBUG("unload");
	return 0;
}

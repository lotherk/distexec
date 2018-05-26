
/* distexec.c: ADD DESCRIPTION HERE
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

#include "distexec/logger.h"
#include "distexec/error.h"
#include "distexec/util.h"
#include "distexec/node.h"
#include "distexec/export.h"
#include "distexec/plugin.h"
#include "distexec/distexec.h"
#include "distexec/thread.h"

static char *command = NULL;
static libdistexec_plugin_t *plugin = NULL;
static libdistexec_config_t *config = NULL;
static libdistexec_mutex_t *mutexes = NULL;
static libdistexec_mutex_t yield_mutex;

struct container {
	libdistexec_node_t *node;
	int nindex;
	unsigned int id;
};

static void *thread_exec(void *arg)
{
	int rc;
	struct container *c = arg;
	LOG_DEBUG("node index: %d", c->nindex);
	libdistexec_node_t *node = c->node;

	if (NULL == node || !node) {
		LOG_ERROR("wtf, node can not be NULL!");
		goto release;
	}

	LOG_DEBUG("node: %s", node->hostname);
	rc = libdistexec_call_callback_execute(node, command);
	//rc = plugin->func.exec(node, command);

	if (rc != 0)
		LOG_ERROR("Could not execute on %s", node->hostname);
	else
		LOG_DEBUG("done! %s", node->hostname);
 release:

	libdistexec_mutex_unlock(&(mutexes[c->id]));
	return NULL;
}

EXPORT int libdistexec_init()
{
	int rc;
	rc = libdistexec_mutex_init(&yield_mutex);
	rc = libdistexec_node_init();
	return 0;
}

EXPORT int libdistexec_collect(const char *filter)
{
	int rc;
	rc = libdistexec_call_callback_collect(filter);
	rc = libdistexec_terminate_callback_collect();

	return 0;
}

EXPORT int libdistexec_execute(libdistexec_config_t * cfg, const char *cmd)
{
	int rc;
	libdistexec_llist_t *nodes = libdistexec_node_get_nodes();
	libdistexec_llist_t *current_node = nodes;
	size_t count = libdistexec_llist_count(nodes);

	if (count < 1)
		return -EINVAL;

	if (cfg->concurrent < 1)
		cfg->concurrent = 1;

	if (cfg->concurrent > count) {
		LOG_WARN
		    ("concurrent (%d) is higher than nodes count, adjusting to %d",
		     cfg->concurrent, count);
		cfg->concurrent = count;
	}

	command = strdup(cmd);

	int i;
	int current = 0;
	//plugin = frontend;
	config = cfg;

	struct container *containers[cfg->concurrent];
	libdistexec_thread_t threads[cfg->concurrent];
	mutexes = calloc(cfg->concurrent, sizeof(libdistexec_mutex_t));

	if (NULL == mutexes)
		return -ENOMEM;

	for (i = 0; i < cfg->concurrent; i++) {
		containers[i] = malloc(sizeof(struct container));
		containers[i]->id = i;
		containers[i]->node = NULL;
		containers[i]->nindex = 0;
		threads[i] = 0;
		libdistexec_mutex_init(&(mutexes[i]));
	}

	do {
		for (i = 0; i < cfg->concurrent; i++) {
			struct container *c = containers[i];
			int rc = libdistexec_mutex_trylock(&(mutexes[c->id]));
			if (current_node == NULL)
				goto shutdown;

			if (rc == 0) {
				LOG_DEBUG("slot %d is free (current: %d)",
					  c->id, current);
				c->nindex = current;
				//libdistexec_node_t *n = nodes[current++];
				libdistexec_node_t *n = current_node->value;
				current_node = current_node->next;
				LOG_DEBUG("node: %s", n->hostname);
				c->node = n;
				libdistexec_thread_create(&(threads[c->id]),
							  thread_exec, c);
				count--;
				current++;

			}
		}
		libdistexec_sleep(10);
	} while (count > 0);

 shutdown:

	for (i = 0; i < cfg->concurrent; i++) {
		struct container *c = containers[i];
		libdistexec_thread_join(threads[c->id]);
	}
	rc = libdistexec_terminate_callback_execute();
	free(command);
	command = NULL;
	return 0;
}

#define MAX_YIELD_HANDLERS 256
static int (*yield_handlers[MAX_YIELD_HANDLERS]) (libdistexec_node_t *,
						  const char *, int);
static size_t yield_handlers_i = 0;

EXPORT int
libdistexec_register_yield_handler(int (*yh)
				   (libdistexec_node_t *, const char *,
				    int stream))
{
	if (yield_handlers_i >= MAX_YIELD_HANDLERS)
		return -1;

	yield_handlers[yield_handlers_i++] = yh;
	return 0;
}

EXPORT int libdistexec_yield(libdistexec_node_t * n, const char *s, int i)
{
	libdistexec_mutex_lock(&yield_mutex);
	int x;
	for (x = 0; x < yield_handlers_i; x++)
		yield_handlers[x] (n, s, i);

	libdistexec_mutex_unlock(&yield_mutex);
	return 0;
}

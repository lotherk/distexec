
/* node.c: ADD DESCRIPTION HERE
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
#include "distexec/logger.h"
#include "distexec/node.h"
#include "distexec/thread.h"
#include "distexec/util.h"

static libdistexec_mutex_t add_mutex;

LIBDISTEXEC_LL(nodes);

int libdistexec_node_init()
{
	int rc;
	rc = libdistexec_mutex_init(&add_mutex);
	LIBDISTEXEC_LL_INIT_IF_NULL(nodes);

	return 0;
}

EXPORT libdistexec_llist_t *libdistexec_node_get_nodes()
{
	LIBDISTEXEC_LL_INIT_IF_NULL(nodes);
	return LIBDISTEXEC_LL_EXPAND(nodes);
}

EXPORT libdistexec_node_t *libdistexec_node()
{
	LOG_ERROR("deprecated! use libdistexec_node_new instead");
	return libdistexec_node_new();
}

EXPORT libdistexec_node_t *libdistexec_node_new()
{
	libdistexec_node_t *n = malloc(sizeof(libdistexec_node_t));

	if (NULL == n)
		LIBDISTEXEC_ABORT(-1, "Can not alloc memory");

	n->hostname = NULL;
	n->port = 0;

	return n;
}

EXPORT int libdistexec_node_add(libdistexec_node_t * node)
{

	if (NULL == node)
		return -EINVAL;

	libdistexec_mutex_lock(&add_mutex);
	LOG_DEBUG("adding node %s", node->hostname);

	LIBDISTEXEC_LL_APPEND(nodes, node);

	libdistexec_mutex_unlock(&add_mutex);
	return 0;
}

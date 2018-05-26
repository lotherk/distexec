
/* node.h: ADD DESCRIPTION HERE
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
 * @date	02/05/2017
 * @file	node.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_NODE_H
#define DISTEXEC_NODE_H

#include "distexec/export.h"
#include "distexec/util.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
	const char  *hostname;
	unsigned int    port;

} libdistexec_node_t;

int libdistexec_node_init();

EXPORT libdistexec_node_t *libdistexec_node();
EXPORT libdistexec_node_t *libdistexec_node_new();
EXPORT libdistexec_llist_t *libdistexec_node_get_nodes();

EXPORT int libdistexec_node_add(libdistexec_node_t *node);
#ifdef __cplusplus
}
#endif
#endif

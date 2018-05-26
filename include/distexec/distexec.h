
/* distexec.h: ADD DESCRIPTION HERE
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
 * @date	06/07/2017
 * @file	distexec.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_DISTEXEC_H
#define DISTEXEC_DISTEXEC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "distexec/node.h"
#include "distexec/export.h"
#include "distexec/plugin.h"

typedef struct {
	int concurrent;
} libdistexec_config_t;

EXPORT int libdistexec_init();
EXPORT int libdistexec_collect(const char *filter);
EXPORT int libdistexec_execute(libdistexec_config_t *cfg, const char *cmd);
EXPORT int libdistexec_register_yield_handler(int (*yh)(libdistexec_node_t*, const char*, int stream));
EXPORT int libdistexec_yield(libdistexec_node_t *n, const char *s, int i);

#ifdef __cplusplus
}
#endif
#endif

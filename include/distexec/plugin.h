
/* plugin.h: ADD DESCRIPTION HERE
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
 * @date	02/10/2017
 * @file	plugin.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_PLUGIN_H
#define DISTEXEC_PLUGIN_H

#include "distexec/node.h"
#include "distexec/util.h"
#include "distexec/export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct libdistexec_callback_t libdistexec_callback_t;

typedef enum {
	BACKEND,
	FRONTEND
} libdistexec_plugin_type_t;

typedef struct {
	void *handle;
	int (*load)();
	int (*unload)();
	libdistexec_callback_t *callbacks[1024];
	size_t callbacks_s;
} libdistexec_plugin;

typedef struct {
	const char **values;
	int (*set)(const char *, const char *);
	size_t len;
} libdistexec_plugin_config_t;

union plugin_func {
	int (*fetch)(const char *, libdistexec_node_t***);
	int (*exec)(libdistexec_node_t*, const char *command);
};

typedef struct {
	const char *name;
	libdistexec_plugin_type_t type;
	libdistexec_plugin_config_t *config;
	union plugin_func func;
	int (*init)();
	libdistexec_plugin *parent;
} libdistexec_plugin_t;


struct libdistexec_callback_t {
	const char *name;
	libdistexec_plugin *plugin;
	libdistexec_plugin_config_t config;
	int (*init)();
	int (*terminate)();
};

typedef struct {
	libdistexec_callback_t callback;
	int (*call)(libdistexec_node_t*, const char *);
} libdistexec_callback_execute_t;

typedef struct {
	libdistexec_callback_t callback;
	int (*call)(const char*);
} libdistexec_callback_collect_t;

EXPORT int libdistexec_register_callback_collect(const char *name, int (*cb)(const char*), int (*init)(), int (*terminate)(), int (*set_config)(const char *, const char *), const char **config_values, size_t config_size);
EXPORT int libdistexec_register_callback_execute(const char *name, int (*cb)(libdistexec_node_t*, const char*), int (*init)(), int (*terminate)(), int (*set_config)(const char *, const char *), const char **config_values, size_t config_size);

EXPORT int libdistexec_init_callback_collect();
EXPORT int libdistexec_call_callback_collect(const char *filter);
EXPORT int libdistexec_terminate_callback_collect();
EXPORT int libdistexec_init_callback_execute();
EXPORT int libdistexec_call_callback_execute(libdistexec_node_t *node, const char *cmd);
EXPORT int libdistexec_terminate_callback_execute();

EXPORT int frontend_register(const char *name,
				 int (*init)(),
				 int (*exec) (libdistexec_node_t *, const char *c),
				 int (*set_config)(const char *, const char *),
				 const char **values,
				 size_t size);
EXPORT int backend_register(const char *name,
				int (*init)(),
				int (*fetch) (const char *, libdistexec_node_t***),
				int (*set_config)(const char *, const char *),
				const char **values,
				size_t size);

EXPORT libdistexec_plugin *libdistexec_plugin_open(const char *file);

EXPORT libdistexec_plugin_t *libdistexec_backend_get(const char *name);
EXPORT libdistexec_plugin_t *libdistexec_frontend_get(const char *name);

EXPORT int libdistexec_plugin_config(libdistexec_plugin_t *p, const char *config[], size_t size);
#ifdef __cplusplus
}
#endif
#endif


/* plugin.c: ADD DESCRIPTION HERE
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

#include "distexec/plugin.h"
#include "distexec/util.h"

#define SYM(p, h, n) \
        p->h = libdistexec_dlsym(p->handle, n); \
        if (p->h == NULL)

LIBDISTEXEC_LL(frontends);	/* linked list */
LIBDISTEXEC_LL(backends);	/* linked list */

LIBDISTEXEC_LL(execute);
LIBDISTEXEC_LL(collect);

static libdistexec_plugin *current_plugin = NULL;

static libdistexec_plugin_t *plugin_register(libdistexec_plugin_type_t type,
					     const char *name, int (*init) (),
					     int (*set_config) (const char *,
								const char *),
					     const char **values, size_t size);

EXPORT int frontend_register(const char *name,
			     int (*init) (),
			     int (*exec) (libdistexec_node_t *, const char *),
			     int (*set_config) (const char *, const char *),
			     const char **values, size_t size)
{
	LIBDISTEXEC_LL_INIT_IF_NULL(frontends);

	libdistexec_plugin_t *frontend =
	    plugin_register(FRONTEND, name, init, set_config, values, size);

	if (NULL == frontend)
		return -1;

	frontend->func.exec = exec;
	LIBDISTEXEC_LL_APPEND(frontends, frontend);
	return 0;
}

EXPORT int backend_register(const char *name,
			    int (*init) (),
			    int (*fetch) (const char *, libdistexec_node_t ***),
			    int (*set_config) (const char *, const char *),
			    const char **values, size_t size)
{
	LIBDISTEXEC_LL_INIT_IF_NULL(backends);

	libdistexec_plugin_t *backend =
	    plugin_register(BACKEND, name, init, set_config, values, size);

	if (NULL == backend)
		return -1;

	backend->func.fetch = fetch;
	LIBDISTEXEC_LL_APPEND(backends, backend);
	return 0;
}

static libdistexec_plugin_t *plugin_register(libdistexec_plugin_type_t type,
					     const char *name, int (*init) (),
					     int (*set_config) (const char *,
								const char *),
					     const char **values, size_t size)
{

	if (NULL == current_plugin)
		LIBDISTEXEC_ABORT(-1, "current_plugin must not be NULL");

	libdistexec_plugin_t *p = malloc(sizeof(libdistexec_plugin_t));

	p->type = type;
	p->name = strdup(name);
	p->parent = current_plugin;
	p->config = malloc(sizeof(libdistexec_plugin_config_t));
	p->config->values = values;
	p->config->len = size;
	p->config->set = set_config;
	p->init = init;

	return p;
}

EXPORT libdistexec_plugin *libdistexec_plugin_open(const char *file)
{

	LOG_DEBUG("Trying to load plugin %s", file);
	void *handle = libdistexec_dlopen(file);
	if (NULL == handle) {
		libdistexec_error_t *e = libdistexec_error_last();
		libdistexec_error_dump(e);
		return NULL;
	}

	libdistexec_plugin *plugin = malloc(sizeof(libdistexec_plugin));

	if (NULL == plugin)
		LIBDISTEXEC_ABORT(-1, "Cannot alloc");

	current_plugin = plugin;
	plugin->callbacks_s = 0;
	plugin->handle = handle;
	SYM(plugin, load, "load") {
		LOG_ERROR("%s is missing load()", file);
		goto cleanup;
	}
	SYM(plugin, unload, "unload") {
		LOG_ERROR("%s is missing unload()", file);
		goto cleanup;
	}

	if (plugin->load()) {
		LOG_ERROR("Error calling load in %s", file);
		goto cleanup;
	}
	current_plugin = NULL;
	return plugin;

 cleanup:
	LOG_ERROR("Error loading %s", file);
	libdistexec_dlclose(plugin->handle);
	free(plugin);
	return NULL;

}

EXPORT int libdistexec_init_callback_execute()
{
	int rc;
	LIBDISTEXEC_LL_EACH(execute, libdistexec_callback_execute_t *, cb) {
		if (NULL == cb->callback.init)
			continue;

		rc = cb->callback.init();

		if (rc) {
			LOG_DEBUG("[%s] init failed, removing. (%d)",
				  cb->callback.name);
			libdistexec_llist_t *prev = __current->prev;

			if (NULL != prev)
				if (NULL != __current->next)
					prev->next = __current->next;
		}
	}
	LIBDISTEXEC_LL_END();

	return 0;
}

EXPORT int libdistexec_init_callback_collect()
{
	int rc;
	LIBDISTEXEC_LL_EACH(collect, libdistexec_callback_collect_t *, cb) {

		if (NULL == cb->callback.init)
			continue;

		rc = cb->callback.init();

		if (rc) {
			LOG_DEBUG("[%s] init failed, removing. (%d)",
				  cb->callback.name);
			libdistexec_llist_t *prev = __current->prev;

			if (NULL != prev)
				if (NULL != __current->next)
					prev->next = __current->next;
		}
	}
	LIBDISTEXEC_LL_END();

	return 0;
}

EXPORT int libdistexec_call_callback_execute(libdistexec_node_t * node,
					     const char *command)
{
	int rc;
	LIBDISTEXEC_LL_EACH(execute, libdistexec_callback_execute_t *, cb) {
		rc = cb->call(node, command);
		if (rc)
			LOG_ERROR("callback error %s: %d", cb->callback.name,
				  rc);
	}
	LIBDISTEXEC_LL_END();

	return 0;
}

EXPORT int libdistexec_terminate_callback_collect()
{
	int rc;
	LIBDISTEXEC_LL_EACH(collect, libdistexec_callback_collect_t *, cb) {
		if (NULL == cb->callback.terminate)
			continue;

		rc = cb->callback.terminate();

		if (rc) {
			LOG_DEBUG("[%s] terminate failed, removing. (%d)",
				  cb->callback.name);
			libdistexec_llist_t *prev = __current->prev;

			if (NULL != prev)
				if (NULL != __current->next)
					prev->next = __current->next;
		}
	}
	LIBDISTEXEC_LL_END();

	return 0;
}

EXPORT int libdistexec_terminate_callback_execute()
{
	int rc;
	LIBDISTEXEC_LL_EACH(execute, libdistexec_callback_execute_t *, cb) {
		if (NULL == cb->callback.terminate)
			continue;

		rc = cb->callback.terminate();

		if (rc) {
			LOG_DEBUG("[%s] terminate failed, removing. (%d)",
				  cb->callback.name);
			libdistexec_llist_t *prev = __current->prev;

			if (NULL != prev)
				if (NULL != __current->next)
					prev->next = __current->next;
		}
	}
	LIBDISTEXEC_LL_END();

	return 0;
}

EXPORT int libdistexec_register_callback_execute(const char *name,
						 int (*cb) (libdistexec_node_t
							    *, const char *),
						 int (*init) (),
						 int (*terminate) (),
						 int (*set_config) (const char
								    *,
								    const char
								    *),
						 const char **config_values,
						 size_t config_size)
{

	if (cb == NULL || name == NULL)
		return -EINVAL;

	LIBDISTEXEC_LL_INIT_IF_NULL(execute);

	libdistexec_callback_execute_t *exec =
	    malloc(sizeof(libdistexec_callback_execute_t));

	if (NULL == exec)
		return -ENOMEM;

	LOG_DEBUG("registering execute callback %s", name);

	exec->call = cb;

	exec->callback.init = init;
	exec->callback.terminate = terminate;

	if (NULL != init)
		if (init() != 0) {
			free(exec);
			return -1;
		}

	exec->callback.name = strdup(name);
	exec->callback.config.set = set_config;
	exec->callback.config.values = config_values;
	exec->callback.config.len = config_size;
	exec->callback.plugin = current_plugin;

	current_plugin->callbacks[current_plugin->callbacks_s++] =
	    &(exec->callback);

	LIBDISTEXEC_LL_APPEND(execute, exec);
	return 0;
}

EXPORT int libdistexec_call_callback_collect(const char *filter)
{
	int rc;
	LIBDISTEXEC_LL_EACH(collect, libdistexec_callback_collect_t *, cb) {
		rc = cb->call(filter);
		if (rc)
			LOG_ERROR("callback error %s: %d", cb->callback.name,
				  rc);
	}
	LIBDISTEXEC_LL_END();

	return 0;
}

EXPORT int libdistexec_register_callback_collect(const char *name,
						 int (*cb) (const char *),
						 int (*init) (),
						 int (*terminate) (),
						 int (*set_config) (const char
								    *,
								    const char
								    *),
						 const char **config_values,
						 size_t config_size)
{

	if (cb == NULL || name == NULL)
		return -EINVAL;

	LIBDISTEXEC_LL_INIT_IF_NULL(collect);

	libdistexec_callback_collect_t *coll =
	    malloc(sizeof(libdistexec_callback_collect_t));

	if (NULL == coll)
		return -ENOMEM;

	LOG_DEBUG("registering collect callback %s", name);
	coll->call = cb;

	coll->callback.init = init;
	coll->callback.terminate = terminate;

	if (NULL != init)
		if (init() != 0) {
			free(coll);
			return -1;
		}

	coll->callback.name = strdup(name);
	coll->callback.config.set = set_config;
	coll->callback.config.values = config_values;
	coll->callback.config.len = config_size;
	coll->callback.plugin = current_plugin;

	current_plugin->callbacks[current_plugin->callbacks_s++] =
	    &(coll->callback);

	LIBDISTEXEC_LL_APPEND(collect, coll);
	return 0;
}

EXPORT libdistexec_plugin_t *libdistexec_backend_get(const char *name)
{
	if (LIBDISTEXEC_LL_COUNT(backends) == 0)
		LIBDISTEXEC_ABORT(-1, "No backends loaded.");

	LIBDISTEXEC_LL_EACH(backends, libdistexec_plugin_t *, p) {
		if (NULL == p)
			continue;
		if (strcmp(p->name, name) == 0) {
			return p;
		}
	}
	LIBDISTEXEC_LL_END();
	return NULL;
}

EXPORT libdistexec_plugin_t *libdistexec_frontend_get(const char *name)
{
	if (LIBDISTEXEC_LL_COUNT(frontends) == 0)
		LIBDISTEXEC_ABORT(-1, "No frontends loaded.");

	LIBDISTEXEC_LL_EACH(frontends, libdistexec_plugin_t *, p) {
		if (NULL == p)
			continue;

		if (strcmp(p->name, name) == 0) {
			return p;
		}
	}
	LIBDISTEXEC_LL_END();
	return NULL;

}

EXPORT int libdistexec_plugin_config(libdistexec_plugin_t * p,
				     const char *config[], size_t size)
{
	int i;
	for (i = 0; i < size; i++) {
		char *saveptr;
		const char *key = strtok_r((char *)config[i], "=", &saveptr);
		if (NULL == key)
			LIBDISTEXEC_ABORT(-1, "key must not be NULL");

		const char *value = strtok_r(NULL, "\0", &saveptr);
		if (p->config->set(key, value))
			LIBDISTEXEC_ABORT(-1, "Could not set %s => %s for %s",
					  key, value, p->name);

	}
	return 0;
}

EXPORT libdistexec_llist_t *libdistexec_frontends()
{
	return LIBDISTEXEC_LL_EXPAND(frontends);
}

EXPORT libdistexec_llist_t *libdistexec_backends()
{
	return LIBDISTEXEC_LL_EXPAND(backends);
}

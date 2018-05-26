
/*
 * Copyright (C) 2016-2017 Konrad Lother <k@hiddenbox.org>
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
 * @date	01/10/2017
 * @file	util.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_UTIL_H
#define DISTEXEC_UTIL_H

#include <stdlib.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "distexec/logger.h"
#include "distexec/export.h"
#include "distexec/error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _EXPAND(prefix, key) _libdistexec_exp_##prefix##_##key

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


// DLIBDISTEXEC_LL/SO begin
EXPORT void *libdistexec_dlopen(const char *path);
EXPORT int libdistexec_dlclose(void *handle);
EXPORT void *libdistexec_dlsym(void *handle, const char *symbol);
// DLIBDISTEXEC_LL/SO end


// LIST begin
#define LIBDISTEXEC_LIST_STEP 2048   /*  values < 2048 take ~15msec on my system
                                to init the list. >= 2048 takes 0msec... */

typedef struct _libdistexec_list {
	void *values;
	size_t used;
	size_t size;
    size_t step;
} libdistexec_list_t;

typedef struct _libdistexec_llist libdistexec_llist_t;

typedef struct _libdistexec_llist {
	void *value;
	libdistexec_llist_t *next;
	libdistexec_llist_t *prev;
} libdistexec_llist_t;

EXPORT libdistexec_llist_t *libdistexec_llist();
EXPORT void *libdistexec_llist_pop(libdistexec_llist_t *l);
EXPORT void *libdistexec_llist_shift(libdistexec_llist_t **l);
EXPORT int libdistexec_llist_push(libdistexec_llist_t **l, void * val);
EXPORT int libdistexec_llist_append(libdistexec_llist_t *l, void * val);
EXPORT int libdistexec_llist_count(libdistexec_llist_t *l);

#define LIBDISTEXEC_LL_EXPAND(key) _EXPAND(llist, key)

#define LIBDISTEXEC_LL(key) \
	static libdistexec_llist_t *LIBDISTEXEC_LL_EXPAND(key) = NULL; \
	static libdistexec_llist_t *LIBDISTEXEC_LL_EXPAND(key##_current) = NULL;

#define LIBDISTEXEC_LL_INIT(key) LIBDISTEXEC_LL_EXPAND(key) = libdistexec_llist();
#define LIBDISTEXEC_LL_INIT_IF_NULL(key) \
	if (LIBDISTEXEC_LL_EXPAND(key) == NULL && LIBDISTEXEC_LL_EXPAND(key##_current) == NULL) \
		LIBDISTEXEC_LL_INIT(key);
#define LIBDISTEXEC_LL_PUSH(key, value) libdistexec_llist_push(&LIBDISTEXEC_LL_EXPAND(key), (void*) value);
// segfaults
#define LIBDISTEXEC_LL_APPEND(key, value) \
	if (NULL == value) \
		LOG_WARN("Adding NULL to llist"); \
	libdistexec_llist_append(LIBDISTEXEC_LL_EXPAND(key), (void*) value);
//#define LIBDISTEXEC_LL_APPEND(key, value) LIBDISTEXEC_LL_PUSH(key, value); /* dirty workaround */
#define LIBDISTEXEC_LL_SHIFT(key, type) (type) libdistexec_llist_shift(&LIBDISTEXEC_LL_EXPAND(key));
#define LIBDISTEXEC_LL_POP(key, type) (type) libdistexec_llist_pop(LIBDISTEXEC_LL_EXPAND(key));
#define LIBDISTEXEC_LL_EACH(key, type, elem) \
	LIBDISTEXEC_LL_EXPAND(key##_current) = LIBDISTEXEC_LL_EXPAND(key); \
	if (LIBDISTEXEC_LL_EXPAND(key##_current) != NULL) \
	while(LIBDISTEXEC_LL_EXPAND(key##_current)->next != NULL) { \
		type elem = (type) LIBDISTEXEC_LL_EXPAND(key##_current)->value; \
		libdistexec_llist_t *__current = LIBDISTEXEC_LL_EXPAND(key##_current); \
		LIBDISTEXEC_LL_EXPAND(key##_current) = LIBDISTEXEC_LL_EXPAND(key##_current)->next; \
		if (NULL == elem) { \
			LOG_ERROR("Got NULL element, sucks!"); \
			continue; \
		}

#define LIBDISTEXEC_LL_END() }
#define LIBDISTEXEC_LL_COUNT(key) libdistexec_llist_count(LIBDISTEXEC_LL_EXPAND(key))

#define libdistexec_list_init(lst, type, len) \
	lst = malloc(sizeof(libdistexec_list_t)); \
	lst->values = (type *) malloc(len * sizeof(type)); \
	lst->used = 0; \
    lst->step = len >= 1 ? len : LIBDISTEXEC_LIST_STEP; \
	lst->size = len;

#define libdistexec_list_add(lst, type, elem) \
	if (lst->used == lst->size) { \
		lst->size += lst->step; \
		lst->values = (type *) realloc(lst->values, lst->size * sizeof(type)); \
	} \
	((type *)lst->values)[lst->used++] = elem;

#define libdistexec_list_get(lst, type, index) \
	((type *)lst->values)[index]

#define libdistexec_list_each(lst, type, elem, counter) \
	for(counter = 0; counter < lst->used; counter++) { \
		type elem = libdistexec_list_get(lst, type, counter); \

#define libdistexec_end }

#define libdistexec_list_count(lst) lst->used

#define libdistexec_list_free(lst) \
	free(lst->values); \
	free(lst);

// LIST end

EXPORT void libdistexec_sleep(int ms);
EXPORT uint64_t libdistexec_tstamp_msec();
EXPORT uint64_t libdistexec_tstamp_usec();

typedef struct _perfmon {
	char *name;
	uint64_t start_time;
	uint64_t end_time;
	uint64_t duration;
} libdistexec_perfmon_t;

EXPORT size_t libdistexec_find_file(const char *path,
		                              const char *pattern, char **results, size_t rlen, int flag);

EXPORT libdistexec_llist_t *libdistexec_frontends();
EXPORT libdistexec_llist_t *libdistexec_backends();

EXPORT int libdistexec_pcre_str_match(const char *str, const char *pattern);
#ifdef __cplusplus
}
#endif

#endif

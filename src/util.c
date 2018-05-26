
/*
 * this file is part of distexec
 *
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

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include <dirent.h>
#include <unistd.h>
#include <pcre.h>
#include "distexec/logger.h"
#include "distexec/export.h"
#include "distexec/error.h"
#include "distexec/util.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <shlwapi.h>
#else
#include <fnmatch.h>
#endif

// DLL/SO begin
EXPORT void *libdistexec_dlopen(const char *path)
{
	void *handle = NULL;

#ifdef _WIN32
	handle = LoadLibrary(path);
	if (NULL == handle) {
		LIBDISTEXEC_ERROR(-1, "could not load dynamic library %s: %s",
				  path,
				  libdistexec_error_str(libdistexec_errno()));
#else
	handle = dlopen(path, RTLD_LAZY);
	if (NULL == handle) {
		LIBDISTEXEC_ERROR(-1, "could not load dynamic library %s: %s",
				  path, dlerror());
#endif

	}
	return handle;
}

EXPORT int libdistexec_dlclose(void *handle)
{
	int ret;
#ifdef _WIN32

#else
	ret = dlclose(handle);
#endif
	return ret;
}

EXPORT void *libdistexec_dlsym(void *handle, const char *symbol)
{
	void *ref;

#ifdef _WIN32
	ref = GetProcAddress((HINSTANCE) handle, symbol);
#else
	ref = dlsym(handle, symbol);
#endif

	if (NULL == ref) {
		LIBDISTEXEC_ERROR(libdistexec_errno(),
				  "could not load symbol %s: %s", symbol,
				  libdistexec_error_str(libdistexec_errno()));
		return NULL;
	}

	return ref;
}

// DLL/SO end

EXPORT void libdistexec_sleep(int ms)
{
#ifdef _WIN32
	timeBeginPeriod(1);
	Sleep(ms);
	timeEndPeriod(1);
#else
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#endif
}

EXPORT uint64_t libdistexec_tstamp_msec()
{
	return libdistexec_tstamp_usec() / 1000;
}

EXPORT uint64_t libdistexec_tstamp_usec()
{
	uint64_t res;
	uint64_t sec;
	long int usec;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec;
	usec = tv.tv_usec;

	sec *= 1000000.0;

	res = sec + usec;

	return res;
}

EXPORT libdistexec_llist_t *libdistexec_llist()
{
	libdistexec_llist_t *l = malloc(sizeof(libdistexec_llist_t));

	if (l == NULL) {
		errno = ENOMEM;
		//LIBDISTEXEC_ABORT(-1, "could not create list, malloc failed.");
		return NULL;
	}

	l->next = NULL;
	l->prev = NULL;
	l->value = NULL;

	return l;
}

EXPORT void *libdistexec_llist_pop(libdistexec_llist_t * l)
{
	if (l == NULL) {
		LIBDISTEXEC_ERROR(-1, "list must not be NULL");
		return NULL;
	}

	void *retval = NULL;
	libdistexec_llist_t *current = NULL;

	if (l->next == NULL) {
		retval = l->value;
		free(l);
		l = NULL;
		return retval;
	}

	current = l;
	while (current->next->next != NULL)
		current = current->next;

	retval = current->next->value;
	free(current->next);
	current->next = NULL;

	return retval;
}

EXPORT void *libdistexec_llist_shift(libdistexec_llist_t ** l)
{
	void *retval = NULL;
	libdistexec_llist_t *next_node = NULL;

	if (*l == NULL)
		return NULL;

	next_node = (*l)->next;
	retval = (*l)->value;
	free(*l);
	*l = next_node;
	return retval;
}

EXPORT int libdistexec_llist_push(libdistexec_llist_t ** l, void *val)
{
	libdistexec_llist_t *new_node = libdistexec_llist();
	new_node->value = val;
	new_node->next = *l;
	*l = new_node;
	return 0;
}

EXPORT int libdistexec_llist_append(libdistexec_llist_t * l, void *val)
{
	libdistexec_llist_t *new_list = libdistexec_llist();
	if (NULL == new_list) {
		errno = ENOMEM;
		return -ENOMEM;
	}

	libdistexec_llist_t *current = l;

	while (current->next != NULL)
		current = current->next;

	new_list->value = val;
	new_list->next = NULL;
	new_list->prev = current;
	current->next = new_list;
	return 0;
}

EXPORT int libdistexec_llist_count(libdistexec_llist_t * l)
{
	int counter = 0;
	libdistexec_llist_t *current = l;
	while (current->next != NULL) {
		current = current->next;
		counter++;
	}
	return counter;
}

EXPORT size_t libdistexec_find_file(const char *path,
				    const char *pattern, char **results,
				    size_t rlen, int flag)
{
	size_t len = 0;
	struct dirent *dirent;
	DIR *dir;
	int i;
	if (strlen(path) == 0)
		return 0;

	dir = opendir(path);

	if (NULL == dir)
		return 0;

	while ((dirent = readdir(dir)) != NULL) {
		if (strcmp(dirent->d_name, ".") == 0
		    || strcmp(dirent->d_name, "..") == 0)
			continue;

#if defined(_WIN32)
		if (PathMatchSpec(dirent->d_name, pattern) == 1) {
#else
		if (fnmatch(pattern, dirent->d_name, 0) == 0) {
#endif
			char tmp[strlen(path) + strlen(dirent->d_name) + 2];
			sprintf(tmp, "%s/%s", path, dirent->d_name);
			results[len] = strdup(tmp);
			len++;
		}

		if (flag == 1 && len == 1) {
			closedir(dir);
			break;
		}
	}
	closedir(dir);
	return len;
}

EXPORT int libdistexec_pcre_str_match(const char *str, const char *pattern)
{
	pcre *rc;
	return 0;
}

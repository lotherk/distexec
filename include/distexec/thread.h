
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






#ifndef DISTEXEC_THREAD_H
#define DISTEXEC_THREAD_H

#include "distexec/export.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef HANDLE libdistexec_mutex_t;
typedef HANDLE libdistexec_thread_t;
#else
typedef pthread_mutex_t libdistexec_mutex_t;
typedef pthread_t libdistexec_thread_t;
#endif

EXPORT int libdistexec_mutex_lock(libdistexec_mutex_t * m);
EXPORT int libdistexec_mutex_unlock(libdistexec_mutex_t * m);
EXPORT int libdistexec_mutex_trylock(libdistexec_mutex_t * m);
EXPORT int libdistexec_mutex_init(libdistexec_mutex_t * m);

EXPORT int libdistexec_thread_create(libdistexec_thread_t * t, void *func(void *), void *data);
EXPORT int libdistexec_thread_join(libdistexec_thread_t t);
EXPORT int libdistexec_thread_cancel(libdistexec_thread_t t);

#ifdef __cplusplus
}
#endif

#endif


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

#include "distexec/thread.h"
#include "distexec/util.h"

EXPORT int libdistexec_mutex_lock(libdistexec_mutex_t * m)
{
	int res;
#if defined(_WIN32)
	res = WaitForSingleObject(*m, INFINITE);
#else
	res = pthread_mutex_lock(m);
#endif
	return res;
}

EXPORT int libdistexec_mutex_unlock(libdistexec_mutex_t * m)
{
	int res;
#if defined(_WIN32)
	res = ReleaseMutex(*m);
	res = res == 1 ? 0 : res;
#else
	res = pthread_mutex_unlock(m);
#endif
	return res;
}

EXPORT int libdistexec_mutex_init(libdistexec_mutex_t * m)
{
	int res;
#if defined(_WIN32)
	*m = CreateMutex(NULL, FALSE, NULL);
#else
	res = pthread_mutex_init(m, NULL);
#endif
	return res;
}

EXPORT int libdistexec_thread_create(libdistexec_thread_t * t,
				     void *func(void *), void *data)
{
	int res = 0;
#if defined(_WIN32)
	DWORD threadid;
	*t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) func, data, 0,
			  &threadid);
#else
	res = pthread_create(t, NULL, func, data);
#endif
	return res;
}

EXPORT int libdistexec_thread_join(libdistexec_thread_t t)
{
	int res;
#if defined(_WIN32)
	res = WaitForSingleObject(t, INFINITE);
#else
	res = pthread_join(t, NULL);
#endif
	return res;
}

EXPORT int libdistexec_mutex_trylock(libdistexec_mutex_t * m)
{
	int res;
#if defined(_WIN32)
	res = WaitForSingleObject(*m, 0);
#else
	res = pthread_mutex_trylock(m);
#endif
	return res;
}

EXPORT int libdistexec_thread_cancel(libdistexec_thread_t t)
{
	int res;
#if defined(_WIN32)
	res = TerminateThread(t, 0);
	res = res != 0 ? 0 : res;
#else
	res = pthread_cancel(t);
#endif
	return res;
}

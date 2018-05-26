
/* popen3.h: ADD DESCRIPTION HERE
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
 * @date	02/17/2017
 * @file	popen3.h
 * @author	Konrad Lother
 */

#ifndef DISTEXEC_POPEN3_H
#define DISTEXEC_POPEN3_H

#include <unistd.h>
#include <sys/types.h>

#include "distexec/export.h"

#ifdef __cplusplus
extern "C" {
#endif
EXPORT pid_t libdistexec_popen3(int *pipes, const char *command, char *const argv[]);

EXPORT int libdistexec_pclose3(pid_t pid, int *pipes);

EXPORT int libdistexec_waitpid(int pid);
#ifdef __cplusplus
}
#endif
#endif

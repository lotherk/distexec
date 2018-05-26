
/* popen3.c: ADD DESCRIPTION HERE
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
#include <unistd.h>
#include <sys/types.h>
#ifdef _WIN32

#else
#include <sys/wait.h>
#endif

#include "distexec/popen3.h"
#include "distexec/error.h"

EXPORT pid_t libdistexec_popen3(int *pipes, const char *command,
				char *const argv[])
{
#ifdef _WIN32
	LIBDISTEXEC_ABORT(-1, "NOT IMPLEMENTED");
#else
	int in[2];
	int out[2];
	int err[2];
	pid_t pid;

	if (pipe(in) != 0)
		LIBDISTEXEC_ABORT(-1, "pipe(in) failed");

	if (pipe(out) != 0)
		LIBDISTEXEC_ABORT(-1, "pipe(out) failed");

	if (pipe(err) != 0)
		LIBDISTEXEC_ABORT(-1, "pipe(err) failed");

	pid = fork();

	switch (pid) {
	case -1:
		LIBDISTEXEC_ABORT(-1, "fork() failed");
	case 0:
		close(in[1]);
		close(out[0]);
		close(err[0]);

		dup2(in[0], 0);
		dup2(out[1], 1);
		dup2(err[1], 2);

		pipes[0] = in[1];
		pipes[1] = out[0];
		pipes[2] = err[0];
		if (execv(command, argv) != 0)
			LIBDISTEXEC_ABORT(-1, "error %i: %s",
					  libdistexec_errno(),
					  libdistexec_error_str
					  (libdistexec_errno()));

		return -1;
	default:
		close(in[0]);
		close(out[1]);
		close(err[1]);

		pipes[0] = in[1];
		pipes[1] = out[0];
		pipes[2] = err[0];
		return pid;
	}

#endif
	return -1;
}

EXPORT int libdistexec_pclose3(pid_t pid, int *pipes)
{
#ifdef _WIN32
	LIBDISTEXEC_ABORT(-1, "NOT IMPLEMENTED");
#else

#endif
	return 0;
}

EXPORT int libdistexec_waitpid(int pid)
{
#ifdef _WIN32
	LIBDISTEXEC_ABORT(-1, "NOT IMPLEMENTED");
#else
	int status;
	waitpid(pid, &status, 0);
#endif
	return 0;
}

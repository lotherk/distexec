
/* test-popen3.c: ADD DESCRIPTION HERE
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
#include <stdio.h>

#include "distexec/popen3.h"
#include "distexec/error.h"
#include "distexec/logger.h"
#include "distexec/util.h"

int main(void) {

	libdistexec_logger_init(stdout, stdout);
	libdistexec_logger_set_level(LALL);
	int pipes[3];
	const char *command = "../tests/test-popen3.sh";
	char *const arg[]     = { "distexec", NULL };

	int r = libdistexec_popen3(pipes, command, arg);
	printf("test success %i\n", r);
	//libdistexec_sleep(1000);
	FILE *in = fdopen(pipes[0], "w");
	FILE *out = fdopen(pipes[1], "r");
	FILE *err = fdopen(pipes[2], "r");
	if (NULL == in)
		LIBDISTEXEC_ABORT(-1, "fdopen failed");

	if (NULL == out)
		LIBDISTEXEC_ABORT(-1, "out failed");

	char teststr[] = "this is a test\n";
	fwrite(teststr, 1, sizeof(teststr), in);
	fflush(in);
	char outbuf[1024];
	char *r1 = fgets(outbuf, 1024, out) ;
	printf("%s r1 stdout: '%s'\n", r1, outbuf);

	outbuf[0] = '\0';
	char *r2 = fgets(outbuf, 1024, out) ;
	printf("%s r2 stdout: '%s'\n", r2, outbuf);

	char teststr1[] = "teststr1\n";
	fwrite(teststr1, 1, sizeof(teststr1), in);
	fflush(in);

	outbuf[0] = '\0';
	char *r3 = fgets(outbuf, 1024, out) ;
	printf("%s r3 stdout: '%s'\n", r3, outbuf);

	char errbuf[1024];
	fgets(errbuf, 1024, err) ;
	printf("stderr: '%s'\n", errbuf);
	libdistexec_waitpid(r);
	return 0;
}

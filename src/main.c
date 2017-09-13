
/* main.c: ADD DESCRIPTION HERE
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
#include <errno.h>

#include "distexec/logger.h"

#include "cmdline.h"

static logger_t logger;

int main(int argc, char *argv[])
{
    int rc;

    /* logger initialization */
    rc = logger_init(&logger);
    if (0 != rc) {
        fprintf(stderr, "Error while initializing logger: %s\n", strerror(errno));
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    logger.out = stdout;
    logger.err = stderr;
    logger.name = "core";

    /* command line parsing */

	/* initialize gengetopt parser */
	struct cmdline_parser_params *params;
	struct gengetopt_args_info args_info;
	params = cmdline_parser_params_create();

	if (cmdline_parser(argc, argv, &args_info) != 0)
		exit(EXIT_FAILURE);

	/* potential end-less loop */
	if (args_info.config_given > 0) {
		params->initialize = 0;
		params->override = 1;
		int i;
		for (i = 0; i < args_info.config_given; i++) {
			params->initialize = 0;
			params->override = 1;
			if (cmdline_parser_config_file
			    (args_info.config_arg[i], &args_info, params)
			    != 0)
				exit(EXIT_FAILURE);
		}
	}




    return EXIT_SUCCESS;
}

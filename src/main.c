
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

#include "config.h"
#include "distexec/error.h"
#include "distexec/node.h"
#include "distexec/logger.h"
#include "distexec/thread.h"
#include "distexec/util.h"
#include "distexec/plugin.h"
#include "distexec/distexec.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

static libdistexec_plugin_t *backend;
static libdistexec_plugin_t *frontend;

static libdistexec_config_t libdistexec_cfg;

static libdistexec_mutex_t out_mutex;

LIBDISTEXEC_LL(plugins);
LIBDISTEXEC_LL(plugin_path);

/* this file is generated by cmake from cmdline.ggo.in */
#include "copa.h"		// comandline parser

#if defined(_WIN32)
#define LIB_SUFFIX "dll"
#elif defined(__APPLE__)
#define LIB_SUFFIX "dylib"
#else
#define LIB_SUFFIX "so"
#endif

copa_parser_t *cmd_parser;
copa_opt_t *help_opt, *version_opt, *config_opt, *output_opt, *progress_opt,
    *plugin_path_opt, *plugin_opt, *grep_opt, *timeout_opt,
    *concurrent_opt, *debug_opt;

copa_opt_t *log_file_opt, *log_format_opt, *log_format_date_opt,
    *log_format_time_opt;

#define REQUIRE_PARAM(opt, param) \
	if (opt->values_s == 0) { \
		fprintf(stderr, "error: argument %s requires parameter (see --help)\n", param); \
		fflush(stderr); \
		exit(EXIT_FAILURE); \
	}

#define REQUIRE_ARGUMENT(arg, name) \
	if (! arg->given) { \
		fprintf(stderr, "error: argument %s is required (see --help)\n", name); \
		fflush(stderr); \
		exit(EXIT_FAILURE); \
	}

static libdistexec_plugin *load_plugin(const char *name, const char *path);
static int autoload_plugins(const char *path[], size_t size);
static int yield_handler_output(libdistexec_node_t * n, const char *s, int i);

static void help_opt_cb(copa_opt_t * o)
{
#define H(...) fprintf(stdout, __VA_ARGS__)

	H("distexec %s Copyright (C) 2017 Konrad Lother <k@hiddenbox.org>\n",
	  PROJECT_VERSION);
	H("\n");
	H("This software is supplied WITHOUT ANY WARRANTY; without even the implied\n");
	H("warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. This is\n");
	H("free software, and you are welcome to redistribute it under certain\n");
	H("conditions; see the file COPYING for details.\n");
	H("\n\n");
	H("Usage: distexec [OPTIONS]... -- command...\n");
	H("\n");
	H("General:\n");
	H("  -h, --help                Print this text\n");
	H("  -V, --version             Print version\n");
	H("  -C, --config              Configuration file\n");
	H("  -O, --output              Print output\n");
	H("  -P, --progress            Print progressbar\n");
	H("\n\n");
	H("Plugin:\n");
	H("  --plugin-path=PATH        Path to search for plugins\n");
	H("                            (default: %s)\n", PLUGIN_PATH_DEFAULT);
	H("  -p, --plugin=NAME         Plugin to load\n");
	H("  --help-plugin=NAME        Print config parameters for plugin\n");
	H("\n");
	H("Read to documentation of the plugin(s) you want to load\n");
	H("and supply config parameters (--PLUGINNAME-KEY=VALUE) if needed.\n");
	H("\n\n");
	H("Output filtering:\n");
	H("  -g, --grep=REGEX          Match output against REGEX\n");
	H("\n\n");
	H("Execution:\n");
	H("  -c, --concurrent=INT      Set number of concurrent executions (default: %d)\n", CONCURRENT_DEFAULT);
	H("\n\n");
	H("Logging:\n");
	H("  -L, --log-file=FILE       Log to file\n");
	H("  --log-format              Log format\n");
	H("                            (default: %s)\n", LOGGER_FORMAT_DEFAULT);
	H("  --log-format-date         Log format date\n");
	H("                            (default: %s)\n", LOGGER_FORMAT_DATE);
	H("  --log-format-time         Log format time\n");
	H("                            (default: %s)\n", LOGGER_FORMAT_TIME);
	H("  --help-log-format         Print log format help\n");
	H("\n\n");
	H("Other:\n");
	H("  --debug                   Enable debug messages (default: off)\n");
	H("\n\n");
	H("See the distexec man page for examples.\n");

#undef H
	fflush(stdout);
	exit(EXIT_SUCCESS);
}

static void version_opt_cb(copa_opt_t * o)
{
	fprintf(stdout, "distexec %s Copyright (C) Konrad Lother\n",
		PROJECT_VERSION);
	fflush(stdout);
	exit(EXIT_SUCCESS);
}

static void plugin_path_opt_cb(copa_opt_t * o)
{
	REQUIRE_PARAM(o, "--plugin-path");

	LIBDISTEXEC_LL_INIT_IF_NULL(plugin_path);
	int i;
	for (i = 0; i < o->values_s; i++)
		LIBDISTEXEC_LL_APPEND(plugin_path, o->values[i]);
}

static void plugin_opt_cb(copa_opt_t * o)
{
	REQUIRE_PARAM(o, "--plugin");
	LIBDISTEXEC_LL_INIT_IF_NULL(plugins);

	int rc;
	libdistexec_plugin *plugin;
	int i;
	for (i = 0; i < o->values_s; i++) {
		char *value = o->values[i];
		LOG_DEBUG("attempting to load plugin %s", value);
		LIBDISTEXEC_LL_EACH(plugin_path, const char *, p) {
			if (NULL == p)
				continue;

			LOG_DEBUG("checking %s", p);
			plugin = load_plugin(value, p);
			if (NULL != plugin) {
				LOG_DEBUG("Plugin %s loaded from %s", value, p);
				break;
			}
		}
		LIBDISTEXEC_LL_END();

		if (NULL != plugin) {
			int i;
			for (i = 0; i < plugin->callbacks_s; i++) {
				libdistexec_callback_t *cb =
				    plugin->callbacks[i];
				libdistexec_plugin_config_t config = cb->config;

				if (NULL == config.values)
					continue;

				int n;
				LOG_DEBUG("callback name: %s", cb->name);

				for (n = 0; n < config.len; n++) {
					copa_parser_t *parser = copa_parser();
					if (NULL == parser)
						LIBDISTEXEC_ABORT(ENOMEM,
								  "nomem, urgh");

					size_t obuf_len =
					    strlen(cb->name) +
					    strlen(config.values[n]) + 2;

					char obuf_name[obuf_len];
					snprintf(obuf_name, obuf_len, "%s-%s",
						 cb->name, config.values[n]);

					copa_opt_t *opt =
					    copa_opt(parser, 0, obuf_name, NULL,
						     NULL);
					opt->has_param = 1;
					copa_parse(parser, o->parser->unknown,
						   o->parser->unknown_s);

					if (opt->given) {
						printf("set %s => %s\n",
						       config.values[n],
						       opt->values[opt->
								   values_s -
								   1]);
						config.set(config.values[n],
							   opt->values[opt->
								       values_s
								       - 1]);
					}

					copa_free(parser);
				}

			}
		}
	}
}

static void concurrent_opt_cb(copa_opt_t * o)
{
	REQUIRE_PARAM(o, "--concurrent");

	// FIXME: use strtol
	libdistexec_cfg.concurrent = atoi(o->values[o->values_s - 1]);
}

static void config_opt_cb(copa_opt_t * o)
{
	REQUIRE_PARAM(o, "--config");
}

int main(int argc, char *argv[])
{
	int rc;
	int i;

	/* we need arguments. */
	if (argc <= 1) {
		fprintf(stderr, "See --help (or --detailed-help)\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}

	cmd_parser = copa_parser();

	/* general options */
	help_opt = copa_opt(cmd_parser, 'h', "help", NULL, help_opt_cb);
	version_opt =
	    copa_opt(cmd_parser, 'V', "version", NULL, version_opt_cb);
	config_opt = copa_opt(cmd_parser, 'C', "config", NULL, config_opt_cb);
	output_opt = copa_opt(cmd_parser, 'O', "output", NULL, NULL);
	progress_opt = copa_opt(cmd_parser, 'P', "progress", NULL, NULL);

	/* plugin options */
	plugin_path_opt =
	    copa_opt(cmd_parser, 0, "plugin-path", NULL, plugin_path_opt_cb);
	plugin_path_opt->has_param = 1;
	plugin_opt = copa_opt(cmd_parser, 'p', "plugin", NULL, plugin_opt_cb);
	plugin_opt->has_param = 1;

	/* output options */
	grep_opt = copa_opt(cmd_parser, 'g', "grep", NULL, NULL);
	grep_opt->has_param = 1;

	/* execution options */
	concurrent_opt =
	    copa_opt(cmd_parser, 'c', "concurrent", NULL, concurrent_opt_cb);
	concurrent_opt->has_param = 1;

	/* logging options */
	log_file_opt = copa_opt(cmd_parser, 'L', "log-file", NULL, NULL);
	log_file_opt->has_param = 1;
	log_format_opt = copa_opt(cmd_parser, 0, "log-format", NULL, NULL);
	log_format_opt->has_param = 1;
	log_format_date_opt =
	    copa_opt(cmd_parser, 0, "log-format-date", NULL, NULL);
	log_format_date_opt->has_param = 1;
	log_format_time_opt =
	    copa_opt(cmd_parser, 0, "log-format-time", NULL, NULL);
	log_format_time_opt->has_param = 1;

	/* other options */
	debug_opt = copa_opt(cmd_parser, 0, "debug", NULL, NULL);

	rc = copa_parse(cmd_parser, argv, argc);

	REQUIRE_ARGUMENT(plugin_opt, "--plugin");

	if (!plugin_path_opt->given)
		plugin_path_opt->values[plugin_path_opt->values_s++] =
		    PLUGIN_PATH_DEFAULT;
	plugin_path_opt_cb(plugin_path_opt);

	if (!concurrent_opt->given)
		libdistexec_cfg.concurrent = CONCURRENT_DEFAULT;

	/* set logfile, if given */
	if (log_file_opt->given) {
		FILE *logfile =
		    fopen(log_file_opt->values[log_file_opt->values_s - 1],
			  "w+");
		if (NULL == logfile)
			LIBDISTEXEC_ABORT(libdistexec_errno(),
					  "Could not open logfile %s: %s",
					  log_file_opt->values[log_file_opt->
							       values_s - 1],
					  libdistexec_error_str
					  (libdistexec_errno()));
		libdistexec_logger_init(logfile, logfile);
	} else {
		libdistexec_logger_init(stdout, stderr);
	}

	/* set log format, if given */
	if (log_format_opt->given) {
		REQUIRE_ARGUMENT(log_format_opt, "--log-format");
		libdistexec_logger_set_format((char *)log_format_opt->
					      values[log_format_opt->values_s -
						     1]);
	}

	if (log_format_date_opt->given) {
		REQUIRE_ARGUMENT(log_format_date_opt, "--log-format-date");
		libdistexec_logger_set_format_date((char *)log_format_date_opt->
						   values[log_format_date_opt->
							  values_s - 1]);
	}

	if (log_format_time_opt->given) {
		REQUIRE_ARGUMENT(log_format_time_opt, "--log-format-time");
		libdistexec_logger_set_format_time((char *)log_format_time_opt->
						   values[log_format_time_opt->
							  values_s - 1]);
	}

	/* enable debug? */
	if (debug_opt->given)
		libdistexec_logger_set_level(LALL);

	copa_callback(cmd_parser);

	for (i = 0; i < cmd_parser->unknown_s; i++) {
		LOG_DEBUG("unknown %d: %s", i, cmd_parser->unknown[i]);
	}
	if (libdistexec_init())
		LIBDISTEXEC_ABORT(-1, "Could not initialize libdistexec");

	libdistexec_register_yield_handler(&yield_handler_output);

	LOG_DEBUG("call collect");
	rc = libdistexec_collect(NULL);
	LOG_DEBUG("return: %d", rc);

	/* build command to run */
	char *cmd_buf = NULL;

	if (cmd_parser->arguments_s > 0) {
		cmd_buf = calloc(1024, sizeof(char));
		for (i = 0; i < cmd_parser->arguments_s; i++) {
			char *s = cmd_parser->arguments[i];
			sprintf(cmd_buf, "%s %s", cmd_buf, s);
		}
		(void)*(cmd_buf++);	// skipping first whitespace, meh...
	} else {
		LIBDISTEXEC_ABORT(-1, "no command given");
	}

	rc = libdistexec_execute(&libdistexec_cfg, cmd_buf);

	if (rc)
		LIBDISTEXEC_ABORT(-1, "libdistexec_execute failed");

	LIBDISTEXEC_LL_EACH(plugins, libdistexec_plugin *, p) {
		p->unload();
	}

	LIBDISTEXEC_LL_END();
	return 0;
}

static libdistexec_plugin *load_plugin(const char *name, const char *path)
{

	size_t plen = strlen(name) + 2 + strlen(LIB_SUFFIX);
	char pattern[plen];
	snprintf(pattern, plen, "%s.%s", name, LIB_SUFFIX);

	LOG_DEBUG("pattern: %s\n", pattern);
	size_t rlen = 1024;
	char *results[rlen];
	size_t r = libdistexec_find_file(path, pattern, results, rlen, 0);
	if (0 == r)
		return NULL;

	int i;
	for (i = 0; i < r; i++) {
		libdistexec_plugin *p = libdistexec_plugin_open(results[i]);
		if (NULL == p)
			continue;

		LIBDISTEXEC_LL_APPEND(plugins, p);
		return p;
	}
	return NULL;

}

static int yield_handler_output(libdistexec_node_t * n, const char *s, int i)
{
	FILE *out = NULL;
	if (i == 0)
		out = stdout;
	else if (i == 1)
		out = stderr;
	else
		LIBDISTEXEC_ABORT(-1, "%d not valid output stream", i);

	fprintf(out, "[%d] %s: %s", i, n->hostname, s);
	fflush(out);

	return 0;
}

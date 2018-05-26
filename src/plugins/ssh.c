
/* ssh.c: ADD DESCRIPTION HERE
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
#include <stdlib.h>
#include <unistd.h>
#include <libssh2.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#endif
#include <fcntl.h>

#include "distexec/export.h"
#include "distexec/distexec.h"
#include "distexec/plugin.h"
#include "distexec/logger.h"
#include "distexec/util.h"

static libdistexec_logger_t logger;
static int exec(libdistexec_node_t * node, const char *command);

static struct {
	const char *username;
	const char *password;
	const char *known_hosts;
	const char *private_key;
	const char *public_key;
	const char *auth;
} cfg;

static const char *config_values[] = {
	"username",
	"known_hosts",
	"private_key",
	"public_key",
	"auth",
};

static int set_config_value(const char *key, const char *value)
{
	LOG_DEBUG("Setting %s = %s", key, value);
	if (strcmp(key, "username") == 0)
		cfg.username = strdup(value);
	else if (strcmp(key, "password") == 0)
		cfg.password = strdup(value);
	else if (strcmp(key, "known_hosts") == 0)
		cfg.known_hosts = strdup(value);
	else if (strcmp(key, "private_key") == 0)
		cfg.private_key = strdup(value);
	else if (strcmp(key, "public_key") == 0)
		cfg.public_key = strdup(value);
	else if (strcmp(key, "auth") == 0)
		cfg.auth = strdup(value);
	else
		LIBDISTEXEC_ABORT(-1, "Unknown config key: %s", key);

	return 0;

}

static int exec_init()
{
	LOG_DEBUG("Initializing SSH Frontend");
	int rc = libssh2_init(0);
	cfg.username = NULL;
	cfg.password = NULL;
	cfg.known_hosts = NULL;
	cfg.private_key = NULL;
	cfg.public_key = NULL;
	cfg.auth = "agent";

	return rc;

}

EXPORT int load()
{

	if (libdistexec_register_callback_execute
	    ("ssh", exec, exec_init, NULL, set_config_value, config_values,
	     ARRAY_SIZE(config_values))) {
		return -1;
	}


	return 0;
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION * session)
{
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;
	int dir;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);

	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);

	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

	return rc;
}

static int exec(libdistexec_node_t * node, const char *command)
{
#undef MACRO_LOGGER
#define MACRO_LOGGER logger
	libdistexec_logger_t logger;
	char logname[strlen(node->hostname) + strlen("ssh") + 1];
	sprintf(logname, "ssh@%s", node->hostname);
	libdistexec_logger_new(&logger, logname);

	LIBSSH2_SESSION *session;
	LIBSSH2_CHANNEL *channel;
	LIBSSH2_KNOWNHOSTS *nh;
	LIBSSH2_AGENT *agent = NULL;
	struct libssh2_agent_publickey *identity, *prev_identity = NULL;
	struct sockaddr_in sin;
	const char *fingerprint;
	const char *ipaddress;
	int rc;
	size_t len;
	int type;

	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

#ifdef _WIN32
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		LIBDISTEXEC_ABORT(-1, "WSAStartup failed");
	SOCKET sock;
#else
	int sock;

#endif

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	struct addrinfo result, *p_result = &result;
	int error;

	error = getaddrinfo(node->hostname, NULL, &hints, &p_result);
	if (error != 0) {
		LOG_ERROR("Could not resolve %s: (%d) %s", node->hostname,
			  error, gai_strerror(error));
		return -1;
	}

	ipaddress =
	    inet_ntoa(((struct sockaddr_in *)p_result->ai_addr)->sin_addr);

	freeaddrinfo(p_result);

	//sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (-1 == sock) {
		LOG_ERROR("Could not open socket: ");
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(22);
	sin.sin_addr.s_addr = inet_addr(ipaddress);

	LOG_DEBUG("Connecting to (%s) %s:%i", node->hostname, ipaddress, 22);
	rc = connect(sock, (struct sockaddr *)(&sin),
		     sizeof(struct sockaddr_in));

	if (rc != 0) {
		LOG_ERROR("failed to connect: %s",
			  libdistexec_error_str(libdistexec_errno()));
		// goto cleanup stuff socket close n shit
		return -1;
	}
#if defined(_WIN32)
	u_long flags = 1;	/* 0 = blocking, non-zero = non-blocking */
	rc = ioctlsocket(sock, FIONBIO, &flags);
	if (rc != NO_ERROR)
		LIBDISTEXEC_ABORT(-1, "ioctlsocket failed");
#else

	rc = fcntl(sock, F_SETFL, O_NONBLOCK);
	if (-1 == rc)
		LIBDISTEXEC_ABORT(-1, "fcntl failed");

	rc = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	if (-1 == rc)
		LIBDISTEXEC_ABORT(-1, "fcntl failed");
#endif
	LOG_DEBUG("Initializing ssh session");
	session = libssh2_session_init();
	if (!session)
		LIBDISTEXEC_ABORT(-1, "libssh2_session_init failed");

	libssh2_session_set_blocking(session, 0);

	LOG_DEBUG("SSH handshake");
	while ((rc =
		libssh2_session_handshake(session,
					  sock)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (0 != rc) {
		LOG_ERROR("ssh handshake failed: %d", rc);
		libssh2_session_free(session);
		return -1;
	}

	LOG_DEBUG("knownhost init");
	nh = libssh2_knownhost_init(session);

	if (!nh) {
		LOG_ERROR("!nh, eek!");
		return -2;	// eeek
	}

	if (NULL != cfg.known_hosts) {
		LOG_DEBUG("know hosts: %s", cfg.known_hosts);
		libssh2_knownhost_readfile(nh, cfg.known_hosts,
					   LIBSSH2_KNOWNHOST_FILE_OPENSSH);
		libssh2_knownhost_writefile(nh, cfg.known_hosts,
					    LIBSSH2_KNOWNHOST_FILE_OPENSSH);
	}

	fingerprint = libssh2_session_hostkey(session, &len, &type);

	if (fingerprint) {
		LOG_DEBUG("Fingerprint: %s", fingerprint);
		struct libssh2_knownhost *host;
		int check = libssh2_knownhost_checkp(nh, ipaddress, 22,
						     fingerprint, len,
						     LIBSSH2_KNOWNHOST_TYPE_PLAIN
						     |
						     LIBSSH2_KNOWNHOST_KEYENC_RAW,
						     &host);
		/*****
	         * At this point, we could verify that 'check' tells us the key is
        	 * fine or bail out.
	         *****/

		if (1 == check) {
			LOG_DEBUG("Found key: %s", host->key);
			// found in known_hosts
		} else if (2 == check) {
			// not found in known hosts
			;
		} else {
			// undefined?!
			;
		}
	} else {
		LOG_ERROR("no fingerprint, eek!");
		return -3;
	}

	libssh2_knownhost_free(nh);

	/*
	   use password auth if:
	   cfg.identity is NULL or strlen 0 and
	   cfg.password is not NULL and strlen > 0

	   use key auth if:
	   cfg.identity is not NULL and strlen > 0
	   cfg.password
	 */

	if (strcmp(cfg.auth, "agent") == 0) {

		LOG_DEBUG("init ssh agent");
		agent = libssh2_agent_init(session);

		if (!agent) {
			LIBDISTEXEC_ABORT(-1, "Could not init ssh agent");
		}

		LOG_DEBUG("Connect to ssh agent");
		if (libssh2_agent_connect(agent)) {
			LIBDISTEXEC_ABORT(-1, "Could not connect to ssh agent");
		}

		LOG_DEBUG("Requesting identities to ssh-agent");
		if (libssh2_agent_list_identities(agent)) {
			LIBDISTEXEC_ABORT(-1,
					  "Could not request identities to ssh-agent");
		}

		LOG_DEBUG("while(1).... get identities");
		while (1) {
			rc = libssh2_agent_get_identity(agent, &identity,
							prev_identity);

			if (1 == rc)
				break;

			if (rc < 0) {
				LIBDISTEXEC_ABORT(-1,
						  "Failure obtaining identity from ssh-agent support");
			}

			while ((rc =
				libssh2_agent_userauth(agent, cfg.username,
						       identity)) ==
			       LIBSSH2_ERROR_EAGAIN)
				waitsocket(sock, session);

			if (rc) {
				LOG_ERROR
				    ("Authentication with username %s and public key %s failed!",
				     cfg.username, identity->comment);
				libssh2_agent_free(agent);
				return -1;

			} else {
				LOG_INFO
				    ("Authentication with username %s and public key %s succeeded!",
				     cfg.username, identity->comment);
				break;
			}

			prev_identity = identity;
		}
	} else {
		LIBDISTEXEC_ABORT(-1, "Unsupported auth method: %s", cfg.auth);
	}

	LOG_DEBUG("opening channel");
	while ((channel = libssh2_channel_open_session(session)) == NULL &&
	       libssh2_session_last_error(session, NULL, NULL,
					  0) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (NULL == channel) {
		LOG_ERROR("channel is NULL for %s", node->hostname);
		return -1;
	}

	LOG_DEBUG("Executing command: %s", command);
	while ((rc =
		libssh2_channel_exec(channel, command)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (rc != 0) {
		LIBDISTEXEC_ABORT(-1, "error!");
	}

	int bytecount = 0;
	int exitcode = 127;
	char *exitsignal = (char *)"none";
	for (;;) {
		int rc;
		int rc_out;
		int rc_err;
		do {
			int stdout_l = 0;
			int stderr_l = 0;

			char stdout_buffer[0x4000] = { '\0' };
			char stderr_buffer[0x4000] = { '\0' };
			char stdout_buffer_s[1] = { '\0' };
			char stderr_buffer_s[1] = { '\0' };
			size_t stdout_buffer_i = 0;
			size_t stderr_buffer_i = 0;

			// read stdout until the first \n
 readstdout:
			while (1) {
				rc_out =
				    libssh2_channel_read(channel,
							 stdout_buffer_s,
							 sizeof
							 (stdout_buffer_s));

				if (rc_out > 0) {
					bytecount += rc_out;
					if (stdout_buffer_s[0] == '\n') {
						stdout_buffer[stdout_buffer_i] =
						    '\n';
						stdout_buffer[stdout_buffer_i +
							      1] = '\0';
						libdistexec_yield(node,
								  stdout_buffer,
								  0);
						stdout_buffer_i = 0;
						break;
					} else {
						if (stdout_buffer_i <
						    sizeof(stdout_buffer)) {
							stdout_buffer
							    [stdout_buffer_i++]
							    =
							    stdout_buffer_s[0];
							goto readstdout;
						} else {
							LIBDISTEXEC_ABORT(-1,
									  "stdout buffer overflow!");
						}
					}
				} else {
					if (stdout_buffer_i > 0) {
						stdout_buffer[stdout_buffer_i] =
						    '\0';
						libdistexec_yield(node,
								  stdout_buffer,
								  0);
						stdout_buffer_i = 0;
					}
					break;
				}
			}

 readstderr:
			while (1) {
				rc_err =
				    libssh2_channel_read_stderr(channel,
								stderr_buffer_s,
								sizeof
								(stderr_buffer_s));
				if (rc_err > 0) {
					bytecount += rc_err;
					if (stderr_buffer_s[0] == '\n') {
						stderr_buffer[stderr_buffer_i] =
						    '\n';
						stderr_buffer[stderr_buffer_i +
							      1] = '\0';
						libdistexec_yield(node,
								  stderr_buffer,
								  1);
						stderr_buffer_i = 0;
						break;
					} else {
						if (stderr_buffer_i <
						    sizeof(stderr_buffer)) {
							stderr_buffer
							    [stderr_buffer_i++]
							    =
							    stderr_buffer_s[0];
							goto readstderr;
						} else {
							LIBDISTEXEC_ABORT(-1,
									  "stderr buffer overflow!");
						}
					}
				} else {
					if (stderr_buffer_i > 0) {
						stderr_buffer[stderr_buffer_i] =
						    '\0';
						libdistexec_yield(node,
								  stderr_buffer,
								  1);
						stderr_buffer_i = 0;
					}
					break;
				}
			}
		} while (rc_out > 0 || rc_err > 0);

		if (rc_out == LIBSSH2_ERROR_EAGAIN
		    || rc_err == LIBSSH2_ERROR_EAGAIN)
			waitsocket(sock, session);
		else
			break;
	}

	LOG_DEBUG("closing channel");
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (rc == 0) {
		exitcode = libssh2_channel_get_exit_status(channel);
		libssh2_channel_get_exit_signal(channel, &exitsignal, NULL,
						NULL, NULL, NULL, NULL);
	}

	if (exitsignal)
		LOG_DEBUG("Got exitsignal: %s", exitsignal);
	else
		LOG_DEBUG("EXIT: %d bytecount: %d", exitcode, bytecount);

 shutdown:
	if (agent) {
		libssh2_agent_free(agent);
		agent = NULL;
	}

	if (channel) {
		libssh2_channel_free(channel);
		channel = NULL;
	}

	if (session) {
		libssh2_session_disconnect(session,
					   "Normal shutdown, thank you for playing");
		libssh2_session_free(session);
		session = NULL;
	}
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif

	return 0;
/*
	LOG_DEBUG("Logging in with %s...", username);
	rc = libssh2_userauth_password(session, username, password);
	if (0 != rc)
		LIBDISTEXEC_ABORT(-1, "libssh2_userauth_password failed");
*/
}

EXPORT int unload()
{
	LOG_DEBUG("unload");
	libssh2_exit();
	return 0;
}

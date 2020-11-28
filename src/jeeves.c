#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/cerver.h>

#include <cerver/http/http.h>
#include <cerver/http/route.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"
#include "handler.h"
#include "version.h"

unsigned int PORT = CERVER_DEFAULT_PORT;

unsigned int CERVER_RECEIVE_BUFFER_SIZE = CERVER_DEFAULT_RECEIVE_BUFFER_SIZE;
unsigned int CERVER_TH_THREADS = CERVER_DEFAULT_POOL_THREADS;
unsigned int CERVER_CONNECTION_QUEUE = CERVER_DEFAULT_CONNECTION_QUEUE;

static HttpResponse *oki_doki = NULL;
static HttpResponse *bad_request = NULL;
static HttpResponse *server_error = NULL;
static HttpResponse *bad_user = NULL;

static HttpResponse *jeeves_works = NULL;
static HttpResponse *current_version = NULL;

#pragma region main

static unsigned int jeeves_env_get_port (void) {
	
	unsigned int retval = 1;

	char *port_env = getenv ("PORT");
	if (port_env) {
		PORT = (unsigned int) atoi (port_env);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get PORT from env");
	}

	return retval;

}

static void jeeves_env_get_receive_buffer_size (void) {

	char *buffer_size = getenv ("CERVER_RECEIVE_BUFFER_SIZE");
	if (buffer_size) {
		CERVER_RECEIVE_BUFFER_SIZE = (unsigned int) atoi (buffer_size);
		cerver_log_success (
			"CERVER_RECEIVE_BUFFER_SIZE -> %d\n", CERVER_RECEIVE_BUFFER_SIZE
		);
	}

	else {
		cerver_log_warning (
			"Failed to get CERVER_RECEIVE_BUFFER_SIZE from env - using default %d!",
			CERVER_RECEIVE_BUFFER_SIZE
		);
	}

}

static void jeeves_env_get_th_threads (void) {

	char *th_threads = getenv ("CERVER_TH_THREADS");
	if (th_threads) {
		CERVER_TH_THREADS = (unsigned int) atoi (th_threads);
		cerver_log_success ("CERVER_TH_THREADS -> %d\n", CERVER_TH_THREADS);
	}

	else {
		cerver_log_warning (
			"Failed to get CERVER_TH_THREADS from env - using default %d!",
			CERVER_TH_THREADS
		);
	}

}

static void jeeves_env_get_connection_queue (void) {

	char *connection_queue = getenv ("CERVER_CONNECTION_QUEUE");
	if (connection_queue) {
		CERVER_CONNECTION_QUEUE = (unsigned int) atoi (connection_queue);
		cerver_log_success ("CERVER_CONNECTION_QUEUE -> %d\n", CERVER_CONNECTION_QUEUE);
	}

	else {
		cerver_log_warning (
			"Failed to get CERVER_CONNECTION_QUEUE from env - using default %d!",
			CERVER_CONNECTION_QUEUE
		);
	}

}

static unsigned int jeeves_init_env (void) {

	unsigned int errors = 0;

	errors |= jeeves_env_get_port ();

	jeeves_env_get_receive_buffer_size ();

	jeeves_env_get_th_threads ();

	jeeves_env_get_connection_queue ();

	return errors;

}

static unsigned int jeeves_init_responses (void) {

	unsigned int retval = 1;

	oki_doki = http_response_json_key_value (
		(http_status) 200, "oki", "doki"
	);

	bad_request = http_response_json_key_value (
		(http_status) 400, "error", "Bad request!"
	);

	server_error = http_response_json_key_value (
		(http_status) 500, "error", "Internal server error!"
	);

	bad_user = http_response_json_key_value (
		(http_status) 400, "error", "Bad user!"
	);

	jeeves_works = http_response_json_key_value (
		(http_status) 200, "msg", "Barcel works!"
	);

	char *status = c_string_create (
		"%s - %s", JEEVES_VERSION_NAME, JEEVES_VERSION_DATE
	);

	if (status) {
		current_version = http_response_json_key_value (
			(http_status) 200, "version", status
		);

		free (status);
	}

	if (
		oki_doki && bad_request && server_error && bad_user
		&& jeeves_works && current_version
	) retval = 0;

	return retval;

}

// inits jeeves main values
unsigned int jeeves_init (void) {

	unsigned int retval = 1;

	if (!jeeves_init_env ()) {
		unsigned int errors = 0;

		errors |= jeeves_handler_init ();

		errors |= jeeves_init_responses ();

		retval = errors;
	}

	return retval;

}

// ends jeeves main values
unsigned int jeeves_end (void) {

	unsigned int errors = 0;

	http_respponse_delete (oki_doki);
	http_respponse_delete (bad_request);
	http_respponse_delete (server_error);
	http_respponse_delete (bad_user);

	http_respponse_delete (jeeves_works);
	http_respponse_delete (current_version);

	jeeves_handler_end ();

	return errors;

}

#pragma endregion

#pragma region routes

// GET /jeeves
void jeeves_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (jeeves_works, http_receive);

}

// GET /jeeves/version
void jeeves_version_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (current_version, http_receive);

}

// GET /jeeves/auth
void jeeves_auth_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	// TODO:
	(void) http_response_send (oki_doki, http_receive);

}

#pragma endregion
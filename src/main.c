#include <stdlib.h>
#include <stdio.h>

#include <signal.h>
#include <time.h>

#include <cerver/version.h>
#include <cerver/cerver.h>

#include <cerver/http/http.h>
#include <cerver/http/route.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"
#include "handler.h"
#include "version.h"

#include "controllers/users.h"

#include "routes/users.h"

Cerver *jeeves_cerver = NULL;

static void end (int dummy) {
	
	if (jeeves_cerver) {
		cerver_stats_print (jeeves_cerver, false, false);
		cerver_log_msg ("\nHTTP Cerver stats:\n");
		http_cerver_all_stats_print ((HttpCerver *) jeeves_cerver->cerver_data);
		cerver_log_line_break ();
		cerver_teardown (jeeves_cerver);
	}

	(void) jeeves_end ();

	cerver_end ();

	exit (0);

}

static void jeeves_set_routes (HttpCerver *http_cerver) {

	/* register top level route */
	// GET /jeeves
	HttpRoute *jeeves_route = http_route_create (REQUEST_METHOD_GET, "jeeves", jeeves_handler);
	http_cerver_route_register (http_cerver, jeeves_route);

	/* register jeeves children routes */
	// GET /jeeves/version
	HttpRoute *jeeves_version_route = http_route_create (REQUEST_METHOD_GET, "version", jeeves_version_handler);
	http_route_child_add (jeeves_route, jeeves_version_route);

	// GET /jeeves/auth
	HttpRoute *jeeves_auth_route = http_route_create (REQUEST_METHOD_GET, "auth", jeeves_auth_handler);
	http_route_set_auth (jeeves_auth_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (jeeves_auth_route, jeeves_user_parse_from_json, jeeves_user_delete);
	http_route_child_add (jeeves_route, jeeves_auth_route);

}

static void jeeves_set_users_routes (HttpCerver *http_cerver) {

	/* register top level route */
	// GET /api/users
	HttpRoute *users_route = http_route_create (REQUEST_METHOD_GET, "api/users", users_handler);
	http_cerver_route_register (http_cerver, users_route);

	/* register users children routes */
	// POST api/users/login
	HttpRoute *users_login_route = http_route_create (REQUEST_METHOD_POST, "login", users_login_handler);
	http_route_child_add (users_route, users_login_route);

	// POST api/users/register
	HttpRoute *users_register_route = http_route_create (REQUEST_METHOD_POST, "register", users_register_handler);
	http_route_child_add (users_route, users_register_route);

}

static void start (void) {

	jeeves_cerver = cerver_create (
		CERVER_TYPE_WEB,
		"jeeves-service",
		PORT,
		PROTOCOL_TCP,
		false,
		CERVER_CONNECTION_QUEUE
	);

	if (jeeves_cerver) {
		/*** cerver configuration ***/
		cerver_set_receive_buffer_size (jeeves_cerver, CERVER_RECEIVE_BUFFER_SIZE);
		cerver_set_thpool_n_threads (jeeves_cerver, CERVER_TH_THREADS);
		cerver_set_handler_type (jeeves_cerver, CERVER_HANDLER_TYPE_THREADS);

		/*** web cerver configuration ***/
		HttpCerver *http_cerver = (HttpCerver *) jeeves_cerver->cerver_data;

		http_cerver_set_uploads_path (http_cerver, JEEVES_UPLOADS_TEMP_DIR);

		http_cerver_auth_set_jwt_algorithm (http_cerver, JWT_ALG_RS256);
		http_cerver_auth_set_jwt_priv_key_filename (http_cerver, "keys/key.key");
		http_cerver_auth_set_jwt_pub_key_filename (http_cerver, "keys/key.pub");

		jeeves_set_routes (http_cerver);

		if (ENABLE_USERS_ROUTES) {
			jeeves_set_users_routes (http_cerver);
		}

		// add a catch all route
		http_cerver_set_catch_all_route (http_cerver, jeeves_catch_all_handler);

		if (cerver_start (jeeves_cerver)) {
			cerver_log_error (
				"Failed to start %s!",
				jeeves_cerver->info->name->str
			);

			cerver_delete (jeeves_cerver);
		}
	}

	else {
		cerver_log_error ("Failed to create cerver!");

		cerver_delete (jeeves_cerver);
	}

}

int main (int argc, const char **argv) {

	srand ((unsigned int) time (NULL));

	(void) signal (SIGINT, end);
	(void) signal (SIGTERM, end);
	(void) signal (SIGKILL, end);

	(void) signal (SIGPIPE, SIG_IGN);

	cerver_init ();

	cerver_version_print_full ();

	jeeves_version_print_full ();

	if (!jeeves_init ()) {
		start ();
	}

	else {
		cerver_log_error ("Failed to init jeeves!");
	}

	(void) jeeves_end ();

	cerver_end ();

	return 0;

}
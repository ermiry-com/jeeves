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

#include "handler.h"
#include "jeeves.h"
#include "mongo.h"
#include "version.h"
#include "worker.h"

#include "models/action.h"
#include "models/job.h"
#include "models/role.h"
#include "models/user.h"

#include "controllers/roles.h"
#include "controllers/users.h"

unsigned int PORT = CERVER_DEFAULT_PORT;

unsigned int CERVER_RECEIVE_BUFFER_SIZE = CERVER_DEFAULT_RECEIVE_BUFFER_SIZE;
unsigned int CERVER_TH_THREADS = CERVER_DEFAULT_POOL_THREADS;
unsigned int CERVER_CONNECTION_QUEUE = CERVER_DEFAULT_CONNECTION_QUEUE;

static const String *MONGO_URI = NULL;
static const String *MONGO_APP_NAME = NULL;
static const String *MONGO_DB = NULL;

bool ENABLE_USERS_ROUTES = false;

HttpResponse *oki_doki = NULL;
HttpResponse *bad_request = NULL;
HttpResponse *server_error = NULL;
HttpResponse *bad_user = NULL;
HttpResponse *missing_values = NULL;

HttpResponse *jeeves_works = NULL;
HttpResponse *current_version = NULL;

HttpResponse *job_created_bad = NULL;
HttpResponse *job_deleted_bad = NULL;

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

static unsigned int jeeves_env_get_mongo_app_name (void) {

	unsigned int retval = 1;

	char *mongo_app_name_env = getenv ("MONGO_APP_NAME");
	if (mongo_app_name_env) {
		MONGO_APP_NAME = str_new (mongo_app_name_env);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get MONGO_APP_NAME from env!");
	}

	return retval;

}

static unsigned int jeeves_env_get_mongo_db (void) {

	unsigned int retval = 1;

	char *mongo_db_env = getenv ("MONGO_DB");
	if (mongo_db_env) {
		MONGO_DB = str_new (mongo_db_env);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get MONGO_DB from env!");
	}

	return retval;

}

static unsigned int jeeves_env_get_mongo_uri (void) {

	unsigned int retval = 1;

	char *mongo_uri_env = getenv ("MONGO_URI");
	if (mongo_uri_env) {
		MONGO_URI = str_new (mongo_uri_env);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get MONGO_URI from env!");
	}

	return retval;

}

static void jeeves_env_get_enable_users_routes (void) {

	char *enable_users = getenv ("ENABLE_USERS_ROUTES");
	if (enable_users) {
		if (!strcmp (enable_users, "TRUE")) {
			ENABLE_USERS_ROUTES = true;
			cerver_log_success ("ENABLE_USERS_ROUTES -> TRUE\n");
		}

		else {
			ENABLE_USERS_ROUTES = false;
			cerver_log_success ("ENABLE_USERS_ROUTES -> FALSE\n");
		}
	}

	else {
		cerver_log_warning (
			"Failed to get ENABLE_USERS_ROUTES from env - using default FALSE!"
		);
	}

}

static unsigned int jeeves_init_env (void) {

	unsigned int errors = 0;

	errors |= jeeves_env_get_port ();

	jeeves_env_get_receive_buffer_size ();

	jeeves_env_get_th_threads ();

	jeeves_env_get_connection_queue ();

	errors |= jeeves_env_get_mongo_uri ();

	errors |= jeeves_env_get_mongo_app_name ();

	errors |= jeeves_env_get_mongo_db ();

	jeeves_env_get_enable_users_routes ();

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

	missing_values = http_response_json_key_value (
		(http_status) 400, "error", "Missing values!"
	);

	jeeves_works = http_response_json_key_value (
		(http_status) 200, "msg", "Jeeves works!"
	);

	char *status = c_string_create (
		"%s - %s", JEEVES_VERSION_NAME, JEEVES_VERSION_DATE
	);

	/*** jobs ***/

	job_created_bad = http_response_json_key_value (
		(http_status) 400, "error", "Failed to create job!"
	);

	job_deleted_bad = http_response_json_key_value (
		(http_status) 400, "error", "Failed to delete job!"
	);

	if (status) {
		current_version = http_response_json_key_value (
			(http_status) 200, "version", status
		);

		free (status);
	}

	if (
		oki_doki && bad_request && server_error && bad_user && missing_values
		&& jeeves_works && current_version
		&& job_created_bad && job_deleted_bad
	) retval = 0;

	return retval;

}

static unsigned int jeeves_mongo_connect (void) {

	unsigned int errors = 0;

	bool connected_to_mongo = false;

	mongo_set_uri (MONGO_URI->str);
	mongo_set_app_name (MONGO_APP_NAME->str);
	mongo_set_db_name (MONGO_DB->str);

	if (!mongo_connect ()) {
		// test mongo connection
		if (!mongo_ping_db ()) {
			cerver_log_success ("Connected to Mongo DB!");

			// open handle to actions collection
			errors |= actions_collection_get ();

			// open handle to jobs collection
			errors |= jobs_collection_get ();

			// open handle to roles collection
			errors |= roles_collection_get ();

			// open handle to users collection
			errors |= users_collection_get ();

			connected_to_mongo = true;
		}
	}

	if (!connected_to_mongo) {
		cerver_log_error ("Failed to connect to mongo!");
		errors |= 1;
	}

	return errors;

}

static unsigned int jeeves_mongo_init (void) {

	unsigned int retval = 1;

	if (!jeeves_mongo_connect ()) {
		if (!jeeves_roles_init ()) {
			retval = 0;
		}

		else {
			cerver_log_error ("Failed to get roles from db!");
		}
	}

	return retval;

}

// inits jeeves main values
unsigned int jeeves_init (void) {

	unsigned int retval = 1;

	if (!jeeves_init_env ()) {
		unsigned int errors = 0;

		errors |= jeeves_mongo_init ();

		errors |= jeeves_handler_init ();

		errors |= jeeves_users_init ();

		errors |= jeeves_init_responses ();

		errors |= jeeves_uploads_worker_init ();

		retval = errors;
	}

	return retval;

}

static unsigned int jeeves_mongo_end (void) {

	if (mongo_get_status () == MONGO_STATUS_CONNECTED) {
		actions_collection_close ();

		jobs_collection_close ();

		roles_collection_close ();

		users_collection_close ();

		(void) jeeves_uploads_worker_end ();

		mongo_disconnect ();
	}

	return 0;

}

// ends jeeves main values
unsigned int jeeves_end (void) {

	unsigned int errors = 0;

	http_respponse_delete (oki_doki);
	http_respponse_delete (bad_request);
	http_respponse_delete (server_error);
	http_respponse_delete (bad_user);
	http_respponse_delete (missing_values);

	http_respponse_delete (jeeves_works);
	http_respponse_delete (current_version);

	http_respponse_delete (job_created_bad);
	http_respponse_delete (job_deleted_bad);

	errors |= jeeves_mongo_end ();

	jeeves_roles_end ();

	jeeves_users_end ();

	jeeves_handler_end ();

	str_delete ((String *) MONGO_URI);
	str_delete ((String *) MONGO_APP_NAME);
	str_delete ((String *) MONGO_DB);

	return errors;

}
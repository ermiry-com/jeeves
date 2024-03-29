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

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include <cmongo/mongo.h>

#include "jeeves.h"
#include "runtime.h"
#include "worker.h"

#include "models/action.h"
#include "models/job.h"
#include "models/role.h"
#include "models/user.h"

#include "controllers/jobs.h"
#include "controllers/roles.h"
#include "controllers/service.h"
#include "controllers/users.h"

RuntimeType RUNTIME = RUNTIME_TYPE_NONE;

unsigned int PORT = CERVER_DEFAULT_PORT;

unsigned int CERVER_RECEIVE_BUFFER_SIZE = CERVER_DEFAULT_RECEIVE_BUFFER_SIZE;
unsigned int CERVER_TH_THREADS = CERVER_DEFAULT_POOL_THREADS;
unsigned int CERVER_CONNECTION_QUEUE = CERVER_DEFAULT_CONNECTION_QUEUE;

static char MONGO_URI[MONGO_URI_SIZE] = { 0 };
static char MONGO_APP_NAME[MONGO_APP_NAME_SIZE] = { 0 };
static char MONGO_DB[MONGO_DB_SIZE] = { 0 };

static char REAL_PRIV_KEY[PRIV_KEY_SIZE] = { 0 };
const char *PRIV_KEY = REAL_PRIV_KEY;

static char REAL_PUB_KEY[PUB_KEY_SIZE] = { 0 };
const char *PUB_KEY = REAL_PUB_KEY;

bool ENABLE_USERS_ROUTES = false;

static void jeeves_env_get_runtime (void) {
	
	char *runtime_env = getenv ("RUNTIME");
	if (runtime_env) {
		RUNTIME = runtime_from_string (runtime_env);
		cerver_log_success (
			"RUNTIME -> %s", runtime_to_string (RUNTIME)
		);
	}

	else {
		cerver_log_warning ("Failed to get RUNTIME from env!");
	}

}

static unsigned int jeeves_env_get_port (void) {
	
	unsigned int retval = 1;

	char *port_env = getenv ("PORT");
	if (port_env) {
		PORT = (unsigned int) atoi (port_env);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get PORT from env!");
	}

	return retval;

}

static void jeeves_env_get_cerver_receive_buffer_size (void) {

	char *buffer_size = getenv ("CERVER_RECEIVE_BUFFER_SIZE");
	if (buffer_size) {
		CERVER_RECEIVE_BUFFER_SIZE = (unsigned int) atoi (buffer_size);
		cerver_log_success (
			"CERVER_RECEIVE_BUFFER_SIZE -> %d", CERVER_RECEIVE_BUFFER_SIZE
		);
	}

	else {
		cerver_log_warning (
			"Failed to get CERVER_RECEIVE_BUFFER_SIZE from env - using default %d!",
			CERVER_RECEIVE_BUFFER_SIZE
		);
	}
}

static void jeeves_env_get_cerver_th_threads (void) {

	char *th_threads = getenv ("CERVER_TH_THREADS");
	if (th_threads) {
		CERVER_TH_THREADS = (unsigned int) atoi (th_threads);
		cerver_log_success ("CERVER_TH_THREADS -> %d", CERVER_TH_THREADS);
	}

	else {
		cerver_log_warning (
			"Failed to get CERVER_TH_THREADS from env - using default %d!",
			CERVER_TH_THREADS
		);
	}

}

static void jeeves_env_get_cerver_connection_queue (void) {

	char *connection_queue = getenv ("CERVER_CONNECTION_QUEUE");
	if (connection_queue) {
		CERVER_CONNECTION_QUEUE = (unsigned int) atoi (connection_queue);
		cerver_log_success ("CERVER_CONNECTION_QUEUE -> %d", CERVER_CONNECTION_QUEUE);
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
		(void) strncpy (
			MONGO_APP_NAME,
			mongo_app_name_env,
			MONGO_APP_NAME_SIZE - 1
		);

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
		(void) strncpy (
			MONGO_DB,
			mongo_db_env,
			MONGO_DB_SIZE - 1
		);

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
		(void) strncpy (
			MONGO_URI,
			mongo_uri_env,
			MONGO_URI_SIZE - 1
		);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get MONGO_URI from env!");
	}

	return retval;

}

static unsigned int jeeves_env_get_private_key (void) {

	unsigned int retval = 1;

	char *priv_key_env = getenv ("PRIV_KEY");
	if (priv_key_env) {
		(void) strncpy (
			REAL_PRIV_KEY,
			priv_key_env,
			PRIV_KEY_SIZE - 1
		);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get PRIV_KEY from env!");
	}

	return retval;

}

static unsigned int jeeves_env_get_public_key (void) {

	unsigned int retval = 1;

	char *pub_key_env = getenv ("PUB_KEY");
	if (pub_key_env) {
		(void) strncpy (
			REAL_PUB_KEY,
			pub_key_env,
			PUB_KEY_SIZE - 1
		);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get PUB_KEY from env!");
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

	jeeves_env_get_runtime ();

	errors |= jeeves_env_get_port ();

	jeeves_env_get_cerver_receive_buffer_size ();

	jeeves_env_get_cerver_th_threads ();

	jeeves_env_get_cerver_connection_queue ();

	errors |= jeeves_env_get_mongo_app_name ();

	errors |= jeeves_env_get_mongo_db ();

	errors |= jeeves_env_get_mongo_uri ();

	errors |= jeeves_env_get_private_key ();

	errors |= jeeves_env_get_public_key ();

	jeeves_env_get_enable_users_routes ();

	return errors;

}

static unsigned int jeeves_mongo_connect (void) {

	unsigned int errors = 0;

	bool connected_to_mongo = false;

	mongo_set_uri (MONGO_URI);
	mongo_set_app_name (MONGO_APP_NAME);
	mongo_set_db_name (MONGO_DB);

	if (!mongo_connect ()) {
		// test mongo connection
		if (!mongo_ping_db ()) {
			cerver_log_success ("Connected to Mongo DB!");

			errors |= actions_model_init ();

			errors |= jobs_model_init ();

			errors |= roles_model_init ();

			errors |= users_model_init ();

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

		errors |= jeeves_service_init ();

		errors |= jeeves_users_init ();

		errors |= jeeves_jobs_init ();

		errors |= jeeves_worker_init ();

		retval = errors;
	}

	return retval;

}

static unsigned int jeeves_mongo_end (void) {

	if (mongo_get_status () == MONGO_STATUS_CONNECTED) {
		actions_model_end ();

		jobs_model_end ();

		roles_model_end ();

		users_model_end ();

		mongo_disconnect ();
	}

	return 0;

}

// ends jeeves main values
unsigned int jeeves_end (void) {

	unsigned int errors = 0;

	(void) jeeves_worker_end ();

	errors |= jeeves_mongo_end ();

	jeeves_roles_end ();

	jeeves_users_end ();

	jeeves_service_end ();

	jeeves_jobs_end ();

	return errors;

}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>
#include <cerver/http/json/json.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "errors.h"
#include "jeeves.h"
#include "mongo.h"

#include "models/job.h"
#include "models/user.h"

#include "controllers/jobs.h"

static char *jeeves_get_jobs_handler_generate_json (
	mongoc_cursor_t *jobs_cursor,
	size_t *json_len
) {

	char *retval = NULL;

	bson_t *doc = bson_new ();
	if (doc) {
		bson_t jobs_array = { 0 };
		(void) bson_append_array_begin (doc, "jobs", -1, &jobs_array);
		char buf[16] = { 0 };
		const char *key = NULL;
		size_t keylen = 0;

		int i = 0;
		const bson_t *job_doc = NULL;
		while (mongoc_cursor_next (jobs_cursor, &job_doc)) {
			keylen = bson_uint32_to_string (i, &key, buf, sizeof (buf));
			(void) bson_append_document (&jobs_array, key, (int) keylen, job_doc);

			bson_destroy ((bson_t *) job_doc);

			i++;
		}
		(void) bson_append_array_end (doc, &jobs_array);

		retval = bson_as_relaxed_extended_json (doc, json_len);
	}

	return retval;

}

// GET /api/jeeves/jobs
// Returns all of the authenticated user's jobs
void jeeves_get_jobs_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	User *user = (User *) request->decoded_data;
	if (user) {
		// get user's jobs from the db
		mongoc_cursor_t *jobs_cursor = jeeves_jobs_get_all_by_user (
			&user->oid, NULL
		);

		if (jobs_cursor) {
			// convert them to json and send them back
			size_t json_len = 0;
			char *json = jeeves_get_jobs_handler_generate_json (
				jobs_cursor, &json_len
			);

			if (json) {
				(void) http_response_json_custom_reference_send (
					http_receive,
					200,
					json, json_len
				);

				free (json);
			}

			else {
				(void) http_response_send (server_error, http_receive);
			}

			mongoc_cursor_destroy (jobs_cursor);
		}

		else {
			(void) http_response_send (server_error, http_receive);
		}
	}

	else {
		(void) http_response_send (bad_user, http_receive);
	}

}

static void jeeves_job_parse_json (
	json_t *json_body,
	const char **name,
	const char **description
) {

	// get values from json to create a new transaction
	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (json_body) == JSON_OBJECT) {
		json_object_foreach (json_body, key, value) {
			if (!strcmp (key, "name")) {
				*name = json_string_value (value);
				(void) printf ("name: \"%s\"\n", *name);
			}

			else if (!strcmp (key, "description")) {
				*description = json_string_value (value);
				(void) printf ("description: \"%s\"\n", *description);
			}
		}
	}

}

static JeevesError jeeves_create_job_handler_internal (
	JeevesJob **job,
	const char *user_id, const String *request_body
) {

	JeevesError error = JEEVES_ERROR_NONE;

	if (request_body) {
		const char *name = NULL;
		const char *description = NULL;

		json_error_t json_error =  { 0 };
		json_t *json_body = json_loads (request_body->str, 0, &json_error);
		if (json_body) {
			jeeves_job_parse_json (
				json_body,
				&name, &description
			);

			if (name) {
				// TODO:
			}

			else {
				error = JEEVES_ERROR_MISSING_VALUES;
			}

			json_decref (json_body);
		}

		else {
			cerver_log_error (
				"json_loads () - json error on line %d: %s\n", 
				json_error.line, json_error.text
			);

			error = JEEVES_ERROR_BAD_REQUEST;
		}
	}

	return error;

}

// POST /api/jeeves/jobs
// A user has requested to create a new job
void jeeves_create_job_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	User *user = (User *) request->decoded_data;
	if (user) {
		JeevesJob *job = NULL;

		JeevesError error = jeeves_create_job_handler_internal (
			&job,
			user->id, request->body
		);

		if (error == JEEVES_ERROR_NONE) {
			#ifdef JEEVES_DEBUG
			jeeves_job_print (job);
			#endif

			// insert into the db
			if (!mongo_insert_one (
				jobs_collection,
				jeeves_job_to_bson (job)
			)) {
				// return success to user
				(void) http_response_send (
					oki_doki,
					http_receive
				);
			}

			else {
				(void) http_response_send (
					job_created_bad,
					http_receive
				);
			}

			// TODO: delete
		}

		else {
			jeeves_error_send_response (error, http_receive);
		}
	}

	else {
		(void) http_response_send (bad_user, http_receive);
	}

}

// GET /api/jeeves/jobs/test
void jeeves_jobs_test_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (oki_doki, http_receive);

}

// GET /api/jeeves/jobs/:id/info
void jeeves_job_info_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	const String *job_id = request->params[0];

	User *user = (User *) request->decoded_data;
	if (user) {
		// TODO:
	}

	else {
		(void) http_response_send (bad_user, http_receive);
	}

}

// GET /api/jeeves/jobs/:id/start
void jeeves_job_start_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	const String *job_id = request->params[0];

	User *user = (User *) request->decoded_data;
	if (user) {
		bson_oid_init_from_string (&user->oid, user->id);

		// check that the job belongs to the user
		JeevesJob *job = jeeves_job_get_by_id_and_user (
			job_id, &user->oid
		);

		if (job) {
			// TODO: start job

			// update the job in the db
			if (!mongo_update_one (
				jobs_collection,
				jeeves_job_query_oid (&job->oid),
				jeeves_job_start_update_bson ()
			)) {
				(void) http_response_send (oki_doki, http_receive);
			}

			jeeves_job_return (job);
		}

		else {
			(void) http_response_send (bad_request, http_receive);
		}
	}

	else {
		(void) http_response_send (bad_user, http_receive);
	}

}

// GET /api/jeeves/jobs/:id/stop
void jeeves_job_stop_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	const String *job_id = request->params[0];

	User *user = (User *) request->decoded_data;
	if (user) {
		bson_oid_init_from_string (&user->oid, user->id);

		// check that the job belongs to the user
		JeevesJob *job = jeeves_job_get_by_id_and_user (
			job_id, &user->oid
		);

		if (job) {
			// TODO: stop job

			// update the job in the db
			if (!mongo_update_one (
				jobs_collection,
				jeeves_job_query_oid (&job->oid),
				jeeves_job_stop_update_bson ()
			)) {
				(void) http_response_send (oki_doki, http_receive);
			}

			jeeves_job_return (job);
		}

		else {
			(void) http_response_send (bad_request, http_receive);
		}
	}

	else {
		(void) http_response_send (bad_user, http_receive);
	}

}
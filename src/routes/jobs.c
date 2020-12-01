#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"

#include "models/job.h"
#include "models/user.h"

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

}

// POST /api/jeeves/jobs
void jeeves_create_job_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (oki_doki, http_receive);

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

	(void) http_response_send (oki_doki, http_receive);

}
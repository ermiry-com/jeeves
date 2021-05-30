#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>
#include <cerver/http/json/json.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include "errors.h"
#include "jeeves.h"

#include "models/job.h"
#include "models/user.h"

#include "controllers/jobs.h"

// GET /api/jeeves/jobs
// Returns all of the authenticated user's jobs
void jeeves_get_jobs_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	User *user = (User *) request->decoded_data;
	if (user) {
		size_t json_len = 0;
		char *json = NULL;

		if (!jeeves_jobs_get_all_by_user_to_json (
			&user->oid,
			&json, &json_len
		)) {
			if (json) {
				(void) http_response_json_custom_reference_send (
					http_receive,
					HTTP_STATUS_OK,
					json, json_len
				);

				free (json);
			}

			else {
				(void) http_response_send (no_user_jobs, http_receive);
			}
		}

		else {
			(void) http_response_send (no_user_jobs, http_receive);
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
	}

}

// POST /api/jeeves/jobs
// A user has requested to create a new job
void jeeves_create_job_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	User *user = (User *) request->decoded_data;
	if (user) {
		JeevesError error = jeeves_job_create (
			user, request->body
		);

		switch (error) {
			case JEEVES_ERROR_NONE: {
				// return success to user
				(void) http_response_send (
					oki_doki,
					http_receive
				);
			} break;

			default: {
				jeeves_error_send_response (error, http_receive);
			} break;
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
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
		if (job_id) {
			size_t json_len = 0;
			char *json = NULL;

			if (!jeeves_job_get_by_id_and_user_to_json (
				job_id->str, &user->oid,
				NULL,
				&json, &json_len
			)) {
				if (json) {
					(void) http_response_json_custom_reference_send (
						http_receive, HTTP_STATUS_OK, json, json_len
					);

					free (json);
				}

				else {
					(void) http_response_send (server_error, http_receive);
				}
			}

			else {
				(void) http_response_send (no_user_job, http_receive);
			}
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
	}

}

// POST /api/jeeves/jobs/:id/config
void jeeves_job_config_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	const String *job_id = request->params[0];

	User *user = (User *) request->decoded_data;
	if (user) {
		JeevesError error = jeeves_job_config (
			user, job_id,
			request->body
		);

		switch (error) {
			case JEEVES_ERROR_NONE: {
				http_request_multi_part_keep_files (request);
				(void) http_response_send (oki_doki, http_receive);
			} break;

			default:
				jeeves_error_send_response (error, http_receive);
				break;
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
	}

}

static void jeeves_job_upload_handler_internal (
	const HttpReceive *http_receive,
	const HttpRequest *request,
	const char *user_id, JeevesJob *job
) {

	// get images that will be added to the job
	DoubleList *filenames = http_request_multi_parts_get_all_saved_filenames (request);
	if (filenames) {
		// create jobs images
		const char *filename = NULL;
		JobImage *job_image = NULL;
		int image_id = !job->n_images ? 0 : job->n_images - 1;
		DoubleList *images = dlist_init (job_image_delete, NULL);
		char *end = NULL;
		for (ListElement *le = dlist_start (filenames); le; le = le->next) {
			filename = (const char *) le->data;

			job_image = job_image_create (
				image_id,
				filename,
				NULL, "null"
			);

			end = strstr (filename, JEEVES_UPLOADS_TEMP_DIR);
			if (end) {
				(void) snprintf (
					job_image->original, JOB_IMAGE_ORIGINAL_SIZE,
					"%s/%s%s",
					JEEVES_UPLOADS_PATH,
					user_id,
					end + strlen (JEEVES_UPLOADS_TEMP_DIR)
				);
			}

			(void) dlist_insert_at_end_unsafe (
				images,
				job_image
			);

			image_id += 1;
		}

		// update current job with new images
		if (!mongo_update_one (
			jobs_collection,
			jeeves_job_query_oid (&job->oid),
			jeeves_job_images_add_bson (images)
		)) {
			// check if the job is ready to be started
			if (job->type != JOB_TYPE_NONE) {
				(void) mongo_update_one (
					jobs_collection,
					jeeves_job_query_oid (&job->oid),
					jeeves_job_status_update_bson (JOB_STATUS_READY)
				);
			}

			// request UPLOADS worker to save frames to persistent storage
			(void) jeeves_uploads_worker_push (
				jeeves_upload_new (request->dirname->str, user_id)
			);

			(void) http_response_send (oki_doki, http_receive);
		}

		else {
			http_request_multi_part_discard_files (request);
			(void) http_response_send (server_error, http_receive);
		}

		dlist_delete (images);
	}

	else {
		cerver_log_warning ("No images in request!");
		(void) http_response_send (bad_request_error, http_receive);
	}

}

// POST /api/jeeves/jobs/:id/upload
void jeeves_job_upload_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	const String *job_id = request->params[0];

	User *user = (User *) request->decoded_data;
	if (user) {
		bson_oid_init_from_string (&user->oid, user->id);

		JeevesJob *job = jeeves_job_get_by_id_and_user (
			job_id, &user->oid
		);

		if (job) {
			jeeves_job_upload_handler_internal (
				http_receive, request,
				user->id, job
			);

			jeeves_job_return (job);
		}

		else {
			cerver_log_warning (
				"upload_handler () - Job %s does NOT belong to user %s",
				job_id->str, user->id
			);

			http_request_multi_part_discard_files (request);
			(void) http_response_send (bad_request_error, http_receive);
		}
	}

	else {
		http_request_multi_part_discard_files (request);
		(void) http_response_send (bad_user_error, http_receive);
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
		JeevesError error = jeeves_job_start (user, job_id);

		switch (error) {
			case JEEVES_ERROR_NONE: {
				(void) http_response_send (oki_doki, http_receive);
			} break;

			default:
				jeeves_error_send_response (error, http_receive);
				break;
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
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
		JeevesError error = jeeves_job_stop (user, job_id);

		switch (error) {
			case JEEVES_ERROR_NONE: {
				(void) http_response_send (oki_doki, http_receive);
			} break;

			default:
				jeeves_error_send_response (error, http_receive);
				break;
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
	}

}

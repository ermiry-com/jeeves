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
#include "worker.h"

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
		char *json = transactions_get_all_by_user_to_json (
			&user->oid, NULL,
			&json_len
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
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
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
				#ifdef JEEVES_DEBUG
				(void) printf ("name: \"%s\"\n", *name);
				#endif
			}

			else if (!strcmp (key, "description")) {
				*description = json_string_value (value);
				#ifdef JEEVES_DEBUG
				(void) printf ("description: \"%s\"\n", *description);
				#endif
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
				*job = jeeves_job_create (
					user_id,
					name, description
				);

				if (*job == NULL) error = JEEVES_ERROR_SERVER_ERROR;
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
			if (!jeeves_job_insert_one (job)) {
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

			jeeves_job_return (job);
		}

		else {
			jeeves_error_send_response (error, http_receive);
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
						http_receive, 200, json, json_len
					);
					
					free (json);
				}

				else {
					(void) http_response_send (server_error, http_receive);
				}
			}

			else {
				// TODO: change to job not found response
				(void) http_response_send (server_error, http_receive);
			}
		}
	}

	else {
		(void) http_response_send (bad_user_error, http_receive);
	}

}

static void jeeves_job_config_parse_json (
	json_t *json_body,
	const char **type
) {

	// get values from json to create a new transaction
	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (json_body) == JSON_OBJECT) {
		json_object_foreach (json_body, key, value) {
			if (!strcmp (key, "type")) {
				*type = json_string_value (value);
				(void) printf ("type: \"%s\"\n", *type);
			}
		}
	}

}

static JeevesError jeeves_job_config_handler_internal (
	const String *request_body, const char *user_id,
	JeevesJob *job
) {

	JeevesError error = JEEVES_ERROR_NONE;

	if (request_body) {
		// the job type to be executed
		const char *type = NULL;

		json_error_t json_error =  { 0 };
		json_t *json_body = json_loads (request_body->str, 0, &json_error);
		if (json_body) {
			jeeves_job_config_parse_json (
				json_body,
				&type
			);

			if (type) {
				// set configuration to current job
				job->type = job_type_from_string (type);
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

// POST /api/jeeves/jobs/:id/config
void jeeves_job_config_handler (
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
			// get configuration for selected job
			JeevesError error = jeeves_job_config_handler_internal (
				request->body, user->id,
				job
			);

			if (error == JEEVES_ERROR_NONE) {
				// update job's configuration in the db
				if (!mongo_update_one (
					jobs_collection,
					jeeves_job_query_oid (&job->oid),
					jeeves_job_config_update_bson (job)
				)) {
					// check if the job is ready to be started
					if (job->n_images) {
						(void) mongo_update_one (
							jobs_collection,
							jeeves_job_query_oid (&job->oid),
							jeeves_job_status_update_bson (JOB_STATUS_READY)
						);
					}

					(void) http_response_send (oki_doki, http_receive);
				}

				else {
					(void) http_response_send (server_error, http_receive);
				}
			}

			else {
				jeeves_error_send_response (error, http_receive);
			}

			jeeves_job_return (job);
		}

		else {
			cerver_log_warning (
				"config_handler () - Job %s does NOT belong to user %s",
				job_id->str, user->id
			);

			http_request_multi_part_discard_files (request);
			(void) http_response_send (bad_request, http_receive);
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
					job_image->original, JOB_IMAGE_ORIGINAL_LEN,
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
		(void) http_response_send (bad_request, http_receive);
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
			(void) http_response_send (bad_request, http_receive);
		}
	}

	else {
		http_request_multi_part_discard_files (request);
		(void) http_response_send (bad_user_error, http_receive);
	}

}

static void jeeves_job_start_handler_actual (
	const HttpReceive *http_receive,
	JeevesJob *job
) {

	// check if the job has NOT been started
	if (
		jeeves_jobs_worker_check (&job->oid), 
		(job->status != JOB_STATUS_RUNNING)
		&& (job->status == JOB_STATUS_READY)
	) {
		// start job
		if (!jeeves_jobs_worker_create (job)) {
			// update the job in the db
			if (!mongo_update_one (
				jobs_collection,
				jeeves_job_query_oid (&job->oid),
				jeeves_job_start_update_bson ()
			)) {
				(void) http_response_send (oki_doki, http_receive);
			}

			else {
				jeeves_job_return (job);
				cerver_log_error (
					"jeeves_job_start_handler () - failed to update job status"
				);
				(void) http_response_send (server_error, http_receive);
			}
		}

		else {
			jeeves_job_return (job);
			(void) http_response_send (server_error, http_receive);
		}
	}

	else {
		(void) http_response_send (bad_request, http_receive);
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
			jeeves_job_start_handler_actual (
				http_receive,
				job
			);
		}

		else {
			(void) http_response_send (bad_request, http_receive);
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
		(void) http_response_send (bad_user_error, http_receive);
	}

}
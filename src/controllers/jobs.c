#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/string.h>

#include <cerver/collections/pool.h>

#include <cerver/http/response.h>
#include <cerver/http/json/json.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include <cmongo/crud.h>
#include <cmongo/select.h>

#include "errors.h"
#include "jeeves.h"
#include "worker.h"

#include "models/job.h"

#include "controllers/jobs.h"
#include "controllers/users.h"

static Pool *jobs_pool = NULL;

const bson_t *job_no_user_query_opts = NULL;
static CMongoSelect *job_no_user_select = NULL;

HttpResponse *no_user_jobs = NULL;
HttpResponse *no_user_job = NULL;

HttpResponse *job_created_bad = NULL;
HttpResponse *job_deleted_bad = NULL;

void jeeves_job_return (void *jobs_ptr);

static unsigned int jeeves_jobs_init_pool (void) {

	unsigned int retval = 1;

	jobs_pool = pool_create (jeeves_job_delete);
	if (jobs_pool) {
		pool_set_create (jobs_pool, jeeves_job_new);
		pool_set_produce_if_empty (jobs_pool, true);
		if (!pool_init (jobs_pool, jeeves_job_new, DEFAULT_JOBS_POOL_INIT)) {
			retval = 0;
		}

		else {
			cerver_log_error ("Failed to init jobs pool!");
		}
	}

	else {
		cerver_log_error ("Failed to create jobs pool!");
	}

	return retval;

}

static unsigned int jeeves_jobs_init_query_opts (void) {

	unsigned int retval = 1;

	job_no_user_select = cmongo_select_new ();

	(void) cmongo_select_insert_field (job_no_user_select, "name");
	(void) cmongo_select_insert_field (job_no_user_select, "description");

	(void) cmongo_select_insert_field (job_no_user_select, "status");

	(void) cmongo_select_insert_field (job_no_user_select, "type");

	(void) cmongo_select_insert_field (job_no_user_select, "imagesCount");

	(void) cmongo_select_insert_field (job_no_user_select, "created");
	(void) cmongo_select_insert_field (job_no_user_select, "started");
	(void) cmongo_select_insert_field (job_no_user_select, "ended");

	job_no_user_query_opts = mongo_find_generate_opts (job_no_user_select);

	if (job_no_user_query_opts) retval = 0;

	return retval;

}

static unsigned int jeeves_jobs_init_responses (void) {

	unsigned int retval = 1;

	no_user_jobs = http_response_json_key_value (
		HTTP_STATUS_NOT_FOUND, "msg", "No user's jobs"
	);

	no_user_job = http_response_json_key_value (
		HTTP_STATUS_NOT_FOUND, "msg", "User's job was not found"
	);

	job_created_bad = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Failed to create job!"
	);

	job_deleted_bad = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Failed to delete job!"
	);

	if (
		no_user_jobs && no_user_job
		&& job_created_bad && job_deleted_bad
	) retval = 0;

	return retval;

}

unsigned int jeeves_jobs_init (void) {

	unsigned int errors = 0;

	errors |= jeeves_jobs_init_pool ();

	errors |= jeeves_jobs_init_query_opts ();

	errors |= jeeves_jobs_init_responses ();

	return errors;

}

void jeeves_jobs_end (void) {

	http_response_delete (job_created_bad);
	http_response_delete (job_deleted_bad);

	pool_delete (jobs_pool);
	jobs_pool = NULL;

}

static JeevesJob *jeeves_job_create_actual (
	const char *user_id,
	const char *name,
	const char *description
) {

	JeevesJob *job = (JeevesJob *) pool_pop (jobs_pool);
	if (job) {
		bson_oid_init (&job->oid, NULL);

		bson_oid_init_from_string (&job->user_oid, user_id);

		if (name) (void) strncpy (job->name, name, JOB_NAME_SIZE - 1);
		if (description) (void) strncpy (job->description, description, JOB_DESCRIPTION_SIZE - 1);

		job->status = JOB_STATUS_WAITING;

		job->created = time (NULL);
	}

	return job;

}

unsigned int jeeves_jobs_get_all_by_user_to_json (
	const bson_oid_t *user_oid,
	char **json, size_t *json_len
) {

	return jobs_get_all_by_user_to_json (
		user_oid, job_no_user_query_opts,
		json, json_len
	);

}

JeevesJob *jeeves_job_get_by_id_and_user (
	const String *job_id, const bson_oid_t *user_oid
) {

	JeevesJob *job = NULL;

	if (job_id && user_oid) {
		job = (JeevesJob *) pool_pop (jobs_pool);
		if (job) {
			bson_oid_init_from_string (&job->oid, job_id->str);

			if (jeeves_job_get_by_oid_and_user (
				job,
				&job->oid, user_oid,
				NULL
			)) {
				jeeves_job_return (job);
				job = NULL;
			}
		}
	}

	return job;

}

u8 jeeves_job_get_by_id_and_user_to_json (
	const char *job_id, const bson_oid_t *user_oid,
	const bson_t *query_opts,
	char **json, size_t *json_len
) {

	u8 retval = 1;

	if (job_id) {
		bson_oid_t job_oid = { 0 };
		bson_oid_init_from_string (&job_oid, job_id);

		retval = jeeves_job_get_by_oid_and_user_to_json (
			&job_oid, user_oid,
			query_opts,
			json, json_len
		);
	}

	return retval;

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

static JeevesError jeeves_job_create_parse_json (
	JeevesJob **job,
	const char *user_id, const String *request_body
) {

	JeevesError error = JEEVES_ERROR_NONE;

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
			*job = jeeves_job_create_actual (
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
		#ifdef JEEVES_DEBUG
		cerver_log_error (
			"json_loads () - json error on line %d: %s\n",
			json_error.line, json_error.text
		);
		#endif

		error = JEEVES_ERROR_BAD_REQUEST;
	}

	return error;

}

JeevesError jeeves_job_create (
	const User *user, const String *request_body
) {

	JeevesError error = JEEVES_ERROR_NONE;

	if (request_body) {
		JeevesJob *job = NULL;

		error = jeeves_job_create_parse_json (
			&job,
			user->id, request_body
		);

		if (error == JEEVES_ERROR_NONE) {
			#ifdef JEEVES_DEBUG
			jeeves_job_print (job);
			#endif

			// insert into the db
			if (!jeeves_job_insert_one (job)) {
				cerver_log_success ("Saved job %s!", job->id);
			}

			else {
				error = JEEVES_ERROR_SERVER_ERROR;
			}

			jeeves_job_return (job);
		}
	}

	else {
		#ifdef JEEVES_DEBUG
		cerver_log_error ("Missing request body to create job!");
		#endif

		error = JEEVES_ERROR_BAD_REQUEST;
	}

	return error;

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
				#ifdef JEEVES_DEBUG
				(void) printf ("type: \"%s\"\n", *type);
				#endif
			}
		}
	}

}

static JeevesError jeeves_job_config_internal (
	JeevesJob *job, const String *request_body
) {

	JeevesError error = JEEVES_ERROR_NONE;

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

	return error;

}

JeevesError jeeves_job_config (
	const User *user, const String *job_id,
	const String *request_body
) {

	JeevesError error = JEEVES_ERROR_NONE;

	if (request_body) {
		JeevesJob *job = jeeves_job_get_by_id_and_user (
			job_id, &user->oid
		);

		if (job) {
			error = jeeves_job_config_internal (job, request_body);

			if (error == JEEVES_ERROR_NONE) {
				// update job's configuration in the db
				if (!jeeves_job_update_config (job)) {
					// check if the job is ready to be started
					if (job->n_images) {
						(void) jeeves_job_update_status (
							&job->oid, JOB_STATUS_READY
						);
					}
				}

				else {
					error = JEEVES_ERROR_SERVER_ERROR;
				}
			}

			jeeves_job_return (job);
		}

		else {
			#ifdef JEEVES_DEBUG
			cerver_log_error ("Job was not found!");
			#endif

			error = JEEVES_ERROR_BAD_REQUEST;
		}
	}

	else {
		#ifdef JEEVES_DEBUG
		cerver_log_error ("jeeves_job_config () - Missing request body");
		#endif

		error = JEEVES_ERROR_BAD_REQUEST;
	}

	return error;

}

JeevesError jeeves_job_upload (
	const User *user, const String *job_id,
	DoubleList *filenames, const char *dirname
) {

	JeevesError error = JEEVES_ERROR_NONE;

	JeevesJob *job = jeeves_job_get_by_id_and_user (
		job_id, &user->oid
	);

	if (job) {
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
					user->id,
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
		if (!jeeves_job_update_images (&job->oid, images)) {
			// check if the job is ready to be started
			if (job->type != JOB_TYPE_NONE) {
				(void) jeeves_job_update_status (
					&job->oid, JOB_STATUS_READY
				);
			}

			// request UPLOADS worker to save frames to persistent storage
			(void) jeeves_uploads_worker_push (
				jeeves_upload_new (dirname, user->id)
			);
		}

		else {
			error = JEEVES_ERROR_SERVER_ERROR;
		}

		dlist_delete (images);

		jeeves_job_return (job);
	}

	else {
		error = JEEVES_ERROR_BAD_REQUEST;
	}

	return error;

}

JeevesError jeeves_job_start (
	const User *user, const String *job_id
) {

	JeevesError error = JEEVES_ERROR_NONE;

	JeevesJob *job = jeeves_job_get_by_id_and_user (
		job_id, &user->oid
	);

	if (job) {
		// check if the job has NOT been started
		if (
			jeeves_jobs_worker_check (&job->oid),
			(job->status != JOB_STATUS_RUNNING)
			&& (job->status == JOB_STATUS_READY)
		) {
			// start job
			if (!jeeves_jobs_worker_create (job)) {
				// update the job in the db
				if (!jeeves_job_update_start (&job->oid)) {
					cerver_log_success ("Job %s has started!", job->id);
				}

				else {
					cerver_log_error (
						"jeeves_job_start_handler () - "
						"failed to update job %s status",
						job->id
					);

					error = JEEVES_ERROR_SERVER_ERROR;
				}
			}

			else {
				error = JEEVES_ERROR_SERVER_ERROR;
			}
		}

		else {
			error = JEEVES_ERROR_BAD_REQUEST;
		}

		jeeves_job_return (job);
	}

	else {
		error = JEEVES_ERROR_BAD_REQUEST;
	}

	return error;

}

JeevesError jeeves_job_stop (
	const User *user, const String *job_id
) {

	JeevesError error = JEEVES_ERROR_NONE;

	JeevesJob *job = jeeves_job_get_by_id_and_user (
		job_id, &user->oid
	);

	if (job) {
		// TODO: stop job

		// update the job in the db
		if (!jeeves_job_update_stop (&job->oid)) {
			cerver_log_success ("Job %s has been stopped!", job->id);
		}

		jeeves_job_return (job);
	}

	else {
		error = JEEVES_ERROR_BAD_REQUEST;
	}

	return error;

}

void jeeves_job_return (void *job_ptr) {

	if (job_ptr) {
		JeevesJob *job = (JeevesJob *) job_ptr;

		dlist_reset (job->images);

		DoubleList *temp = job->images;

		(void) memset (job, 0, sizeof (JeevesJob));

		job->images = temp;

		(void) pool_push (jobs_pool, job_ptr);
	}

}

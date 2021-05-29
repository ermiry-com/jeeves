#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/string.h>

#include <cerver/collections/pool.h>

#include <cerver/http/response.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include "jeeves.h"

#include "models/job.h"

#include "controllers/jobs.h"

static Pool *jobs_pool = NULL;

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

static unsigned int jeeves_jobs_init_responses (void) {

	unsigned int retval = 1;

	job_created_bad = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Failed to create job!"
	);

	job_deleted_bad = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Failed to delete job!"
	);

	if (job_created_bad && job_deleted_bad) retval = 0;

	return retval;

}

unsigned int jeeves_jobs_init (void) {

	unsigned int errors = 0;

	errors |= jeeves_jobs_init_pool ();

	errors |= jeeves_jobs_init_responses ();

	return errors;

}

void jeeves_jobs_end (void) {

	http_response_delete (job_created_bad);
	http_response_delete (job_deleted_bad);

	pool_delete (jobs_pool);
	jobs_pool = NULL;

}

JeevesJob *jeeves_job_create (
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

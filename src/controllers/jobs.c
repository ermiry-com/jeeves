#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/string.h>

#include <cerver/collections/pool.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

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

unsigned int jeeves_jobs_init (void) {

	unsigned int errors = 0;

	errors |= jeeves_jobs_init_pool ();

	return errors;

}

void jeeves_jobs_end (void) {

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

		if (name) (void) strncpy (job->name, name, JOB_NAME_LEN);
		if (description) (void) strncpy (job->description, description, JOB_DESCRIPTION_LEN);

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

void jeeves_job_return (void *jobs_ptr) {

	if (jobs_ptr) {
		(void) memset (jobs_ptr, 0, sizeof (JeevesJob));
		(void) pool_push (jobs_pool, jobs_ptr);
	}

}
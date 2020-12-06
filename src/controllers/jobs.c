#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/collections/pool.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"

#include "models/job.h"

#include "controllers/jobs.h"

static Pool *jobs_pool = NULL;

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
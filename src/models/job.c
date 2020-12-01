#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <cerver/utils/log.h>

#include "mongo.h"

#include "models/job.h"

#define JOBS_COLL_NAME         				"jobs"

mongoc_collection_t *jobs_collection = NULL;

// opens handle to jobs collection
unsigned int jobs_collection_get (void) {

	unsigned int retval = 1;

	jobs_collection = mongo_collection_get (JOBS_COLL_NAME);
	if (jobs_collection) {
		cerver_log_debug ("Opened handle to jobs collection!");
		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get handle to jobs collection!");
	}

	return retval;

}

void jobs_collection_close (void) {

	if (jobs_collection) mongoc_collection_destroy (jobs_collection);

}

void *jeeves_job_new (void) {

	JeevesJob *job = (JeevesJob *) malloc (sizeof (JeevesJob));
	if (job) {
		(void) memset (job, 0, sizeof (JeevesJob));
	}

	return job;

}

void jeeves_job_delete (void *job_ptr) {

	if (job_ptr) free (job_ptr);

}

void jeeves_job_print (JeevesJob *job) {

	if (job) {
		(void) printf ("id: %s\n", job->id);
		(void) printf ("name: %s\n", job->name);
		(void) printf ("description: %s\n", job->description);

		char buffer[128] = { 0 };
		(void) strftime (buffer, 128, "%d/%m/%y - %T", gmtime (&job->created));
		(void) printf ("created: %s GMT\n", buffer);
		(void) strftime (buffer, 128, "%d/%m/%y - %T", gmtime (&job->started));
		(void) printf ("started: %s GMT\n", buffer);
		(void) strftime (buffer, 128, "%d/%m/%y - %T", gmtime (&job->ended));
		(void) printf ("ended: %s GMT\n", buffer);
	}

}

// get all the jobs that are related to a user
mongoc_cursor_t *jeeves_jobs_get_all_by_user (
	const bson_oid_t *user_oid, const bson_t *opts
) {

	mongoc_cursor_t *retval = NULL;

	if (user_oid) {
		bson_t *query = bson_new ();
		if (query) {
			(void) bson_append_oid (query, "user", -1, user_oid);

			retval = mongo_find_all_cursor_with_opts (
				jobs_collection,
				query, opts
			);
		}
	}

	return retval;

}
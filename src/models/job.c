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

static void jeeves_job_doc_parse (
	JeevesJob *job, const bson_t *job_doc
) {

	bson_iter_t iter = { 0 };
	if (bson_iter_init (&iter, job_doc)) {
		char *key = NULL;
		bson_value_t *value = NULL;
		while (bson_iter_next (&iter)) {
			key = (char *) bson_iter_key (&iter);
			value = (bson_value_t *) bson_iter_value (&iter);

			if (!strcmp (key, "_id")) {
				bson_oid_copy (&value->value.v_oid, &job->oid);
				bson_oid_to_string (&job->oid, job->id);
			}

			else if (!strcmp (key, "user"))
				bson_oid_copy (&value->value.v_oid, &job->user_oid);

			else if (!strcmp (key, "name") && value->value.v_utf8.str) 
				(void) strncpy (job->name, value->value.v_utf8.str, JOB_NAME_LEN);

			else if (!strcmp (key, "description") && value->value.v_utf8.str) 
				(void) strncpy (job->description, value->value.v_utf8.str, JOB_DESCRIPTION_LEN);

			else if (!strcmp (key, "created")) 
				job->created = (time_t) bson_iter_date_time (&iter) / 1000;

			else if (!strcmp (key, "started")) 
				job->started = (time_t) bson_iter_date_time (&iter) / 1000;

			else if (!strcmp (key, "ended")) 
				job->ended = (time_t) bson_iter_date_time (&iter) / 1000;
		}
	}

}

bson_t *jeeves_job_query_oid (const bson_oid_t *oid) {

	bson_t *query = NULL;

	if (oid) {
		query = bson_new ();
		if (query) {
			(void) bson_append_oid (query, "_id", -1, oid);
		}
	}

	return query;

}

const bson_t *jeeves_job_find_by_oid_and_user (
	const bson_oid_t *oid, const bson_oid_t *user_oid,
	const bson_t *query_opts
) {

	const bson_t *retval = NULL;

	bson_t *job_query = bson_new ();
	if (job_query) {
		(void) bson_append_oid (job_query, "_id", -1, oid);
		(void) bson_append_oid (job_query, "user", -1, user_oid);

		retval = mongo_find_one_with_opts (jobs_collection, job_query, query_opts);
	}

	return retval;

}

u8 jeeves_job_get_by_oid_and_user (
	JeevesJob *job,
	const bson_oid_t *oid, const bson_oid_t *user_oid,
	const bson_t *query_opts
) {

	u8 retval = 1;

	if (job) {
		const bson_t *job_doc = jeeves_job_find_by_oid_and_user (oid, user_oid, query_opts);
		if (job_doc) {
			jeeves_job_doc_parse (job, job_doc);
			bson_destroy ((bson_t *) job_doc);

			retval = 0;
		}
	}

	return retval;

}

bson_t *jeeves_job_to_bson (JeevesJob *job) {

	bson_t *doc = NULL;

    if (job) {
        doc = bson_new ();
        if (doc) {
            (void) bson_append_oid (doc, "_id", -1, &job->oid);

			(void) bson_append_oid (doc, "user", -1, &job->user_oid);

			(void) bson_append_utf8 (doc, "name", -1, job->name, -1);
			(void) bson_append_utf8 (doc, "description", -1, job->description, -1);

			(void) bson_append_date_time (doc, "created", -1, job->created * 1000);
			(void) bson_append_date_time (doc, "started", -1, job->started * 1000);
			(void) bson_append_date_time (doc, "ended", -1, job->ended * 1000);
        }
    }

    return doc;

}

bson_t *jeeves_job_update_bson (JeevesJob *job) {

	bson_t *doc = NULL;

    if (job) {
        doc = bson_new ();
        if (doc) {
			bson_t set_doc = { 0 };
			(void) bson_append_document_begin (doc, "$set", -1, &set_doc);

			(void) bson_append_utf8 (&set_doc, "name", -1, job->name, -1);
			(void) bson_append_utf8 (&set_doc, "description", -1, job->description, -1);

			(void) bson_append_document_end (doc, &set_doc);
        }
    }

    return doc;

}

bson_t *jeeves_job_start_update_bson (void) {

	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		bson_append_date_time (&set_doc, "started", -1, time (NULL));
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

bson_t *jeeves_job_stop_update_bson (void) {
	
	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		bson_append_date_time (&set_doc, "stopped", -1, time (NULL));
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

bson_t *jeeves_job_end_update_bson (void) {
	
	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		bson_append_date_time (&set_doc, "ended", -1, time (NULL));
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

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
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

const char *job_status_to_string (JobStatus status) {

	switch (status) {
		#define XX(num, name, string) case JOB_STATUS_##name: return #string;
		JOB_STATUS_MAP(XX)
		#undef XX
	}

	return job_status_to_string (JOB_STATUS_NONE);

}

const char *job_type_to_string (JobType type) {

	switch (type) {
		#define XX(num, name, string) case JOB_TYPE_##name: return #string;
		JOB_TYPE_MAP(XX)
		#undef XX
	}

	return job_type_to_string (JOB_TYPE_NONE);

}

JobType job_type_from_string (const char *type_string) {

	JobType type = JOB_TYPE_NONE;

	if (type_string) {
		if (!strcmp (type_string, "GRAYSCALE")) type = JOB_TYPE_GRAYSCALE;
		else if (!strcmp (type_string, "SHIFT")) type = JOB_TYPE_SHIFT;
		else if (!strcmp (type_string, "CLAMP")) type = JOB_TYPE_CLAMP;
		else if (!strcmp (type_string, "RGB_TO_HUE")) type = JOB_TYPE_RGB_TO_HUE;
	}

	return type;

}

JobImage *job_image_new (void) {

	JobImage *job_image = (JobImage *) malloc (sizeof (JobImage));
	if (job_image) {
		(void) memset (job_image, 0, sizeof (JobImage));
	}

	return job_image;

}

void job_image_delete (void *job_image_ptr) {

	if (job_image_ptr) free (job_image_ptr);

}

JobImage *job_image_create (
	const int image_id,
	const char *original, const char *result
) {

	JobImage *job_image = job_image_new ();
	if (job_image) {
		job_image->id = image_id;
		(void) strncpy (job_image->original, original, JOB_IMAGE_ORIGINAL_LEN);
		(void) strncpy (job_image->result, result, JOB_IMAGE_RESULT_LEN);
	}

	return job_image;

}

bson_t *job_image_to_bson (JobImage *job_image) {

	bson_t *doc = NULL;

    if (job_image) {
        doc = bson_new ();
        if (doc) {
			(void) bson_append_int32 (doc, "_id", -1, job_image->id);
			(void) bson_append_utf8 (doc, "original", -1, job_image->original, -1);
			(void) bson_append_utf8 (doc, "result", -1, job_image->result, -1);
        }
    }

    return doc;

}

void *jeeves_job_new (void) {

	JeevesJob *job = (JeevesJob *) malloc (sizeof (JeevesJob));
	if (job) {
		(void) memset (job, 0, sizeof (JeevesJob));

		job->images = dlist_init (job_image_delete, NULL);
	}

	return job;

}

void jeeves_job_delete (void *job_ptr) {

	if (job_ptr) {
		JeevesJob *job = (JeevesJob *) job_ptr;

		dlist_delete (job->images);
		job->images = NULL;

		free (job_ptr);
	}

}

void jeeves_job_print (JeevesJob *job) {

	if (job) {
		(void) printf ("id: %s\n", job->id);
		(void) printf ("name: %s\n", job->name);
		(void) printf ("description: %s\n", job->description);

		(void) printf ("type: %s\n", job_type_to_string (job->type));;

		char buffer[128] = { 0 };
		(void) strftime (buffer, 128, "%d/%m/%y - %T", gmtime (&job->created));
		(void) printf ("created: %s GMT\n", buffer);
		(void) strftime (buffer, 128, "%d/%m/%y - %T", gmtime (&job->started));
		(void) printf ("started: %s GMT\n", buffer);
		(void) strftime (buffer, 128, "%d/%m/%y - %T", gmtime (&job->ended));
		(void) printf ("ended: %s GMT\n", buffer);
	}

}


void jeeves_job_doc_parse_images (JeevesJob *job, bson_iter_t *iter) {

	const u8 *data = NULL;
	u32 len = 0;
	bson_iter_array (iter, &len, &data);

	bson_t *images_array = bson_new_from_data (data, len);
	bson_iter_t array_iter = { 0 };
	if (bson_iter_init (&array_iter, images_array)) {
		while (bson_iter_next (&array_iter)) {
			// const char *key = bson_iter_key (&array_iter);
			const bson_value_t *value = bson_iter_value (&array_iter);

			const u8 *data = value->value.v_doc.data;
			u32 len = value->value.v_doc.data_len;

			bson_t *image_doc = bson_new_from_data (data, len);
			if (image_doc) {
				bson_iter_t at_iter = { 0 };
				if (bson_iter_init (&at_iter, image_doc)) {
					JobImage *job_image = job_image_new ();
					while (bson_iter_next (&at_iter)) {
						const char *key = bson_iter_key (&at_iter);
						const bson_value_t *value = bson_iter_value (&at_iter);

						if (!strcmp (key, "_id")) {
							// printf ("%d\n", value->value.v_int32);
							job_image->id = value->value.v_int32;
						}

						else if (!strcmp (key, "original")) {
							// printf ("%s\n", value->value.v_utf8.str);
							(void) strncpy (
								job_image->original,
								value->value.v_utf8.str,
								JOB_IMAGE_ORIGINAL_LEN
							);
						}

						else if (!strcmp (key, "result")) {
							// printf ("%s\n", value->value.v_utf8.str);
							(void) strncpy (
								job_image->result,
								value->value.v_utf8.str,
								JOB_IMAGE_RESULT_LEN
							);
						}
					}

					(void) dlist_insert_at_end_unsafe (
						job->images, job_image
					);
				}

				bson_destroy (image_doc);
			}
		}
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

			else if (!strcmp (key, "status"))
				job->status = (JobStatus) value->value.v_int32;

			else if (!strcmp (key, "type"))
				job->type = (JobType) value->value.v_int32;

			else if (!strcmp (key, "imagesCount"))
				job->n_images = value->value.v_int32;

			else if (!strcmp (key, "images")) {
				jeeves_job_doc_parse_images (job, &iter);
			}

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

bson_t *jeeves_job_query_oid_and_user (
	const bson_oid_t *oid, const bson_oid_t *user_oid
) {

	bson_t *job_query = NULL;
	
	if (oid && user_oid) {
		job_query = bson_new ();
		if (job_query) {
			(void) bson_append_oid (job_query, "_id", -1, oid);
			(void) bson_append_oid (job_query, "user", -1, user_oid);
		}
	}

	return job_query;

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

			(void) bson_append_int32 (doc, "status", -1, job->status);

			(void) bson_append_int32 (doc, "type", -1, job->type);

			(void) bson_append_int32 (doc, "imagesCount", -1, job->n_images);

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

bson_t *jeeves_job_config_update_bson (JeevesJob *job) {

	bson_t *doc = NULL;

    if (job) {
        doc = bson_new ();
        if (doc) {
			bson_t set_doc = { 0 };
			(void) bson_append_document_begin (doc, "$set", -1, &set_doc);

			(void) bson_append_int32 (&set_doc, "type", -1, job->type);

			(void) bson_append_document_end (doc, &set_doc);
        }
    }

    return doc;

}

bson_t *jeeves_job_status_update_bson (JobStatus status) {

	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		(void) bson_append_int32 (&set_doc, "status", -1, status);
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

bson_t *jeeves_job_type_update_bson (JobType type) {

	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		(void) bson_append_int32 (&set_doc, "type", -1, type);
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

static void jeeves_job_images_add_update_count_bson (
	bson_t *doc, int images_size
) {

	bson_t *inc_doc = bson_new ();
	if (inc_doc) {
		(void) bson_append_int32 (inc_doc, "imagesCount", -1, images_size);

		(void) bson_append_document (doc, "$inc", -1, inc_doc);

		bson_destroy (inc_doc);
	}

}

static void jeeves_job_images_add_push_images_bson (
	bson_t *doc, DoubleList *images
) {

	bson_t *push_doc = bson_new ();
	if (push_doc) {
		bson_t *images_doc = bson_new ();
		if (images_doc) {
			bson_t *each_array = bson_new ();
			if (each_array) {
				(void) bson_append_array_begin (images_doc, "$each", -1, each_array);

				char buf[16] = { 0 };
				const char *key = NULL;
				size_t keylen = 0;
				unsigned int i = 0;
				bson_t *job_image_bson = NULL;
				for (ListElement *le = dlist_start (images); le; le = le->next) {
					keylen = bson_uint32_to_string (i, &key, buf, sizeof (buf));

					job_image_bson = job_image_to_bson ((JobImage *) le->data);
					if (job_image_bson) {
						(void) bson_append_document (each_array, key, (int) keylen, job_image_bson);
						bson_destroy (job_image_bson);
					}

					i++;
				}

				(void) bson_append_array_end (images_doc, each_array);
				
				bson_destroy (each_array);
			}

			(void) bson_append_document (push_doc, "images", -1, images_doc);

			bson_destroy (images_doc);
		}
		
		(void) bson_append_document (doc, "$push", -1, push_doc);

		bson_destroy (push_doc);
	}

}

bson_t *jeeves_job_images_add_bson (
	DoubleList *images
) {

	bson_t *doc = NULL;

	if (images) {
		doc = bson_new ();
		if (doc) {
			jeeves_job_images_add_update_count_bson (
				doc, (int) images->size
			);

			jeeves_job_images_add_push_images_bson (
				doc, images
			);

			size_t json_len = 0;
			char *json = bson_as_relaxed_extended_json (doc, &json_len);
			if (json) {
				printf ("\n\n%s\n\n", json);
				free (json);
			}
		}
	}

	return doc;

}

bson_t *jeeves_job_start_update_bson (void) {

	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		(void) bson_append_int32 (&set_doc, "status", -1, JOB_STATUS_RUNNING);
		(void) bson_append_date_time (&set_doc, "started", -1, time (NULL));
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

bson_t *jeeves_job_stop_update_bson (void) {
	
	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		(void) bson_append_int32 (&set_doc, "status", -1, JOB_STATUS_STOPPED);
		(void) bson_append_date_time (&set_doc, "stopped", -1, time (NULL));
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

bson_t *jeeves_job_end_update_bson (void) {
	
	bson_t *doc = bson_new ();
	if (doc) {
		bson_t set_doc = { 0 };
		(void) bson_append_document_begin (doc, "$set", -1, &set_doc);
		(void) bson_append_int32 (&set_doc, "status", -1, JOB_STATUS_DONE);
		(void) bson_append_date_time (&set_doc, "ended", -1, time (NULL));
		(void) bson_append_document_end (doc, &set_doc);
	}

	return doc;

}

int jeeves_job_update_one (bson_t *query, bson_t *update) {

	return mongo_update_one (
		jobs_collection,
		query, update
	);

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
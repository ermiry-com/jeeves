#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <cerver/types/string.h>

#include <cerver/cerver.h>
#include <cerver/files.h>

#include <cerver/threads/bsem.h>
#include <cerver/threads/jobs.h>
#include <cerver/threads/thread.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"
#include "worker.h"

static void *jeeves_uploads_worker_thread (void *null_ptr);

static JobQueue *jeeves_uploads_worker_job_queue = NULL;

JeevesUpload *jeeves_upload_new (const char *dirname, const char *user_id) {

	JeevesUpload *upload = (JeevesUpload *) malloc (sizeof (JeevesUpload));
	if (upload) {
		(void) strncpy (upload->dirname, dirname, JEEVES_UPLOAD_USER_ID_LEN);
		(void) strncpy (upload->user_id, user_id, JEEVES_UPLOAD_USER_ID_LEN);
	}

	return upload;

}

void jeeves_upload_delete (void *jeeves_upload_ptr) {
	
	if (jeeves_upload_ptr) free (jeeves_upload_ptr);

}

unsigned int jeeves_uploads_worker_init (void) {

	unsigned int retval = 1;

	jeeves_uploads_worker_job_queue = job_queue_create ();
	if (jeeves_uploads_worker_job_queue) {
		pthread_t thread_id = 0;
		if (!thread_create_detachable (
			&thread_id, jeeves_uploads_worker_thread, NULL
		)) {
			retval = 0;
		}
	}

	return retval;

}

unsigned int jeeves_uploads_worker_end (void) {

	bsem_post (jeeves_uploads_worker_job_queue->has_jobs);

	(void) sleep (1);

	job_queue_delete (jeeves_uploads_worker_job_queue);

	return 0;

}

unsigned int jeeves_uploads_worker_push (JeevesUpload *upload) {

	return job_queue_push (
		jeeves_uploads_worker_job_queue,
		job_create (NULL, upload)
	);

}

// moves saved uploads from requests
// from temporarly directory into persistant storage
static void *jeeves_uploads_worker_thread (void *null_ptr) {

	sleep (5);			// wait until cerver has started

	cerver_log_success ("Jeeves UPLOADS WORKER thread has started!");

	thread_set_name ("jeeves-uploads-worker");

	Job *job = NULL;

	JeevesUpload *upload = NULL;
	char new_dirname[512] = { 0 };
	char old_location[512] = { 0 };
	char new_location[512] = { 0 };
	char command[2048] = { 0 };
	while (jeeves_cerver->isRunning) {
		bsem_wait (jeeves_uploads_worker_job_queue->has_jobs);
		
		// we are safe to analyze the frames & generate embeddings
		job = job_queue_pull (jeeves_uploads_worker_job_queue);
		if (job) {
			(void) printf ("jeeves_uploads_worker_thread () - New job!\n");
			upload = (JeevesUpload *) job->args;

			(void) printf ("DIRNAME: %s\n", upload->dirname);

			(void) snprintf (
				new_dirname, 512,
				"%s/%s",
				JEEVES_UPLOADS_DIR,
				upload->user_id
			);
			(void) printf ("NEW DIRNAME: %s\n", new_dirname);
			(void) files_create_dir (new_dirname, 0777);

			// move directory from temp storage to local storage
			(void) snprintf (
				old_location, 512,
				"%s/%s",
				JEEVES_UPLOADS_TEMP_DIR,
				upload->dirname
			);
			(void) printf ("OLD: %s\n", old_location);

			(void) snprintf (
				new_location, 512,
				"%s/%s/%s", JEEVES_UPLOADS_DIR,
				upload->user_id, upload->dirname
			);
			(void) printf ("NEW: %s\n", new_location);

			(void) snprintf (
				command, 2048,
				"mv %s %s",
				old_location, new_location
			);
			(void) printf ("COMMAND: %s\n", command);

			(void) system (command);
			
			jeeves_upload_delete (upload);

			job_delete (job);
		}
	}

	cerver_log_success ("Jeeves UPLOADS WORKER thread has exited!");

	return NULL;

}
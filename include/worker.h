#ifndef _JEEVES_WORKER_H_
#define _JEEVES_WORKER_H_

#include <bson/bson.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include "models/job.h"

#pragma region jobs

// returns TRUE if the job is currently being running
extern bool jeeves_jobs_worker_check (const bson_oid_t *job_oid);

// a user has requested to start a new job
// so create a dedicated job & process job's images
// with selected configuration
extern u8 jeeves_jobs_worker_create (JeevesJob *job);

#pragma endregion

#pragma region uploads

#define JEEVES_UPLOAD_DIRNAME_LEN             256
#define JEEVES_UPLOAD_USER_ID_LEN             32

typedef struct JeevesUpload {

	char dirname[JEEVES_UPLOAD_DIRNAME_LEN];
	char user_id[JEEVES_UPLOAD_USER_ID_LEN];

} JeevesUpload;

extern JeevesUpload *jeeves_upload_new (
	const char *dirname, const char *user_id
);

extern void jeeves_upload_delete (
	void *jeeves_upload_ptr
);

extern unsigned int jeeves_uploads_worker_push (
	JeevesUpload *upload
);

#pragma endregion

#pragma region main

extern unsigned int jeeves_worker_init (void);

extern unsigned int jeeves_worker_end (void);

#pragma endregion

#endif
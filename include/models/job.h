#ifndef _MODELS_JOB_H_
#define _MODELS_JOB_H_

#include <time.h>

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include <cerver/types/types.h>

#define JOB_ID_LEN						32
#define JOB_NAME_LEN					512
#define JOB_DESCRIPTION_LEN				1024

extern mongoc_collection_t *jobs_collection;

// opens handle to jobs collection
extern unsigned int jobs_collection_get (void);

extern void jobs_collection_close (void);

typedef struct JeevesJob {

	char id[JOB_ID_LEN];
	bson_oid_t oid;

	char name[JOB_NAME_LEN];
	char description[JOB_DESCRIPTION_LEN];

	time_t created;
	time_t started;
	time_t ended;

} JeevesJob;

extern void *jeeves_job_new (void);

extern void jeeves_job_delete (void *job_ptr);

extern void jeeves_job_print (JeevesJob *job);

#endif
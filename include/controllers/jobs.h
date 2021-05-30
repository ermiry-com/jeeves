#ifndef _JEEVES_JOBS_H_
#define _JEEVES_JOBS_H_

#include <cerver/types/string.h>

#include "errors.h"

#include "models/job.h"

#define DEFAULT_JOBS_POOL_INIT			16

struct _HttpResponse;

extern struct _HttpResponse *no_user_jobs;
extern struct _HttpResponse *no_user_job;

extern struct _HttpResponse *job_created_bad;
extern struct _HttpResponse *job_deleted_bad;

extern unsigned int jeeves_jobs_init (void);

extern void jeeves_jobs_end (void);

extern unsigned int jeeves_jobs_get_all_by_user_to_json (
	const bson_oid_t *user_oid,
	char **json, size_t *json_len
);

extern JeevesJob *jeeves_job_get_by_id_and_user (
	const String *job_id, const bson_oid_t *user_oid
);

extern u8 jeeves_job_get_by_id_and_user_to_json (
	const char *job_id, const bson_oid_t *user_oid,
	const bson_t *query_opts,
	char **json, size_t *json_len
);

extern JeevesError jeeves_job_create (
	const User *user, const String *request_body
);

extern JeevesError jeeves_job_config (
	const User *user, const String *job_id,
	const String *request_body
);

extern JeevesError jeeves_job_upload (
	const User *user, const String *job_id,
	DoubleList *filenames, const char *dirname
);

extern JeevesError jeeves_job_start (
	const User *user, const String *job_id
);

extern JeevesError jeeves_job_stop (
	const User *user, const String *job_id
);

extern void jeeves_job_return (void *job_ptr);

#endif
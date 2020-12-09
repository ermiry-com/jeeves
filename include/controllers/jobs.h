#ifndef _JEEVES_JOBS_H_
#define _JEEVES_JOBS_H_

#include <cerver/types/string.h>

#include "models/job.h"

#define DEFAULT_JOBS_POOL_INIT			16

extern unsigned int jeeves_jobs_init (void);

extern void jeeves_jobs_end (void);

extern JeevesJob *jeeves_job_create (
	const char *user_id,
	const char *name,
	const char *description
);

extern JeevesJob *jeeves_job_get_by_id_and_user (
	const String *job_id, const bson_oid_t *user_oid
);

extern void jeeves_job_return (void *job_ptr);

#endif
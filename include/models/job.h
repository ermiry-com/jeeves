#ifndef _MODELS_JOB_H_
#define _MODELS_JOB_H_

#include <time.h>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <cerver/types/types.h>

#include <cerver/collections/dlist.h>

#define JOB_ID_LEN						32
#define JOB_NAME_LEN					512
#define JOB_DESCRIPTION_LEN				1024

#define JOB_IMAGE_SAVED_LEN				512
#define JOB_IMAGE_ORIGINAL_LEN			512
#define JOB_IMAGE_RESULT_LEN			512

extern unsigned int jobs_model_init (void);

extern void jobs_model_end (void);

#define JOB_STATUS_MAP(XX)						\
	XX(0,	NONE, 			None)				\
	XX(1,	WAITING, 		Waiting)			\
	XX(2,	READY, 			READY)				\
	XX(3,	RUNNING, 		Running)			\
	XX(4,	STOPPED, 		Stopped)			\
	XX(5,	INCOMPLETED, 	Incompleted)		\
	XX(6,	DONE, 			Done)

typedef enum JobStatus {

	#define XX(num, name, string) JOB_STATUS_##name = num,
	JOB_STATUS_MAP (XX)
	#undef XX

} JobStatus;

extern const char *job_status_to_string (const JobStatus status);

#define JOB_TYPE_MAP(XX)						\
	XX(0,	NONE, 			None)				\
	XX(1,	GRAYSCALE, 		GrayScale)			\
	XX(2,	SHIFT, 			Shift)				\
	XX(3,	CLAMP, 			Clamp)				\
	XX(4,	RGB_TO_HUE, 	RGB to HUE)

typedef enum JobType {

	#define XX(num, name, string) JOB_TYPE_##name = num,
	JOB_TYPE_MAP (XX)
	#undef XX

} JobType;

extern const char *job_type_to_string (const JobType type);

extern JobType job_type_from_string (const char *type_string);

typedef struct JobImage {

	int id;
	char saved[JOB_IMAGE_SAVED_LEN];
	char original[JOB_IMAGE_ORIGINAL_LEN];
	char result[JOB_IMAGE_RESULT_LEN];

} JobImage;

extern JobImage *job_image_new (void);

extern void job_image_delete (void *job_image_ptr);

extern JobImage *job_image_create (
	const int image_id,
	const char *saved,
	const char *original,
	const char *result
);

extern bson_t *job_image_to_bson (JobImage *job_image);

typedef struct JeevesJob {

	char id[JOB_ID_LEN];
	bson_oid_t oid;

	bson_oid_t user_oid;

	char name[JOB_NAME_LEN];
	char description[JOB_DESCRIPTION_LEN];

	JobStatus status;

	JobType type;

	int n_images;
	DoubleList *images;

	time_t created;
	time_t started;
	time_t stopped;
	time_t ended;

} JeevesJob;

extern void *jeeves_job_new (void);

extern void jeeves_job_delete (void *job_ptr);

extern void jeeves_job_print (JeevesJob *job);

extern bson_t *jeeves_job_query_oid (const bson_oid_t *oid);

extern bson_t *jeeves_job_query_oid_and_user (
	const bson_oid_t *oid, const bson_oid_t *user_oid
);

extern const bson_t *jeeves_job_find_by_oid_and_user (
	const bson_oid_t *oid, const bson_oid_t *user_oid,
	const bson_t *query_opts
);

extern u8 jeeves_job_get_by_oid_and_user (
	JeevesJob *job,
	const bson_oid_t *oid, const bson_oid_t *user_oid,
	const bson_t *query_opts
);

extern bson_t *jeeves_job_to_bson (JeevesJob *job);

extern bson_t *jeeves_job_update_bson (JeevesJob *job);

extern bson_t *jeeves_job_config_update_bson (JeevesJob *job);

extern bson_t *jeeves_job_status_update_bson (JobStatus status);

extern bson_t *jeeves_job_type_update_bson (JobType type);

extern bson_t *jeeves_job_images_add_bson (DoubleList *images);

extern bson_t *jeeves_job_image_query (
	bson_oid_t *oid, int image_id
);

extern bson_t *jeeves_job_image_result_update (
	const char *result
);

extern bson_t *jeeves_job_start_update_bson (void);

extern bson_t *jeeves_job_stop_update_bson (void);

extern bson_t *jeeves_job_end_update_bson (void);

extern unsigned int jeeves_job_update_one (
	bson_t *query, bson_t *update
);

// get all the jobs that are related to a user
extern mongoc_cursor_t *jeeves_jobs_get_all_by_user (
	const bson_oid_t *user_oid, const bson_t *opts
);

#endif
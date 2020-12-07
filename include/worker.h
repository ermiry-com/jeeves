#ifndef _JEEVES_WORKER_H_
#define _JEEVES_WORKER_H_

#include <cerver/types/string.h>

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

extern unsigned int jeeves_uploads_worker_init (void);

extern unsigned int jeeves_uploads_worker_end (void);

extern unsigned int jeeves_uploads_worker_push (
	JeevesUpload *upload
);

#endif
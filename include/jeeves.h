#ifndef _JEEVES_H_
#define _JEEVES_H_

#include <stdbool.h>

#define JEEVES_UPLOADS_TEMP_DIR			"/var/uploads"
#define JEEVES_UPLOADS_DIR				"/home/jeeves/uploads"

struct _Cerver;
struct _HttpResponse;

extern unsigned int PORT;

extern unsigned int CERVER_RECEIVE_BUFFER_SIZE;
extern unsigned int CERVER_TH_THREADS;
extern unsigned int CERVER_CONNECTION_QUEUE;

extern bool ENABLE_USERS_ROUTES;

extern struct _Cerver *jeeves_cerver;

extern struct _HttpResponse *oki_doki;
extern struct _HttpResponse *bad_request;
extern struct _HttpResponse *server_error;
extern struct _HttpResponse *bad_user;
extern struct _HttpResponse *missing_values;

extern struct _HttpResponse *jeeves_works;
extern struct _HttpResponse *current_version;

extern struct _HttpResponse *job_created_bad;
extern struct _HttpResponse *job_deleted_bad;

// inits jeeves main values
extern unsigned int jeeves_init (void);

// ends jeeves main values
extern unsigned int jeeves_end (void);

#endif
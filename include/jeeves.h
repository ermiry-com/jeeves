#ifndef _JEEVES_H_
#define _JEEVES_H_

#define JEEVES_UPLOADS_TEMP_DIR                   "/var/uploads"

struct _HttpReceive;
struct _HttpRequest;

extern unsigned int PORT;

extern unsigned int CERVER_RECEIVE_BUFFER_SIZE;
extern unsigned int CERVER_TH_THREADS;
extern unsigned int CERVER_CONNECTION_QUEUE;

#pragma region main

// inits jeeves main values
extern unsigned int jeeves_init (void);

// ends jeeves main values
extern unsigned int jeeves_end (void);

#pragma endregion

#pragma region routes

// GET /jeeves
void jeeves_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// GET /jeeves/version
void jeeves_version_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// GET /jeeves/auth
void jeeves_auth_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

#pragma endregion

#endif
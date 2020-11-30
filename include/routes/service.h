#ifndef _JEEVES_SERVICE_USERS_H_
#define _JEEVES_SERVICE_USERS_H_

struct _HttpReceive;
struct _HttpRequest;

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

#endif
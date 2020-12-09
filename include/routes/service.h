#ifndef _JEEVES_SERVICE_USERS_H_
#define _JEEVES_SERVICE_USERS_H_

struct _HttpReceive;
struct _HttpRequest;

// GET /api/jeeves
extern void jeeves_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// GET /api/jeeves/version
extern void jeeves_version_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// GET /api/jeeves/auth
extern void jeeves_auth_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

#endif
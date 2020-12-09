#ifndef _JEEVES_ROUTES_USERS_H_
#define _JEEVES_ROUTES_USERS_H_

struct _HttpReceive;
struct _HttpRequest;

// GET /api/users/
extern void users_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// POST /api/users/login
extern void users_login_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// POST /api/users/register
extern void users_register_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

#endif
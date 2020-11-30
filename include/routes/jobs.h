#ifndef _JEEVES_ROUTES_JOBS_H_
#define _JEEVES_ROUTES_JOBS_H_

struct _HttpReceive;
struct _HttpRequest;

// GET /api/jeeves/jobs
extern void jeeves_get_jobs_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// POST /api/jeeves/jobs
extern void jeeves_create_job_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// GET /api/jeeves/jobs/test
extern void jeeves_jobs_test_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

// GET /api/jeeves/jobs/:id/info
extern void jeeves_job_info_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

#endif
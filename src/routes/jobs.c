#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"

// GET /api/jeeves/jobs
void jeeves_get_jobs_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (oki_doki, http_receive);

}

// POST /api/jeeves/jobs
void jeeves_create_job_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (oki_doki, http_receive);

}

// GET /api/jeeves/jobs/test
void jeeves_jobs_test_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (oki_doki, http_receive);

}

// GET /api/jeeves/jobs/:id/info
void jeeves_job_info_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (oki_doki, http_receive);

}
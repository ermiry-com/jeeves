#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "jeeves.h"

#include "models/user.h"

// GET /jeeves
void jeeves_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (jeeves_works, http_receive);

}

// GET /jeeves/version
void jeeves_version_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (current_version, http_receive);

}

// GET /jeeves/auth
void jeeves_auth_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	User *user = (User *) request->decoded_data;

	if (user) {
		#ifdef JEEVES_DEBUG
		user_print (user);
		#endif

		(void) http_response_send (oki_doki, http_receive);
	}

	else {
		(void) http_response_send (bad_user, http_receive);
	}

}
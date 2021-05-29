#include <cerver/handler.h>

#include <cerver/http/http.h>
#include <cerver/http/response.h>

#include "jeeves.h"
#include "errors.h"

#include "controllers/service.h"

const char *jeeves_error_to_string (JeevesError type) {

	switch (type) {
		#define XX(num, name, string) case JEEVES_ERROR_##name: return #string;
		JEEVES_ERROR_MAP(XX)
		#undef XX
	}

	return jeeves_error_to_string (JEEVES_ERROR_NONE);

}

void jeeves_error_send_response (
	JeevesError error,
	const HttpReceive *http_receive
) {

	switch (error) {
		case JEEVES_ERROR_NONE: break;

		case JEEVES_ERROR_BAD_REQUEST:
			(void) http_response_send (bad_request_error, http_receive);
			break;

		case JEEVES_ERROR_MISSING_VALUES:
			(void) http_response_send (missing_values, http_receive);
			break;

		case JEEVES_ERROR_BAD_USER:
			(void) http_response_send (bad_user_error, http_receive);
			break;

		case JEEVES_ERROR_SERVER_ERROR:
			(void) http_response_send (server_error, http_receive);
			break;

		default: break;
	}

}

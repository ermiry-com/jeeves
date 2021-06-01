#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/http/response.h>

#include <cerver/utils/utils.h>

#include "version.h"

HttpResponse *missing_values = NULL;

HttpResponse *jeeves_works = NULL;
HttpResponse *current_version = NULL;

HttpResponse *catch_all = NULL;

unsigned int jeeves_service_init (void) {

	unsigned int retval = 1;

	missing_values = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Missing values!"
	);

	jeeves_works = http_response_json_key_value (
		HTTP_STATUS_OK, "msg", "Jeeves works!"
	);

	char *status = c_string_create (
		"%s - %s", JEEVES_VERSION_NAME, JEEVES_VERSION_DATE
	);

	if (status) {
		current_version = http_response_json_key_value (
			HTTP_STATUS_OK, "version", status
		);

		free (status);
	}

	catch_all = http_response_json_key_value (
		HTTP_STATUS_OK, "msg", "Jeeves Service!"
	);

	if (
		missing_values
		&& jeeves_works && current_version
		&& catch_all
	) retval = 0;

	return retval;

}

void jeeves_service_end (void) {

	http_response_delete (missing_values);

	http_response_delete (jeeves_works);
	http_response_delete (current_version);

	http_response_delete (catch_all);

}

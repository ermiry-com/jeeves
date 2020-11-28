#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>

static HttpResponse *catch_all = NULL;

unsigned int jeeves_handler_init (void) {

	unsigned int retval = 1;

	catch_all = http_response_json_key_value (
		(http_status) 200, "msg", "Barcel Services V2!"
	);

	if (catch_all) retval = 0;

	return retval;

}

void jeeves_handler_end (void) {

	http_respponse_delete (catch_all);

}

// GET *
void jeeves_catch_all_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	http_response_send (catch_all, http_receive);

}
#include <stdlib.h>

#include <cerver/types/string.h>

#include <cerver/handler.h>

#include <cerver/utils/utils.h>

String *jeeves_uploads_dirname_generator (
	const CerverReceive *cr
) {

	String *dirname = str_new (NULL);
	dirname->str = c_string_create (
		"%d-%ld",
		cr->connection->socket->sock_fd, time (NULL)
	);
	dirname->len = strlen (dirname->str);

	return dirname;

}
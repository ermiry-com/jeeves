#include <cerver/utils/log.h>

#include "version.h"

// print full jeeves version information
void jeeves_version_print_full (void) {

	cerver_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Jeeves Service Version: %s", JEEVES_VERSION_NAME
	);

	cerver_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Release Date & time: %s - %s", JEEVES_VERSION_DATE, JEEVES_VERSION_TIME
	);

	cerver_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Author: %s\n", JEEVES_VERSION_AUTHOR
	);

}

// print the version id
void jeeves_version_print_version_id (void) {

	cerver_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Jeeves Service Version ID: %s\n",
		JEEVES_VERSION
	);

}

// print the version name
void jeeves_version_print_version_name (void) {

	cerver_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Jeeves Service Version: %s\n",
		JEEVES_VERSION_NAME
	);

}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "runtime.h"

const char *runtime_to_string (RuntimeType type) {

	switch (type) {
		#define XX(num, name, string) case RUNTIME_TYPE_##name: return #string;
		RUNTIME_TYPE_MAP(XX)
		#undef XX
	}

	return runtime_to_string (RUNTIME_TYPE_NONE);

}

const RuntimeType runtime_from_string (const char *string) {

	if (string) {
		if (!strcasecmp ("development", string)) return RUNTIME_TYPE_DEVELOPMENT;
		if (!strcasecmp ("test", string)) return RUNTIME_TYPE_TEST;
		if (!strcasecmp ("production", string)) return RUNTIME_TYPE_PRODUCTION;
	}

	return RUNTIME_TYPE_NONE;

}
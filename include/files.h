#ifndef _JEEVES_RUNTIME_H_
#define _JEEVES_RUNTIME_H_

#include <cerver/types/string.h>

#include <cerver/handler.h>

extern String *jeeves_uploads_dirname_generator (
	const CerverReceive *cr
);

#endif
#ifndef _JEEVES_HANDLER_H_
#define _JEEVES_HANDLER_H_

struct _HttpReceive;
struct _HttpRequest;

extern unsigned int jeeves_handler_init (void);

extern void jeeves_handler_end (void);

// GET *
extern void jeeves_catch_all_handler (
	const struct _HttpReceive *http_receive,
	const struct _HttpRequest *request
);

#endif
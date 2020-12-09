#ifndef _JEEVES_ERRORS_H_
#define _JEEVES_ERRORS_H_

#define JEEVES_ERROR_MAP(XX)						\
	XX(0,	NONE, 				None)				\
	XX(1,	BAD_REQUEST, 		Bad Request)		\
	XX(2,	MISSING_VALUES, 	Missing Values)		\
	XX(3,	BAD_USER, 			Bad User)			\
	XX(4,	SERVER_ERROR, 		Server Error)

typedef enum JeevesError {

	#define XX(num, name, string) JEEVES_ERROR_##name = num,
	JEEVES_ERROR_MAP (XX)
	#undef XX

} JeevesError;

extern const char *jeeves_error_to_string (JeevesError type);

extern void jeeves_error_send_response (
	JeevesError error,
	const struct _HttpReceive *http_receive
);

#endif
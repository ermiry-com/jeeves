#ifndef _JEEVES_USERS_H_
#define _JEEVES_USERS_H_

#include <bson/bson.h>

#include <cerver/collections/dlist.h>

#include "models/user.h"

#define DEFAULT_USERS_POOL_INIT			16

struct _HttpReceive;
struct _HttpResponse;

typedef enum JeevesUserInput {

	JEEVES_USER_INPUT_NONE			= 0,
	JEEVES_USER_INPUT_NAME			= 1,
	JEEVES_USER_INPUT_USERNAME		= 2,
	JEEVES_USER_INPUT_EMAIL			= 4,
	JEEVES_USER_INPUT_PASSWORD		= 8,
	JEEVES_USER_INPUT_CONFIRM		= 16,
	JEEVES_USER_INPUT_MATCH			= 32,

} JeevesUserInput;

#define JEEVES_USER_ERROR_MAP(XX)					\
	XX(0,	NONE, 				None)				\
	XX(1,	BAD_REQUEST, 		Bad Request)		\
	XX(2,	MISSING_VALUES, 	Missing Values)		\
	XX(3,	REPEATED, 			Existing Email)		\
	XX(4,	NOT_FOUND, 			Not found)			\
	XX(5,	WRONG_PSWD, 		Wrong password)		\
	XX(6,	SERVER_ERROR, 		Server Error)

typedef enum JeevesUserError {

	#define XX(num, name, string) JEEVES_USER_ERROR_##name = num,
	JEEVES_USER_ERROR_MAP (XX)
	#undef XX

} JeevesUserError;

extern const bson_t *user_login_query_opts;
extern const bson_t *user_transactions_query_opts;
extern const bson_t *user_categories_query_opts;
extern const bson_t *user_places_query_opts;

extern struct _HttpResponse *users_works;
extern struct _HttpResponse *missing_user_values;
extern struct _HttpResponse *wrong_password;
extern struct _HttpResponse *user_not_found;
extern struct _HttpResponse *repeated_email;

extern unsigned int jeeves_users_init (void);

extern void jeeves_users_end (void);

extern User *jeeves_user_create (
	const char *name,
	const char *username,
	const char *email,
	const char *password,
	const bson_oid_t *role_oid
);

extern User *jeeves_user_get (void);

extern User *jeeves_user_get_by_email (const char *email);

extern u8 jeeves_user_check_by_email (
	const char *email
);

// {
//   "email": "erick.salas@ermiry.com",
//   "iat": 1596532954
//   "id": "5eb2b13f0051f70011e9d3af",
//   "name": "Erick Salas",
//   "role": "god",
//   "username": "erick"
// }
extern void *jeeves_user_parse_from_json (void *user_json_ptr);

extern unsigned int jeeves_user_generate_token (
	const User *user, char *json_token, size_t *json_len
);

extern User *jeeves_user_register (
	const String *request_body, 
	JeevesUserError *error, JeevesUserInput *input
);

extern User *jeeves_user_login (
	const String *request_body, 
	JeevesUserError *error, JeevesUserInput *input
);

extern void jeeves_user_delete (void *user_ptr);

#endif
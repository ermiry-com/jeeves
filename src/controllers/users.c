#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/collections/dlist.h>
#include <cerver/collections/pool.h>

#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>
#include <cerver/http/json/json.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include <cmongo/crud.h>
#include <cmongo/select.h>

#include "jeeves.h"

#include "models/user.h"

#include "controllers/roles.h"
#include "controllers/users.h"

static Pool *users_pool = NULL;

const bson_t *user_login_query_opts = NULL;
static CMongoSelect *user_login_select = NULL;

HttpResponse *users_works = NULL;
HttpResponse *missing_user_values = NULL;
HttpResponse *wrong_password = NULL;
HttpResponse *user_not_found = NULL;
HttpResponse *repeated_email = NULL;

static unsigned int jeeves_users_init_pool (void) {

	unsigned int retval = 1;

	users_pool = pool_create (user_delete);
	if (users_pool) {
		pool_set_create (users_pool, user_new);
		pool_set_produce_if_empty (users_pool, true);
		if (!pool_init (users_pool, user_new, DEFAULT_USERS_POOL_INIT)) {
			retval = 0;
		}

		else {
			cerver_log_error ("Failed to init users pool!");
		}
	}

	else {
		cerver_log_error ("Failed to create users pool!");
	}

	return retval;	

}

static unsigned int jeeves_users_init_query_opts (void) {

	unsigned int retval = 1;

	user_login_select = cmongo_select_new ();
	(void) cmongo_select_insert_field (user_login_select, "name");
	(void) cmongo_select_insert_field (user_login_select, "username");
	(void) cmongo_select_insert_field (user_login_select, "email");
	(void) cmongo_select_insert_field (user_login_select, "password");
	(void) cmongo_select_insert_field (user_login_select, "role");

	user_login_query_opts = mongo_find_generate_opts (user_login_select);

	if (
		user_login_query_opts
	) retval = 0;

	return retval;

}

static unsigned int jeeves_users_init_responses (void) {

	unsigned int retval = 1;

	users_works = http_response_json_key_value (
		(http_status) 200, "msg", "Users works!"
	);

	missing_user_values = http_response_json_key_value (
		(http_status) 400, "error", "Missing user values!"
	);

	wrong_password = http_response_json_key_value (
		(http_status) 400, "error", "Password is incorrect!"
	);

	user_not_found = http_response_json_key_value (
		(http_status) 404, "error", "User not found!"
	);

	repeated_email = http_response_json_key_value (
		(http_status) 400, "error", "Email was already registered!"
	);

	if (
		users_works
		&& missing_user_values && wrong_password && user_not_found
	) retval = 0;

	return retval;

}

unsigned int jeeves_users_init (void) {

	unsigned int errors = 0;

	errors |= jeeves_users_init_pool ();

	errors |= jeeves_users_init_query_opts ();

	errors |= jeeves_users_init_responses ();

	return errors;

}

void jeeves_users_end (void) {

	cmongo_select_delete (user_login_select);
	bson_destroy ((bson_t *) user_login_query_opts);

	http_response_delete (users_works);
	http_response_delete (missing_user_values);
	http_response_delete (wrong_password);
	http_response_delete (user_not_found);
	http_response_delete (repeated_email);

	pool_delete (users_pool);
	users_pool = NULL;

}

User *jeeves_user_create (
	const char *name,
	const char *username,
	const char *email,
	const char *password,
	const bson_oid_t *role_oid
) {

	User *user = (User *) pool_pop (users_pool);
	if (user) {
		bson_oid_init (&user->oid, NULL);

		(void) strncpy (user->name, name, USER_NAME_LEN - 1);
		(void) strncpy (user->username, username, USER_USERNAME_LEN - 1);
		(void) strncpy (user->email, email, USER_EMAIL_LEN - 1);
		(void) strncpy (user->password, password, USER_PASSWORD_LEN - 1);

		bson_oid_copy (role_oid, &user->role_oid);
	}

	return user;

}

User *jeeves_user_get_by_email (const char *email) {

	User *user = NULL;
	if (email) {
		user = (User *) pool_pop (users_pool);
		if (user) {
			if (user_get_by_email (user, email, user_login_query_opts)) {
				(void) pool_push (users_pool, user);
				user = NULL;
			}
		}
	}

	return user;

}

u8 jeeves_user_check_by_email (
	const char *email
) {

	return user_check_by_email (email);

}

// {
//   "iat": 1596532954
//   "id": "5eb2b13f0051f70011e9d3af",
//   "email": "erick.salas@ermiry.com",
//   "name": "Erick Salas",
//   "role": "god",
//   "username": "erick",
// }
void *jeeves_user_parse_from_json (void *user_json_ptr) {

	json_t *user_json = (json_t *) user_json_ptr;

	User *user = user_new ();
	if (user) {
		const char *email = NULL;
		const char *id = NULL;
		const char *name = NULL;
		const char *role = NULL;
		const char *username = NULL;

		if (!json_unpack (
			user_json,
			"{s:i, s:s, s:s, s:s, s:s, s:s}",
			"iat", &user->iat,
			"id", &id,
			"email", &email,
			"name", &name,
			"role", &role,
			"username", &username
		)) {
			(void) strncpy (user->email, email, USER_EMAIL_LEN - 1);
			(void) strncpy (user->id, id, USER_ID_LEN - 1);
			(void) strncpy (user->name, name, USER_NAME_LEN - 1);
			(void) strncpy (user->role, role, USER_ROLE_LEN - 1);
			(void) strncpy (user->username, username, USER_USERNAME_LEN - 1);

			bson_oid_init_from_string (&user->oid, user->id);

			if (RUNTIME == RUNTIME_TYPE_DEVELOPMENT) {
				user_print (user);
			}
		}

		else {
			cerver_log_error ("user_parse_from_json () - json_unpack () has failed!");

			(void) pool_push (users_pool, user);
			user = NULL;
		}
	}

	return user;

}

void jeeves_user_delete (void *user_ptr) {

	(void) memset (user_ptr, 0, sizeof (User));
	(void) pool_push (users_pool, user_ptr);

}
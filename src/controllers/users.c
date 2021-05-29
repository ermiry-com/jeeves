#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/types/string.h>

#include <cerver/collections/dlist.h>
#include <cerver/collections/pool.h>

#include <cerver/handler.h>

#include <cerver/http/http.h>
#include <cerver/http/response.h>
#include <cerver/http/json/json.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include <cmongo/crud.h>
#include <cmongo/select.h>

#include "jeeves.h"

#include "controllers/roles.h"
#include "controllers/users.h"

#include "models/user.h"

static Pool *users_pool = NULL;

const bson_t *user_login_query_opts = NULL;
static CMongoSelect *user_login_select = NULL;

const bson_t *user_transactions_query_opts = NULL;
static CMongoSelect *user_transactions_select = NULL;

const bson_t *user_categories_query_opts = NULL;
static CMongoSelect *user_categories_select = NULL;

const bson_t *user_places_query_opts = NULL;
static CMongoSelect *user_places_select = NULL;

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

	user_transactions_select = cmongo_select_new ();
	(void) cmongo_select_insert_field (user_transactions_select, "transCount");

	user_transactions_query_opts = mongo_find_generate_opts (user_transactions_select);

	user_categories_select = cmongo_select_new ();
	(void) cmongo_select_insert_field (user_categories_select, "categoriesCount");

	user_categories_query_opts = mongo_find_generate_opts (user_categories_select);

	user_places_select = cmongo_select_new ();
	(void) cmongo_select_insert_field (user_places_select, "placesCount");

	user_places_query_opts = mongo_find_generate_opts (user_places_select);

	if (
		user_login_query_opts
		&& user_transactions_query_opts
		&& user_categories_query_opts
		&& user_places_query_opts
	) retval = 0;

	return retval;

}

static unsigned int jeeves_users_init_responses (void) {

	unsigned int retval = 1;

	users_works = http_response_json_key_value (
		HTTP_STATUS_OK, "msg", "Users works!"
	);

	missing_user_values = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Missing user values!"
	);

	wrong_password = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Password is incorrect!"
	);

	user_not_found = http_response_json_key_value (
		HTTP_STATUS_NOT_FOUND, "error", "User not found!"
	);

	repeated_email = http_response_json_key_value (
		HTTP_STATUS_BAD_REQUEST, "error", "Email was already registered!"
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

	cmongo_select_delete (user_transactions_select);
	bson_destroy ((bson_t *) user_transactions_query_opts);

	cmongo_select_delete (user_categories_select);
	bson_destroy ((bson_t *) user_categories_query_opts);

	cmongo_select_delete (user_places_select);
	bson_destroy ((bson_t *) user_places_query_opts);

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
		bson_oid_to_string (&user->oid, user->id);

		(void) strncpy (user->name, name, USER_NAME_SIZE - 1);
		(void) strncpy (user->username, username, USER_USERNAME_SIZE - 1);
		(void) strncpy (user->email, email, USER_EMAIL_SIZE - 1);
		(void) strncpy (user->password, password, USER_PASSWORD_SIZE - 1);

		bson_oid_copy (role_oid, &user->role_oid);
	}

	return user;

}

User *jeeves_user_get (void) {

	return (User *) pool_pop (users_pool);

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
//   "email": "erick.salas@ermiry.com",
//   "iat": 1596532954
//   "id": "5eb2b13f0051f70011e9d3af",
//   "name": "Erick Salas",
//   "role": "god",
//   "username": "erick"
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
			"{s:s, s:i, s:s, s:s, s:s, s:s}",
			"email", &email,
			"iat", &user->iat,
			"id", &id,
			"name", &name,
			"role", &role,
			"username", &username
		)) {
			(void) strncpy (user->email, email, USER_EMAIL_SIZE - 1);
			(void) strncpy (user->id, id, USER_ID_SIZE - 1);
			(void) strncpy (user->name, name, USER_NAME_SIZE - 1);
			(void) strncpy (user->role, role, USER_ROLE_SIZE - 1);
			(void) strncpy (user->username, username, USER_USERNAME_SIZE - 1);

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

unsigned int jeeves_user_generate_token (
	const User *user, char *json_token, size_t *json_len
) {

	unsigned int retval = 1;

	HttpJwt *http_jwt = http_cerver_auth_jwt_new ();
	if (http_jwt) {
		http_cerver_auth_jwt_add_value_int (http_jwt, "iat", time (NULL));
		http_cerver_auth_jwt_add_value (http_jwt, "id", user->id);
		http_cerver_auth_jwt_add_value (http_jwt, "email", user->email);
		http_cerver_auth_jwt_add_value (http_jwt, "name", user->name);
		http_cerver_auth_jwt_add_value (http_jwt, "role", jeeves_role_name_get_by_oid (&user->role_oid));
		http_cerver_auth_jwt_add_value (http_jwt, "username", user->username);

		// generate & send back auth token
		if (!http_cerver_auth_generate_bearer_jwt_json (
			http_cerver, http_jwt
		)) {
			(void) strncpy (json_token, http_jwt->json, HTTP_JWT_TOKEN_SIZE - 1);
			*json_len = strlen (http_jwt->json);
		}

		http_cerver_auth_jwt_delete (http_jwt);
	}

	return retval;

}

static void users_input_parse_json (
	json_t *json_body,
	const char **name,
	const char **username,
	const char **email,
	const char **password,
	const char **confirm
) {

	// get values from json to create a new transaction
	char *string = NULL;
	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (json_body) == JSON_OBJECT) {
		json_object_foreach (json_body, key, value) {
			if (!strcmp (key, "name")) {
				string = (char *) json_string_value (value);
				if (strlen (string)) {
					*name = string;
					#ifdef JEEVES_DEBUG
					(void) printf ("name: \"%s\"\n", *name);
					#endif
				}
			}

			else if (!strcmp (key, "username")) {
				string = (char *) json_string_value (value);
				if (strlen (string)) {
					*username = string;
					#ifdef JEEVES_DEBUG
					(void) printf ("username: \"%s\"\n", *username);
					#endif
				}
			}

			else if (!strcmp (key, "email")) {
				string = (char *) json_string_value (value);
				if (strlen (string)) {
					*email = string;
					#ifdef JEEVES_DEBUG
					(void) printf ("email: \"%s\"\n", *email);
					#endif
				}
			}

			else if (!strcmp (key, "password")) {
				string = (char *) json_string_value (value);
				if (strlen (string)) {
					*password = string;
					#ifdef JEEVES_DEBUG
					(void) printf ("password: \"%s\"\n", *password);
					#endif
				}
			}

			else if (!strcmp (key, "confirm")) {
				string = (char *) json_string_value (value);
				if (strlen (string)) {
					*confirm = string;
					#ifdef JEEVES_DEBUG
					(void) printf ("confirm: \"%s\"\n", *confirm);
					#endif
				}
			}
		}
	}

}

static JeevesUserInput jeeves_user_register_validate_input_internal (
	const char *name,
	const char *username,
	const char *email,
	const char *password,
	const char *confirm
) {

	JeevesUserInput user_input = JEEVES_USER_INPUT_NONE;

	if (!name) user_input |= JEEVES_USER_INPUT_NAME;
	if (!username) user_input |= JEEVES_USER_INPUT_USERNAME;
	if (!email) user_input |= JEEVES_USER_INPUT_EMAIL;
	if (!password) user_input |= JEEVES_USER_INPUT_PASSWORD;
	if (!confirm) user_input |= JEEVES_USER_INPUT_CONFIRM;

	return user_input;

}

static JeevesUserError jeeves_user_register_validate_input (
	JeevesUserInput *input,
	const char *name,
	const char *username,
	const char *email,
	const char *password,
	const char *confirm
) {

	JeevesUserError error = JEEVES_USER_ERROR_NONE;

	*input = jeeves_user_register_validate_input_internal (
		name, username, email, password, confirm
	);

	if (*input == JEEVES_USER_INPUT_NONE) {
		if (strcmp (password, confirm)) {
			*input |= JEEVES_USER_INPUT_MATCH;
			error = JEEVES_USER_ERROR_BAD_REQUEST;
		}
	}

	else {
		error = JEEVES_USER_ERROR_MISSING_VALUES;
	}

	return error;

}

static JeevesUserError jeeves_user_register_parse_json (
	const String *request_body, JeevesUserInput *input,
	User **user
) {

	JeevesUserError error = JEEVES_USER_ERROR_NONE;

	const char *name = NULL;
	const char *username = NULL;
	const char *email = NULL;
	const char *password = NULL;
	const char *confirm = NULL;

	json_error_t json_error =  { 0 };
	json_t *json_body = json_loads (request_body->str, 0, &json_error);
	if (json_body) {
		users_input_parse_json (
			json_body,
			&name,
			&username,
			&email,
			&password,
			&confirm
		);

		error = jeeves_user_register_validate_input (
			input,
			name,
			username,
			email,
			password,
			confirm
		);

		if (error == JEEVES_USER_ERROR_NONE) {
			*user = jeeves_user_create (
				name,
				username,
				email,
				password,
				&common_role->oid
			);
		}

		json_decref (json_body);
	}

	else {
		#ifdef JEEVES_DEBUG
		cerver_log_error (
			"json_loads () - json error on line %d: %s\n", 
			json_error.line, json_error.text
		);
		#endif

		error = JEEVES_USER_ERROR_BAD_REQUEST;
	}

	return error;

}

User *jeeves_user_register (
	const String *request_body, 
	JeevesUserError *error, JeevesUserInput *input
) {

	User *retval = NULL;

	if (request_body) {
		User *user = NULL;
		*error = jeeves_user_register_parse_json (
			request_body, input, &user
		);

		if (*error == JEEVES_USER_ERROR_NONE) {
			if (user) {
				if (!user_insert_one (user)) {
					retval = user;
				}

				else {
					*error = JEEVES_USER_ERROR_SERVER_ERROR;
				}
			}

			else {
				*error = JEEVES_USER_ERROR_SERVER_ERROR;
			}
		}
	}

	else {
		#ifdef JEEVES_DEBUG
		cerver_log_error ("Missing request body to register user!");
		#endif

		*error = JEEVES_USER_ERROR_BAD_REQUEST;
	}

	return retval;

}

static JeevesUserInput jeeves_user_login_validate_input_internal (
	const char *email,
	const char *password
) {

	JeevesUserInput user_input = JEEVES_USER_INPUT_NONE;

	if (!email) user_input |= JEEVES_USER_INPUT_EMAIL;
	if (!password) user_input |= JEEVES_USER_INPUT_PASSWORD;

	return user_input;

}

static JeevesUserError jeeves_user_login_parse_json (
	const String *request_body, JeevesUserInput *input,
	User *user_values
) {

	JeevesUserError error = JEEVES_USER_ERROR_NONE;

	const char *name = NULL;
	const char *username = NULL;
	const char *email = NULL;
	const char *password = NULL;
	const char *confirm = NULL;

	json_error_t json_error =  { 0 };
	json_t *json_body = json_loads (request_body->str, 0, &json_error);
	if (json_body) {
		users_input_parse_json (
			json_body,
			&name,
			&username,
			&email,
			&password,
			&confirm
		);

		*input = jeeves_user_login_validate_input_internal (
			email, password
		);

		if (*input == JEEVES_USER_INPUT_NONE) {
			(void) strncpy (user_values->email, email, USER_EMAIL_SIZE - 1);
			(void) strncpy (user_values->password, password, USER_PASSWORD_SIZE - 1);
		}

		else {
			error = JEEVES_USER_ERROR_MISSING_VALUES;
		}

		json_decref (json_body);
	}

	else {
		#ifdef JEEVES_DEBUG
		cerver_log_error (
			"json_loads () - json error on line %d: %s\n", 
			json_error.line, json_error.text
		);
		#endif

		error = JEEVES_USER_ERROR_BAD_REQUEST;
	}

	return error;

}

User *jeeves_user_login (
	const String *request_body, 
	JeevesUserError *error, JeevesUserInput *input
) {

	User *retval = NULL;

	if (request_body) {
		User user_values = { 0 };
		*error = jeeves_user_login_parse_json (
			request_body, input, &user_values
		);

		if (*error == JEEVES_USER_ERROR_NONE) {
			User *user = jeeves_user_get_by_email (user_values.email);
			if (user) {
				if (!strcmp (user->password, user_values.password)) {
					#ifdef JEEVES_DEBUG
					cerver_log_success ("User %s login -> success", user->id);
					#endif

					retval = user;
				}

				else {
					#ifdef JEEVES_DEBUG
					cerver_log_error ("User %s login -> wrong password", user->id);
					#endif

					*error = JEEVES_USER_ERROR_WRONG_PSWD;

					jeeves_user_delete (user);
				}
			}

			else {
				#ifdef JEEVES_DEBUG
				cerver_log_error ("Failed to get %s user!", user_values.email);
				#endif

				*error = JEEVES_USER_ERROR_NOT_FOUND;
			}
		}
	}

	else {
		#ifdef JEEVES_DEBUG
		cerver_log_error ("Missing request body to login user!");
		#endif

		*error = JEEVES_USER_ERROR_BAD_REQUEST;
	}

	return retval;

}

void jeeves_user_delete (void *user_ptr) {

	if (user_ptr) {
		(void) memset (user_ptr, 0, sizeof (User));
		(void) pool_push (users_pool, user_ptr);
	}

}

#include <stdlib.h>
#include <string.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/utils/log.h>

#include <cmongo/collections.h>
#include <cmongo/crud.h>
#include <cmongo/model.h>

#include "models/user.h"

#define USERS_COLL_NAME         				"users"

static CMongoModel *users_model = NULL;

static void user_doc_parse (
	void *user_ptr, const bson_t *user_doc
);

unsigned int users_model_init (void) {

	unsigned int retval = 1;

	users_model = cmongo_model_create (USERS_COLL_NAME);
	if (users_model) {
		cmongo_model_set_parser (users_model, user_doc_parse);

		retval = 0;
	}

	return retval;

}

void users_model_end (void) {

	cmongo_model_delete (users_model);

}

void *user_new (void) {

	User *user = (User *) malloc (sizeof (User));
	if (user) {
		(void) memset (user, 0, sizeof (User));
	}

	return user;

}

void user_delete (void *user_ptr) {

	if (user_ptr) free (user_ptr);

}

void user_print (User *user) {

	if (user) {
		(void) printf ("email: %s\n", user->email);
		(void) printf ("iat: %ld\n", user->iat);
		(void) printf ("id: %s\n", user->id);
		(void) printf ("name: %s\n", user->name);
		(void) printf ("role: %s\n", user->role);
		(void) printf ("username: %s\n", user->username);
	}

}

// parses a bson doc into a user model
static void user_doc_parse (
	void *user_ptr, const bson_t *user_doc
) {

	User *user = (User *) user_ptr;

	bson_iter_t iter = { 0 };
	if (bson_iter_init (&iter, user_doc)) {
		char *key = NULL;
		bson_value_t *value = NULL;
		while (bson_iter_next (&iter)) {
			key = (char *) bson_iter_key (&iter);
			value = (bson_value_t *) bson_iter_value (&iter);

			if (!strcmp (key, "_id")) {
				bson_oid_copy (&value->value.v_oid, &user->oid);
				bson_oid_to_string (&user->oid, user->id);
			}

			else if (!strcmp (key, "role")) {
				bson_oid_copy (&value->value.v_oid, &user->role_oid);
			}

			else if (!strcmp (key, "name") && value->value.v_utf8.str) {
				(void) strncpy (
					user->name,
					value->value.v_utf8.str,
					USER_NAME_SIZE - 1
				);
			}

			else if (!strcmp (key, "email") && value->value.v_utf8.str) {
				(void) strncpy (
					user->email,
					value->value.v_utf8.str,
					USER_EMAIL_SIZE - 1
				);
			}

			else if (!strcmp (key, "username") && value->value.v_utf8.str) {
				(void) strncpy (
					user->username,
					value->value.v_utf8.str,
					USER_USERNAME_SIZE - 1
				);
			}

			else if (!strcmp (key, "password") && value->value.v_utf8.str) {
				(void) strncpy (
					user->password,
					value->value.v_utf8.str,
					USER_PASSWORD_SIZE - 1
				);
			}
		}
	}

}

bson_t *user_query_id (const char *id) {

	bson_t *query = NULL;

	if (id) {
		query = bson_new ();
		if (query) {
			bson_oid_t oid = { 0 };
			bson_oid_init_from_string (&oid, id);
			(void) bson_append_oid (query, "_id", -1, &oid);
		}
	}

	return query;

}

bson_t *user_query_email (const char *email) {

	bson_t *query = NULL;

	if (email) {
		query = bson_new ();
		if (query) {
			(void) bson_append_utf8 (query, "email", -1, email, -1);
		}
	}

	return query;

}

u8 user_check_by_email (const char *email) {

	return mongo_check (users_model, user_query_email (email));

}

u8 user_get_by_id (
	User *user, const char *id, const bson_t *query_opts
) {

	u8 retval = 1;

	if (user && id) {
		bson_oid_t oid = { 0 };
		bson_oid_init_from_string (&oid, id);

		bson_t *user_query = bson_new ();
		if (user_query) {
			(void) bson_append_oid (user_query, "_id", -1, &oid);
			retval = mongo_find_one_with_opts (
				users_model,
				user_query, query_opts,
				user
			);
		}
	}

	return retval;

}

// gets a user from the db by its email
u8 user_get_by_email (
	User *user, const char *email, const bson_t *query_opts
) {

	u8 retval = 1;

	if (user && email) {
		bson_t *user_query = bson_new ();
		if (user_query) {
			(void) bson_append_utf8 (user_query, "email", -1, email, -1);
			retval = mongo_find_one_with_opts (
				users_model,
				user_query, query_opts,
				user
			);
		}
	}

	return retval;

}

// gets a user from the db by its username
u8 user_get_by_username (
	User *user, const String *username, const bson_t *query_opts
) {

	u8 retval = 1;

	if (user && username) {
		bson_t *user_query = bson_new ();
		if (user_query) {
			(void) bson_append_utf8 (user_query, "username", -1, username->str, username->len);
			retval = mongo_find_one_with_opts (
				users_model,
				user_query, query_opts,
				user
			);
		}
	}

	return retval;

}

bson_t *user_bson_create (const User *user) {

	bson_t *doc = NULL;

	if (user) {
		doc = bson_new ();
		if (doc) {
			(void) bson_append_oid (doc, "_id", -1, &user->oid);

			if (user->name) (void) bson_append_utf8 (doc, "name", -1, user->name, -1);
			if (user->username) (void) bson_append_utf8 (doc, "username", -1, user->username, -1);
			if (user->email) (void) bson_append_utf8 (doc, "email", -1, user->email, -1);
			if (user->password) (void) bson_append_utf8 (doc, "password", -1, user->password, -1);

			(void) bson_append_oid (doc, "role", -1, &user->role_oid);
		}
	}

	return doc;

}

unsigned int user_insert_one (const User *user) {

	return mongo_insert_one (
		users_model,
		user_bson_create (user)
	);

}
#include <stdlib.h>
#include <string.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/collections/dlist.h>

#include <cerver/utils/log.h>

#include "mongo.h"

#include "models/user.h"

#define USERS_COLL_NAME         				"users"

mongoc_collection_t *users_collection = NULL;

// opens handle to user collection
unsigned int users_collection_get (void) {

	unsigned int retval = 1;

	users_collection = mongo_collection_get (USERS_COLL_NAME);
	if (users_collection) {
		cerver_log_debug ("Opened handle to users collection!");
		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get handle to users collection!");
	}

	return retval;

}

void users_collection_close (void) {

	if (users_collection) mongoc_collection_destroy (users_collection);

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
static void user_doc_parse (User *user, const bson_t *user_doc) {

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

			else if (!strcmp (key, "name") && value->value.v_utf8.str) 
				(void) strncpy (user->name, value->value.v_utf8.str, USER_NAME_LEN);

			else if (!strcmp (key, "email") && value->value.v_utf8.str) 
				(void) strncpy (user->email, value->value.v_utf8.str, USER_EMAIL_LEN);

			else if (!strcmp (key, "username") && value->value.v_utf8.str) 
				(void) strncpy (user->username, value->value.v_utf8.str, USER_USERNAME_LEN);

			else if (!strcmp (key, "password") && value->value.v_utf8.str)
				(void) strncpy (user->password, value->value.v_utf8.str, USER_PASSWORD_LEN);

			else if (!strcmp (key, "transCount")) {
				user->trans_count = value->value.v_int32;
			}

			else if (!strcmp (key, "categoriesCount")) {
				user->categories_count = value->value.v_int32;
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

static const bson_t *user_find_by_oid (
	const bson_oid_t *oid, const bson_t *query_opts
) {

	const bson_t *retval = NULL;

	bson_t *user_query = bson_new ();
	if (user_query) {
		(void) bson_append_oid (user_query, "_id", -1, oid);
		retval = mongo_find_one_with_opts (users_collection, user_query, query_opts);
	}

	return retval;    

}

u8 user_get_by_id (
	User *user, const char *id, const bson_t *query_opts
) {

	u8 retval = 1;

	if (user && id) {
		bson_oid_t oid = { 0 };
		bson_oid_init_from_string (&oid, id);

		const bson_t *user_doc = user_find_by_oid (&oid, query_opts);
		if (user_doc) {
			user_doc_parse (user, user_doc);
			bson_destroy ((bson_t *) user_doc);

			retval = 0;
		}
	}

	return retval;

}

// get a user doc from the db by email
static const bson_t *user_find_by_email (
	const char *email, const bson_t *query_opts
) {

	const bson_t *retval = NULL;

	bson_t *user_query = bson_new ();
	if (user_query) {
		(void) bson_append_utf8 (user_query, "email", -1, email, -1);
		retval = mongo_find_one_with_opts (users_collection, user_query, query_opts);
	}

	return retval;    

}

// gets a user from the db by its email
u8 user_get_by_email (
	User *user, const char *email, const bson_t *query_opts
) {

	u8 retval = 1;

	if (email) {
		const bson_t *user_doc = user_find_by_email (email, query_opts);
		if (user_doc) {
			user_doc_parse (user, user_doc);
			bson_destroy ((bson_t *) user_doc);

			retval = 0;
		}
	}

	return retval;

}

// get a user doc from the db by username
static const bson_t *user_find_by_username (
	const char *username, const bson_t *query_opts
) {

	const bson_t *retval = NULL;

	bson_t *user_query = bson_new ();
	if (user_query) {
		(void) bson_append_utf8 (user_query, "username", -1, username, -1);
		retval = mongo_find_one_with_opts (users_collection, user_query, query_opts);
	}

	return retval;    

}

// gets a user from the db by its username
u8 user_get_by_username (
	User *user, const char *username, const bson_t *query_opts
) {

	u8 retval = 1;

	if (username) {
		const bson_t *user_doc = user_find_by_username (username, query_opts);
		if (user_doc) {
			user_doc_parse (user, user_doc);
			bson_destroy ((bson_t *) user_doc);

			retval = 0;
		}
	}

	return retval;

}

bson_t *user_bson_create (User *user) {

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
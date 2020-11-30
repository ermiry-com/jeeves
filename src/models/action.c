#include <stdlib.h>

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include "models/action.h"

#include "mongo.h"

#define ACTIONS_COLL_NAME  				"actions"

mongoc_collection_t *actions_collection = NULL;

unsigned int actions_collection_get (void) {

	unsigned int retval = 1;

	actions_collection = mongo_collection_get (ACTIONS_COLL_NAME);
	if (actions_collection) {
		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get handle to actions collection!");
	}

	return retval;

}

void actions_collection_close (void) {

	if (actions_collection) mongoc_collection_destroy (actions_collection);

}

RoleAction *action_new (void) {

	RoleAction *action = (RoleAction *) malloc (sizeof (RoleAction));
	if (action) {
		memset (&action->oid, 0, sizeof (bson_oid_t));

		action->name = NULL;
		action->description = NULL;
	}

	return action;

}

void action_delete (void *action_ptr) {

	if (action_ptr) {
		RoleAction *action = (RoleAction *) action_ptr;

		str_delete (action->name);
		str_delete (action->description);

		free (action_ptr);
	}

}

RoleAction *action_create (const char *name, const char *description) {

	RoleAction *action =action_new ();
	if (action) {
		action->name = name ? str_new (name): NULL;
		action->description = description ? str_new (description): NULL;
	}

	return action;

}

void action_print (RoleAction *action) {

	if (action) {
		printf ("Name: %s\n", action->name->str);
		printf ("Description: %s\n", action->description->str);
	}

}

// creates a action bson with all action parameters
bson_t *action_bson_create (RoleAction *action) {

	bson_t *doc = NULL;

	if (action) {
		doc = bson_new ();
		if (doc) {
			bson_oid_init (&action->oid, NULL);
			bson_append_oid (doc, "_id", -1, &action->oid);

			bson_append_utf8 (doc, "name", -1, action->name->str, action->name->len);
			bson_append_utf8 (doc, "description", -1, action->description->str, action->description->len);
		}
	}

	return doc;

}

RoleAction *action_doc_parse (const bson_t *action_doc) {

	RoleAction *action = NULL;

	if (action_doc) {
		action = action_new ();

		bson_iter_t iter = { 0 };
		if (bson_iter_init (&iter, action_doc)) {
			while (bson_iter_next (&iter)) {
				const char *key = bson_iter_key (&iter);
				const bson_value_t *value = bson_iter_value (&iter);

				if (!strcmp (key, "_id")) {
					bson_oid_copy (&value->value.v_oid, &action->oid);
					// const bson_oid_t *oid = bson_iter_oid (&iter);
					// memcpy (&action->oid, oid, sizeof (bson_oid_t));
				}

				else if (!strcmp (key, "name") && value->value.v_utf8.str) 
					action->name = str_new (value->value.v_utf8.str);

				else if (!strcmp (key, "description") && value->value.v_utf8.str) 
					action->description = str_new (value->value.v_utf8.str);

				// else {
				// 	log_msg (stdout, LOG_TYPE_WARNING, LOG_TYPE_NONE, 
				// 		c_string_create ("Got unknown key %s when parsing action doc.", key));
				// } 
			}
		}
	}

	return action;

}

// get an action doc from the db by name
static const bson_t *action_find_by_name (const String *name) {

	const bson_t *retval = NULL;

	if (name) {
		bson_t *action_query = bson_new ();
		if (action_query) {
			bson_append_utf8 (action_query, "name", -1, name->str, name->len);

			retval = mongo_find_one (actions_collection, action_query, NULL);
		}
	}

	return retval;    

}

// gets an action form the db by its name
RoleAction *action_get_by_name (const String *name) {

	RoleAction *action = NULL;

	if (name) {
		const bson_t *action_doc = action_find_by_name (name);
		if (action_doc) {
			action = action_doc_parse (action_doc);
			bson_destroy ((bson_t *) action_doc);
		}
	}

	return action;

}

bson_t *action_bson_create_name_query (const String *name) {

	bson_t *action_query = NULL;

	if (name) {
		action_query = bson_new ();
		if (action_query) {
			bson_append_utf8 (action_query, "name", -1, name->str, name->len);
		}
	}

	return action_query;

}

bson_t *action_bson_create_update (const String *name, const String *description) {

    bson_t *doc = NULL;

	if (name && description) {
		doc = bson_new ();
		if (doc) {
			bson_t set_doc = { 0 };
			bson_append_document_begin (doc, "$set", -1, &set_doc);

			bson_append_utf8 (&set_doc, "name", -1, name->str, name->len);
			bson_append_utf8 (&set_doc, "description", -1, description->str, description->len);
			
			bson_append_document_end (doc, &set_doc);
		} 
	}

    return doc;

}
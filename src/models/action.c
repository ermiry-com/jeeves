#include <stdlib.h>
#include <stdio.h>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <cmongo/collections.h>
#include <cmongo/crud.h>
#include <cmongo/model.h>

#include "models/action.h"

#define ACTIONS_COLL_NAME  				"actions"

static CMongoModel *actions_model = NULL;

void action_doc_parse (
	void *action_ptr, const bson_t *action_doc
);

unsigned int actions_model_init (void) {

	unsigned int retval = 1;

	actions_model = cmongo_model_create (ACTIONS_COLL_NAME);
	if (actions_model) {
		cmongo_model_set_parser (actions_model, action_doc_parse);

		retval = 0;
	}

	return retval;

}

void actions_model_end (void) {

	cmongo_model_delete (actions_model);

}

RoleAction *action_new (void) {

	RoleAction *action = (RoleAction *) malloc (sizeof (RoleAction));
	if (action) {
		(void) memset (action, 0, sizeof (RoleAction));
	}

	return action;

}

void action_delete (void *action_ptr) {

	if (action_ptr) free (action_ptr);

}

RoleAction *action_create (
	const char *name, const char *description
) {

	RoleAction *action = action_new ();
	if (action) {
		if (name) {
			(void) strncpy (
				action->name, name, ACTION_NAME_LEN - 1
			);
		}

		if (description) {
			(void) strncpy (
				action->description, description, ACTION_DESCRIPTION_LEN - 1
			);
		}
	}

	return action;

}

void action_print (RoleAction *action) {

	if (action) {
		(void) printf ("Name: %s\n", action->name);
		(void) printf ("Description: %s\n", action->description);
	}

}

// creates a action bson with all action parameters
bson_t *action_bson_create (RoleAction *action) {

	bson_t *doc = NULL;

	if (action) {
		doc = bson_new ();
		if (doc) {
			bson_oid_init (&action->oid, NULL);
			(void) bson_append_oid (doc, "_id", -1, &action->oid);

			(void) bson_append_utf8 (
				doc, "name", -1, action->name, -1
			);

			(void) bson_append_utf8 (
				doc, "description", -1, action->description, -1
			);
		}
	}

	return doc;

}

void action_doc_parse (
	void *action_ptr, const bson_t *action_doc
) {

	RoleAction *action = (RoleAction *) action_ptr;

	bson_iter_t iter = { 0 };
	if (bson_iter_init (&iter, action_doc)) {
		while (bson_iter_next (&iter)) {
			const char *key = bson_iter_key (&iter);
			const bson_value_t *value = bson_iter_value (&iter);

			if (!strcmp (key, "_id")) {
				bson_oid_copy (&value->value.v_oid, &action->oid);
			}

			else if (!strcmp (key, "name") && value->value.v_utf8.str) {
				(void) strncpy (
					action->name,
					value->value.v_utf8.str,
					ACTION_NAME_LEN - 1
				);
			}

			else if (!strcmp (key, "description") && value->value.v_utf8.str) {
				(void) strncpy (
					action->description,
					value->value.v_utf8.str,
					ACTION_DESCRIPTION_LEN - 1
				);
			}
		}
	}

}

// gets an action form the db by its name
RoleAction *action_get_by_name (const char *name) {

	RoleAction *action = NULL;

	if (name) {
		action = action_new ();

		bson_t *action_query = bson_new ();
		if (action_query) {
			(void) bson_append_utf8 (action_query, "name", -1, name, -1);
			if (mongo_find_one (
				actions_model,
				action_query, NULL,
				action
			)) {
				action_delete (action);
				action = NULL;
			}
		}
	}

	return action;

}

bson_t *action_bson_create_name_query (const char *name) {

	bson_t *action_query = NULL;

	if (name) {
		action_query = bson_new ();
		if (action_query) {
			(void) bson_append_utf8 (action_query, "name", -1, name, -1);
		}
	}

	return action_query;

}

bson_t *action_bson_create_update (
	const char *name, const char *description
) {

	bson_t *doc = NULL;

	if (name && description) {
		doc = bson_new ();
		if (doc) {
			bson_t set_doc = BSON_INITIALIZER;
			(void) bson_append_document_begin (doc, "$set", -1, &set_doc);

			(void) bson_append_utf8 (&set_doc, "name", -1, name, -1);
			(void) bson_append_utf8 (&set_doc, "description", -1, description, -1);

			(void) bson_append_document_end (doc, &set_doc);
		}
	}

	return doc;

}
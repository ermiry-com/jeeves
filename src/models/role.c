#include <stdlib.h>

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include "mongo.h"

#include "models/role.h"

#define ROLES_COLL_NAME  				"roles"

mongoc_collection_t *roles_collection = NULL;

unsigned int roles_collection_get (void) {

	unsigned int retval = 1;

	roles_collection = mongo_collection_get (ROLES_COLL_NAME);
	if (roles_collection) {
		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get handle to roles collection!");
	}

	return retval;

}

void roles_collection_close (void) {

	if (roles_collection) mongoc_collection_destroy (roles_collection);

}


Role *role_new (void) {

	Role *role = (Role *) malloc (sizeof (Role));
	if (role) {
		memset (&role->oid, 0, sizeof (bson_oid_t));

		role->name = NULL;
		role->actions = NULL;
	}

	return role;

}

void role_delete (void *role_ptr) {

	if (role_ptr) {
		Role *role = (Role *) role_ptr;

		str_delete (role->name);
		dlist_delete (role->actions);

		free (role);
	}

}

Role *role_create (const char *name) {

	Role *role = role_new ();
	if (role) {
		role->name = name ? str_new (name) : NULL;
		role->actions = dlist_init (str_delete, str_comparator);
	}

	return role;

}

void role_print (Role *role) {

	if (role) {
		printf ("Name: %s\n", role->name->str);
		if (role->actions) {
			if (role->actions->size > 0) {
				printf ("Actions: \n");
				for (ListElement *le = dlist_start (role->actions); le; le = le->next) {
					printf ("\t%s\n", ((String *) le->data)->str);
				}
			}

			else {
				printf ("No actions!\n");
			}
		}

		else {
			printf ("No actions!\n");
		}
	}

}

// creates a role bson with all role parameters
bson_t *role_bson_create (Role *role) {

	bson_t *doc = NULL;

	if (role) {
		doc = bson_new ();
		if (doc) {
			bson_oid_init (&role->oid, NULL);
			bson_append_oid (doc, "_id", -1, &role->oid);

			bson_append_utf8 (doc, "name", -1, role->name->str, role->name->len);
		
			bson_t actions_array = { 0 };
			BSON_APPEND_ARRAY_BEGIN (doc, "actions", &actions_array);
			if (role->actions->size > 0) {
				char buf[16] = { 0 };
				const char *key = NULL;
				size_t keylen = 0;

				memset (buf, 0, sizeof (buf));

				String *action = NULL;
				unsigned int i = 0;
				for (ListElement *le = dlist_start (role->actions); le; le = le->next) {
					action = (String *) le->data;

					keylen = bson_uint32_to_string (i, &key, buf, sizeof buf);
					bson_append_utf8 (&actions_array, key, (int) keylen, action->str, action->len);

					i++;
				}
			}
			bson_append_array_end (doc, &actions_array);
		}
	}

	return doc;

}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static void role_doc_parse_actions (Role *role, bson_iter_t *iter) {

	if (role && iter) {
		const u8 *data = NULL;
		u32 len = 0;
		bson_iter_array (iter, &len, &data);

		bson_t *actions_array = bson_new_from_data (data, len);
		if (actions_array) {
			bson_iter_t array_iter;
			if (bson_iter_init (&array_iter, actions_array)) {
				while (bson_iter_next (&array_iter)) {
					// const char *key = bson_iter_key (&array_iter);
					const bson_value_t *value = bson_iter_value (&array_iter);

					dlist_insert_after (
						role->actions,
						dlist_end (role->actions),
						str_new (value->value.v_utf8.str)
					);
				}
			}

			bson_destroy (actions_array);
		}
	}

}

#pragma GCC diagnostic pop

Role *role_doc_parse (const bson_t *role_doc) {

	Role *role = NULL;

	if (role_doc) {
		role = role_create (NULL);

		bson_iter_t iter = { 0 };
		if (bson_iter_init (&iter, role_doc)) {
			while (bson_iter_next (&iter)) {
				const char *key = bson_iter_key (&iter);
				const bson_value_t *value = bson_iter_value (&iter);

				if (!strcmp (key, "_id")) {
					bson_oid_copy (&value->value.v_oid, &role->oid);
					// const bson_oid_t *oid = bson_iter_oid (&iter);
					// memcpy (&role->oid, oid, sizeof (bson_oid_t));
				}

				else if (!strcmp (key, "name") && value->value.v_utf8.str) 
					role->name = str_new (value->value.v_utf8.str);

				else if (!strcmp (key, "actions")) {
					// role_doc_parse_actions (role, &iter);
				}
			}
		}
	}

	return role;

}

static const bson_t *role_find_by_oid (const bson_oid_t *oid, bool actions) {

    const bson_t *retval = NULL;

    if (oid) {
        bson_t *role_query = bson_new ();
        if (role_query) {
            bson_append_oid (role_query, "_id", -1, oid);

			DoubleList *select = NULL;
			if (!actions) {
				select = dlist_init (str_delete, str_comparator);
				dlist_insert_after (select, dlist_end (select), str_new ("name"));
			}

            retval = mongo_find_one (roles_collection, role_query, select);

			dlist_delete (select);
        }
    }

    return retval;

}

// gets a role by its oid from the db
// option to select if you want actions or not
Role *role_get_by_oid (const bson_oid_t *oid, bool actions) {

    Role *role = NULL;

    if (oid) {
        const bson_t *role_doc = role_find_by_oid (oid, actions);
        if (role_doc) {
            role = role_doc_parse (role_doc);
            bson_destroy ((bson_t *) role_doc);
        }
    }

    return role;

}

// get a role doc from the db by name
static const bson_t *role_find_by_name (const String *name, bool actions) {

	const bson_t *retval = NULL;

	if (name) {
		bson_t *role_query = bson_new ();
		if (role_query) {
			bson_append_utf8 (role_query, "name", -1, name->str, name->len);

			DoubleList *select = NULL;
			if (!actions) {
				select = dlist_init (str_delete, str_comparator);
				dlist_insert_after (select, dlist_end (select), str_new ("name"));
			}

			retval = mongo_find_one (roles_collection, role_query, select);

			dlist_delete (select);
		}
	}

	return retval;    

}

// gets a role form the db by its name
// option to select if you want actions or not
Role *role_get_by_name (const String *name, bool actions) {

	Role *role = NULL;

	if (name) {
		const bson_t *role_doc = role_find_by_name (name, actions);
		if (role_doc) {
			role = role_doc_parse (role_doc);
			bson_destroy ((bson_t *) role_doc);
		}
	}

	return role;

}

bson_t *role_bson_create_oid_query (const bson_oid_t *oid) {

	bson_t *role_query = NULL;

	if (oid) {
		role_query = bson_new ();
		if (role_query) {
			bson_append_oid (role_query, "_id", -1, oid);
		}
	}

	return role_query;

}

bson_t *role_bson_create_name_query (const String *name) {

	bson_t *role_query = NULL;

	if (name) {
		role_query = bson_new ();
		if (role_query) {
			bson_append_utf8 (role_query, "name", -1, name->str, name->len);
		}
	}

	return role_query;

}

bson_t *role_bson_create_update (Role *role) {

	bson_t *doc = NULL;

	if (role) {
		doc = bson_new ();
		if (doc) {
			bson_t set_doc = { 0 };
			bson_append_document_begin (doc, "$set", -1, &set_doc);

			bson_append_utf8 (&set_doc, "name", -1, role->name->str, role->name->len);

			bson_t actions_array = { 0 };
			BSON_APPEND_ARRAY_BEGIN (&set_doc, "actions", &actions_array);
			if (role->actions->size > 0) {
				char buf[16] = { 0 };
				const char *key = NULL;
				size_t keylen = 0;

				memset (buf, 0, sizeof (buf));

				String *action = NULL;
				unsigned int i = 0;
				for (ListElement *le = dlist_start (role->actions); le; le = le->next) {
					action = (String *) le->data;

					keylen = bson_uint32_to_string (i, &key, buf, sizeof buf);
					bson_append_utf8 (&actions_array, key, (int) keylen, action->str, action->len);

					i++;
				}
			}
			bson_append_array_end (&set_doc, &actions_array);
			
			bson_append_document_end (doc, &set_doc);
		} 
	}

	return doc;

}
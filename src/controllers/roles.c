#include <stdlib.h>
#include <stdio.h>

#include <cerver/types/types.h>
#include <cerver/types/string.h>

#include <cerver/collections/dlist.h>

#include <cerver/utils/log.h>

#include "mongo.h"

#include "models/role.h"

static DoubleList *roles = NULL;
const Role *common_role = NULL;

unsigned int jeeves_roles_init (void) {

	unsigned int retval = 1;

	roles = dlist_init (role_delete, NULL);

	u64 n_docs = 0;
	mongoc_cursor_t *roles_cursor = mongo_find_all_cursor (roles_collection, bson_new (), NULL, &n_docs);
	if (roles_cursor) {
		// Role *role = NULL;
		const bson_t *role_doc = NULL;
		while (mongoc_cursor_next (roles_cursor, &role_doc)) {
			dlist_insert_after (
				roles,
				dlist_end (roles),
				role_doc_parse (role_doc)
			);

			bson_destroy ((bson_t *) role_doc);
		}

		String *role_query = str_new ("common");
		common_role = role_get_by_name (role_query, false);
		str_delete (role_query);

		mongoc_cursor_destroy (roles_cursor);

		retval = 0;
	}

	else {
		cerver_log_error ("Failed to get roles cursor!");
	}

	return retval;

}

void jeeves_roles_end (void) {

	role_delete ((Role *) common_role);

	dlist_delete (roles);

}

const String *jeeves_roles_get_by_oid (const bson_oid_t *role_oid) {

	const String *retval = NULL;

	if (role_oid) {
		for (ListElement *le = dlist_start (roles); le; le = le->next) {
			if (!bson_oid_compare (&((Role *) le->data)->oid, role_oid)) {
				retval = ((Role *) le->data)->name;
				break;
			}
		}
	}

	return retval;

}
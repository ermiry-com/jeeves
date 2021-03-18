#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmongo/crud.h>
#include <cmongo/select.h>

#include <cerver/collections/dlist.h>

#include <cerver/utils/log.h>

#include "models/role.h"

static DoubleList *roles = NULL;

const Role *common_role = NULL;

const Role *jeeves_role_get_by_name (
	const char *role_name
);

static unsigned int jeeves_roles_init_get_roles (void) {

	unsigned int retval = 1;

	CMongoSelect *select = cmongo_select_new ();
	cmongo_select_insert_field (select, "name");

	uint64_t n_docs = 0;
	mongoc_cursor_t *roles_cursor = role_find_all (select, &n_docs);
	if (roles_cursor) {
		Role *role = NULL;
		const bson_t *role_doc = NULL;
		unsigned int errors = 0;
		while (mongoc_cursor_next (roles_cursor, &role_doc)) {
			role = role_new ();
			if (role) {
				role_doc_parse (role, role_doc);

				errors |= dlist_insert_after (
					roles,
					dlist_end (roles),
					role
				);
			}

			else {
				errors |= 1;
			}
		}

		mongoc_cursor_destroy (roles_cursor);

		retval = errors;
	}

	else {
		(void) fprintf (stderr, "Failed to get roles cursor!");
	}

	cmongo_select_delete (select);

	return retval;

}

unsigned int jeeves_roles_init (void) {

	unsigned int retval = 1;

	roles = dlist_init (role_delete, NULL);
	if (!jeeves_roles_init_get_roles ()) {
		common_role = jeeves_role_get_by_name ("common");
		if (common_role) {
			retval = 0;
		}
	}

	return retval;

}

void jeeves_roles_end (void) {

	dlist_delete (roles);

}

const Role *jeeves_role_get_by_oid (const bson_oid_t *role_oid) {

	const Role *retval = NULL;

	if (role_oid) {
		for (ListElement *le = dlist_start (roles); le; le = le->next) {
			if (!bson_oid_compare (&((Role *) le->data)->oid, role_oid)) {
				retval = ((Role *) le->data);
				break;
			}
		}
	}

	return retval;

}

const Role *jeeves_role_get_by_name (
	const char *role_name
) {

	const Role *retval = NULL;

	if (role_name) {
		for (ListElement *le = dlist_start (roles); le; le = le->next) {
			if (!strcmp (((Role *) le->data)->name, role_name)) {
				retval = ((Role *) le->data);
				break;
			}
		}
	}

	return retval;

}

const char *jeeves_role_name_get_by_oid (
	const bson_oid_t *role_oid
) {

	const char *retval = NULL;

	const Role *role = jeeves_role_get_by_oid (role_oid);
	if (role) {
		retval = role->name;
	}

	return retval;

}
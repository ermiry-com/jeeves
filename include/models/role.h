#ifndef _MODELS_ROLE_H_
#define _MODELS_ROLE_H_

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <cmongo/select.h>

#define ROLE_NAME_LEN			128
#define ROLE_ACTIONS_SIZE		32
#define ROLE_ACTION_LEN			64

extern unsigned int roles_model_init (void);

extern void roles_model_end (void);

struct _Role {

	bson_oid_t oid;

	char name[ROLE_NAME_LEN];

	unsigned int n_actions;
	char actions[ROLE_ACTIONS_SIZE][ROLE_ACTION_LEN];

};

typedef struct _Role Role;

extern Role *role_new (void);

extern void role_delete (void *role_ptr);

extern Role *role_create (
	const char *name
);

extern void role_print (
	Role *role
);

// creates a role bson with all role parameters
extern bson_t *role_bson_create (
	Role *role
);

extern void role_doc_parse (
	void *role_ptr, const bson_t *role_doc
);

extern unsigned int role_get_by_oid (
	Role *role, const bson_oid_t *oid, const bson_t *query_opts
);

extern unsigned int role_get_by_cuc (
	Role *role,
	const char *cuc, const bson_t *query_opts
);

// gets a role form the db by its name
// option to select if you want actions or not
extern Role *role_get_by_name (
	const char *name, bool actions
);

extern bson_t *role_bson_create_oid_query (
	const bson_oid_t *oid
);

extern bson_t *role_bson_create_name_query (
	const char *name
);

extern bson_t *role_bson_create_update (
	Role *role
);

extern mongoc_cursor_t *role_find_all (
	const CMongoSelect *select, uint64_t *n_docs
);

#endif
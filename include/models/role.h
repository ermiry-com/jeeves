#ifndef _MODELS_ROLE_H_
#define _MODELS_ROLE_H_

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include <cerver/types/string.h>

#include <cerver/collections/dlist.h>

extern mongoc_collection_t *roles_collection;

extern unsigned int roles_collection_get (void);

extern void roles_collection_close (void);

struct _Role {

	bson_oid_t oid;

	String *name;
	DoubleList *actions;

};

typedef struct _Role Role;

extern Role *role_new (void);

extern void role_delete (void *role_ptr);

extern Role *role_create (const char *name);

extern void role_print (Role *role);

// creates a role bson with all role parameters
extern bson_t *role_bson_create (Role *role);

extern Role *role_doc_parse (const bson_t *role_doc);

// gets a role by its oid from the db
// option to select if you want actions or not
extern Role *role_get_by_oid (const bson_oid_t *oid, bool actions);

// gets a role form the db by its name
// option to select if you want actions or not
extern Role *role_get_by_name (const String *name, bool actions);

extern bson_t *role_bson_create_oid_query (const bson_oid_t *oid);

extern bson_t *role_bson_create_name_query (const String *name);

extern bson_t *role_bson_create_update (Role *role);

#endif
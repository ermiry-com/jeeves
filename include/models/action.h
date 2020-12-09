#ifndef _MODELS_ACTION_H_
#define _MODELS_ACTION_H_

#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include <cerver/types/string.h>

extern mongoc_collection_t *actions_collection;

extern unsigned int actions_collection_get (void);

extern void actions_collection_close (void);

struct _RoleAction {

	bson_oid_t oid;

	String *name;
	String *description;

};

typedef struct _RoleAction RoleAction;

extern RoleAction *action_new (void);

extern void action_delete (void *action_ptr);

extern RoleAction *action_create (const char *name, const char *description);

extern void action_print (RoleAction *action);

// creates a action bson with all action parameters
extern bson_t *action_bson_create (RoleAction *action);

extern RoleAction *action_doc_parse (const bson_t *action_doc);

// gets an action form the db by its name
extern RoleAction *action_get_by_name (const String *name);

extern bson_t *action_bson_create_name_query (const String *name);

extern bson_t *action_bson_create_update (const String *name, const String *description);

#endif
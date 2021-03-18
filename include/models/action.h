#ifndef _MODELS_ACTION_H_
#define _MODELS_ACTION_H_

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#define ACTION_NAME_LEN				128
#define ACTION_DESCRIPTION_LEN		256

extern unsigned int actions_model_init (void);

extern void actions_model_end (void);

struct _RoleAction {

	bson_oid_t oid;

	char name[ACTION_NAME_LEN];
	char description[ACTION_DESCRIPTION_LEN];

};

typedef struct _RoleAction RoleAction;

extern RoleAction *action_new (void);

extern void action_delete (void *action_ptr);

extern RoleAction *action_create (
	const char *name, const char *description
);

extern void action_print (RoleAction *action);

// creates a action bson with all action parameters
extern bson_t *action_bson_create (
	RoleAction *action
);

extern void action_doc_parse (
	void *action_ptr, const bson_t *action_doc
);

// gets an action form the db by its name
extern RoleAction *action_get_by_name (
	const char *name
);

extern bson_t *action_bson_create_name_query (
	const char *name
);

extern bson_t *action_bson_create_update (
	const char *name, const char *description
);

#endif
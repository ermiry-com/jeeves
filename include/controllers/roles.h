#ifndef _JEEVES_ROLES_H_
#define _JEEVES_ROLES_H_

#include <cerver/types/string.h>

#include <bson/bson.h>

#include "models/role.h"

extern const Role *common_role;

extern unsigned int jeeves_roles_init (void);

extern void jeeves_roles_end (void);

extern const String *jeeves_roles_get_by_oid (
    const bson_oid_t *role_oid
);

#endif
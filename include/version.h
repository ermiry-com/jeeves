#ifndef _JEEVES_VERSION_H_
#define _JEEVES_VERSION_H_

#define JEEVES_VERSION                    	"0.2"
#define JEEVES_VERSION_NAME               	"Version 0.2"
#define JEEVES_VERSION_DATE			    	"08/12/2020"
#define JEEVES_VERSION_TIME			    	"18:36 CST"
#define JEEVES_VERSION_AUTHOR			    "Erick Salas"

// print full jeeves version information
extern void jeeves_version_print_full (void);

// print the version id
extern void jeeves_version_print_version_id (void);

// print the version name
extern void jeeves_version_print_version_name (void);

#endif
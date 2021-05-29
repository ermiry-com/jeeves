#ifndef _JEEVES_SERVICE_H_
#define _JEEVES_SERVICE_H_

struct _HttpResponse;

extern struct _HttpResponse *missing_values;

extern struct _HttpResponse *jeeves_works;
extern struct _HttpResponse *current_version;

extern struct _HttpResponse *catch_all;

extern unsigned int jeeves_service_init (void);

extern void jeeves_service_end (void);

#endif
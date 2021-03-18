#ifndef _JEEVES_RUNTIME_H_
#define _JEEVES_RUNTIME_H_

#define RUNTIME_TYPE_MAP(XX)						\
	XX(0,  NONE,      		Undefined)				\
	XX(1,  DEVELOPMENT,     Development)			\
	XX(2,  TEST,	      	Test)					\
	XX(3,  PRODUCTION,      Production)

typedef enum RuntimeType {

	#define XX(num, name, string) RUNTIME_TYPE_##name = num,
	RUNTIME_TYPE_MAP(XX)
	#undef XX

} RuntimeType;

extern const char *runtime_to_string (RuntimeType type);

extern const RuntimeType runtime_from_string (const char *string);

#endif
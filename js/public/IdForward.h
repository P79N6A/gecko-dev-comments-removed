





#ifndef js_IdForward_h
#define js_IdForward_h

#include <stddef.h>


































#if defined(DEBUG) && !defined(JS_NO_JSVAL_JSID_STRUCT_TYPES)
# define JS_USE_JSID_STRUCT_TYPES
#endif

#ifdef JS_USE_JSID_STRUCT_TYPES
struct jsid;
#else
typedef ptrdiff_t jsid;
#endif

#endif 

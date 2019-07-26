





#include "js/Id.h"

#ifdef JS_USE_JSID_STRUCT_TYPES
const jsid JSID_VOID  = { size_t(JSID_TYPE_VOID) };
const jsid JSID_EMPTY = { size_t(JSID_TYPE_OBJECT) };
#endif


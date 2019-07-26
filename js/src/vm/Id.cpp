





#include "js/Id.h"
#include "js/RootingAPI.h"

#ifdef JS_USE_JSID_STRUCT_TYPES
const jsid JSID_VOID  = { size_t(JSID_TYPE_VOID) };
const jsid JSID_EMPTY = { size_t(JSID_TYPE_OBJECT) };
#endif

static const jsid voidIdValue = JSID_VOID;
static const jsid emptyIdValue = JSID_EMPTY;
const JS::HandleId JSID_VOIDHANDLE = JS::HandleId::fromMarkedLocation(&voidIdValue);
const JS::HandleId JSID_EMPTYHANDLE = JS::HandleId::fromMarkedLocation(&emptyIdValue);


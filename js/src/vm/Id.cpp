





#include "js/Id.h"
#include "js/RootingAPI.h"

const jsid JSID_VOID  = { size_t(JSID_TYPE_VOID) };
const jsid JSID_EMPTY = { size_t(JSID_TYPE_SYMBOL) };

static const jsid voidIdValue = JSID_VOID;
static const jsid emptyIdValue = JSID_EMPTY;
const JS::HandleId JSID_VOIDHANDLE = JS::HandleId::fromMarkedLocation(&voidIdValue);
const JS::HandleId JSID_EMPTYHANDLE = JS::HandleId::fromMarkedLocation(&emptyIdValue);









































#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "nsAString.h"
#include "nsIPrincipal.h"

nsresult
xpc_CreateGlobalObject(JSContext *cx, JSClass *clasp,
                       const nsACString &origin, nsIPrincipal *principal,
                       bool wantXrays, JSObject **global,
                       JSCompartment **compartment);

#endif

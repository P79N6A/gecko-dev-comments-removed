






































#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "jsobj.h"
#include "nsAString.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"

class nsIPrincipal;

nsresult
xpc_CreateGlobalObject(JSContext *cx, JSClass *clasp,
                       nsIPrincipal *principal, nsISupports *ptr,
                       bool wantXrays, JSObject **global,
                       JSCompartment **compartment);

nsresult
xpc_CreateMTGlobalObject(JSContext *cx, JSClass *clasp,
                         nsISupports *ptr, JSObject **global,
                         JSCompartment **compartment);

extern JSBool
XPC_WN_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

#define IS_WRAPPER_CLASS(clazz)                                               \
    (clazz->ext.equality == js::Valueify(XPC_WN_Equality))

inline JSBool
DebugCheckWrapperClass(JSObject* obj)
{
    NS_ASSERTION(IS_WRAPPER_CLASS(obj->getClass()),
                 "Forgot to check if this is a wrapper?");
    return JS_TRUE;
}








#define IS_WN_WRAPPER_OBJECT(obj)                                             \
    (DebugCheckWrapperClass(obj) && obj->getSlot(0).isUndefined())
#define IS_SLIM_WRAPPER_OBJECT(obj)                                           \
    (DebugCheckWrapperClass(obj) && !obj->getSlot(0).isUndefined())




#define IS_WN_WRAPPER(obj)                                                    \
    (IS_WRAPPER_CLASS(obj->getClass()) && IS_WN_WRAPPER_OBJECT(obj))
#define IS_SLIM_WRAPPER(obj)                                                  \
    (IS_WRAPPER_CLASS(obj->getClass()) && IS_SLIM_WRAPPER_OBJECT(obj))

inline JSObject *
xpc_GetGlobalForObject(JSObject *obj)
{
    while(JSObject *parent = obj->getParent())
        obj = parent;
    return obj;
}

inline JSObject*
xpc_GetCachedSlimWrapper(nsWrapperCache *cache, JSObject *scope, jsval *vp)
{
    if (cache) {
        JSObject* wrapper = cache->GetWrapper();
        
        
        if (wrapper &&
            IS_SLIM_WRAPPER_OBJECT(wrapper) &&
            wrapper->getCompartment() == scope->getCompartment()) {
            *vp = OBJECT_TO_JSVAL(wrapper);

            return wrapper;
        }
    }

    return nsnull;
}

#endif








































#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "jsobj.h"
#include "nsAString.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"

nsresult
xpc_CreateGlobalObject(JSContext *cx, JSClass *clasp,
                       const nsACString &origin, nsIPrincipal *principal,
                       JSObject **global, JSCompartment **compartment);

extern JSBool
XPC_WN_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);








#define IS_WRAPPER_CLASS(clazz)                                               \
    (clazz->ext.equality == js::Valueify(XPC_WN_Equality))

extern JSBool
DebugCheckWrapperClass(JSObject* obj);








#define IS_WN_WRAPPER_OBJECT(obj)                                             \
    (DebugCheckWrapperClass(obj) &&                                           \
     obj->getSlot(JSSLOT_START(obj->getClass())).isUndefined())
#define IS_SLIM_WRAPPER_OBJECT(obj)                                           \
    (DebugCheckWrapperClass(obj) &&                                           \
     !obj->getSlot(JSSLOT_START(obj->getClass())).isUndefined())




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
            xpc_GetGlobalForObject(wrapper) == xpc_GetGlobalForObject(scope)) {
            *vp = OBJECT_TO_JSVAL(wrapper);

            return wrapper;
        }
    }

    return nsnull;
}

#endif

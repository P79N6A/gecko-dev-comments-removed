





































#ifndef nsWrapperCacheInline_h___
#define nsWrapperCacheInline_h___

#include "nsWrapperCache.h"
#include "xpcprivate.h"

inline JSObject*
nsWrapperCache::GetWrapperPreserveColor() const
{
  JSObject *obj = GetJSObjectFromBits();
  return !IsProxy() || !obj || js::IsProxy(obj) ? obj : nsnull;
}

inline JSObject*
nsWrapperCache::GetWrapper() const
{
    JSObject* obj = GetWrapperPreserveColor();
    xpc_UnmarkGrayObject(obj);
    return obj;
}

inline void
nsWrapperCache::SetWrapper(JSObject* aWrapper)
{
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    NS_ASSERTION(aWrapper, "Use ClearWrapper!");

    JSObject *obj = GetJSObjectFromBits();
    if (obj && mozilla::dom::binding::isExpandoObject(obj)) {
        NS_ASSERTION(mozilla::dom::binding::instanceIsProxy(aWrapper),
                     "We have an expando but this isn't a DOM proxy?");
        js::SetProxyExtra(aWrapper, mozilla::dom::binding::JSPROXYSLOT_EXPANDO,
                          js::ObjectValue(*obj));
    }

    SetWrapperBits(aWrapper);
}

inline JSObject*
nsWrapperCache::GetExpandoObjectPreserveColor() const
{
    JSObject *obj = GetJSObjectFromBits();
    if (!obj) {
        return NULL;
    }

    if (!IsProxy()) {
        
        
        return NULL;
    }

    
    if (mozilla::dom::binding::instanceIsProxy(obj)) {
        return GetExpandoFromSlot(obj);
    }

    return mozilla::dom::binding::isExpandoObject(obj) ? obj : NULL;
}

inline JSObject*
nsWrapperCache::GetExpandoFromSlot(JSObject *obj)
{
    NS_ASSERTION(mozilla::dom::binding::instanceIsProxy(obj),
                 "Asking for an expando but this isn't a DOM proxy?");
    const js::Value &v = js::GetProxyExtra(obj, mozilla::dom::binding::JSPROXYSLOT_EXPANDO);
    return v.isUndefined() ? NULL : v.toObjectOrNull();
}

inline void
nsWrapperCache::ClearWrapper()
{
    NS_ASSERTION(!PreservingWrapper(), "Clearing a preserved wrapper!");
    JSObject *obj = GetJSObjectFromBits();
    if (!obj) {
        return;
    }

    JSObject *expando;
    if (mozilla::dom::binding::instanceIsProxy(obj)) {
        expando = GetExpandoFromSlot(obj);
    }
    else {
        
        
        expando = NULL;
    }

    SetWrapperBits(expando);
}

inline void
nsWrapperCache::ClearWrapperIfProxy()
{
    if (!IsProxy()) {
        return;
    }

    RemoveExpandoObject();

    SetWrapperBits(NULL);
}

#endif 

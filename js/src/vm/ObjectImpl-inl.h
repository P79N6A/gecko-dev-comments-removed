





#ifndef vm_ObjectImpl_inl_h
#define vm_ObjectImpl_inl_h

#include "vm/ObjectImpl.h"

#include "jscntxt.h"

#include "proxy/Proxy.h"
#include "vm/ProxyObject.h"
#include "vm/TypedArrayObject.h"

namespace js {

 inline bool
ObjectImpl::isExtensible(ExclusiveContext *cx, Handle<ObjectImpl*> obj, bool *extensible)
{
    if (obj->asObjectPtr()->is<ProxyObject>()) {
        if (!cx->shouldBeJSContext())
            return false;
        HandleObject h =
            HandleObject::fromMarkedLocation(reinterpret_cast<JSObject* const*>(obj.address()));
        return Proxy::isExtensible(cx->asJSContext(), h, extensible);
    }

    *extensible = obj->nonProxyIsExtensible();
    return true;
}

inline bool
ClassCanHaveFixedData(const Class *clasp)
{
    
    
    
    
    
    return clasp == &ArrayBufferObject::class_ || IsTypedArrayClass(clasp);
}

inline void *
ObjectImpl::fixedData(size_t nslots) const
{
    JS_ASSERT(ClassCanHaveFixedData(getClass()));
    JS_ASSERT(nslots == numFixedSlots() + (hasPrivate() ? 1 : 0));
    return &fixedSlots()[nslots];
}

} 

#endif 

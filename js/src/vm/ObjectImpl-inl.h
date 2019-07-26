





#ifndef vm_ObjectImpl_inl_h
#define vm_ObjectImpl_inl_h

#include "vm/ObjectImpl.h"

#include "mozilla/Assertions.h"

#include "jsgc.h"
#include "jsproxy.h"

#include "gc/Marking.h"
#include "vm/ProxyObject.h"

#include "gc/Barrier-inl.h"

 inline bool
js::ObjectImpl::isExtensible(ExclusiveContext *cx, js::Handle<ObjectImpl*> obj, bool *extensible)
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

#endif 

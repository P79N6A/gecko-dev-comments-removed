





#ifndef vm_ProxyObject_h
#define vm_ProxyObject_h

#include "js/Proxy.h"
#include "vm/NativeObject.h"

namespace js {



class ProxyObject : public JSObject
{
    
    ProxyDataLayout data;

    void static_asserts() {
        static_assert(sizeof(ProxyObject) == sizeof(JSObject_Slots0),
                      "proxy object size must match GC thing size");
        static_assert(offsetof(ProxyObject, data) == ProxyDataOffset,
                      "proxy object layout must match shadow interface");
    }

  public:
    static ProxyObject *New(JSContext *cx, const BaseProxyHandler *handler, HandleValue priv,
                            TaggedProto proto_, JSObject *parent_,
                            const ProxyOptions &options);

    const Value &private_() {
        return GetProxyPrivate(this);
    }

    void setCrossCompartmentPrivate(const Value &priv);
    void setSameCompartmentPrivate(const Value &priv);

    HeapValue *slotOfPrivate() {
        return reinterpret_cast<HeapValue *>(&GetProxyDataLayout(this)->values->privateSlot);
    }

    JSObject *target() const {
        return const_cast<ProxyObject*>(this)->private_().toObjectOrNull();
    }

    const BaseProxyHandler *handler() const {
        return GetProxyHandler(const_cast<ProxyObject *>(this));
    }

    void setHandler(const BaseProxyHandler *handler) {
        SetProxyHandler(this, handler);
    }

    static size_t offsetOfValues() {
        return offsetof(ProxyObject, data.values);
    }
    static size_t offsetOfHandler() {
        return offsetof(ProxyObject, data.handler);
    }
    static size_t offsetOfExtraSlotInValues(size_t slot) {
        MOZ_ASSERT(slot < PROXY_EXTRA_SLOTS);
        return offsetof(ProxyValueArray, extraSlots) + slot * sizeof(Value);
    }

    const Value &extra(size_t n) const {
        return GetProxyExtra(const_cast<ProxyObject *>(this), n);
    }

    void setExtra(size_t n, const Value &extra) {
        SetProxyExtra(this, n, extra);
    }

  private:
    HeapValue *slotOfExtra(size_t n) {
        MOZ_ASSERT(n < PROXY_EXTRA_SLOTS);
        return reinterpret_cast<HeapValue *>(&GetProxyDataLayout(this)->values->extraSlots[n]);
    }

    static bool isValidProxyClass(const Class *clasp) {
        
        
        

        
        

        
        
        return clasp->isProxy() &&
               (clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS) &&
               clasp->trace == proxy_Trace &&
               !clasp->call && !clasp->construct;
    }

  public:
    static unsigned grayLinkExtraSlot(JSObject *obj);

    void renew(JSContext *cx, const BaseProxyHandler *handler, Value priv);

    static void trace(JSTracer *trc, JSObject *obj);

    void nuke(const BaseProxyHandler *handler);

    static const Class class_;
};

} 







template<>
inline bool
JSObject::is<js::ProxyObject>() const
{
    return js::IsProxy(const_cast<JSObject*>(this));
}

#endif 

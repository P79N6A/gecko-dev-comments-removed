





#ifndef vm_ProxyObject_h
#define vm_ProxyObject_h

#include "jsobj.h"
#include "jsproxy.h"

namespace js {



class ProxyObject : public JSObject
{
    
    
    static const uint32_t PRIVATE_SLOT = PROXY_PRIVATE_SLOT;
    static const uint32_t HANDLER_SLOT = PROXY_HANDLER_SLOT;
    static const uint32_t EXTRA_SLOT   = PROXY_EXTRA_SLOT;

  public:
    static ProxyObject *New(JSContext *cx, BaseProxyHandler *handler, HandleValue priv,
                            TaggedProto proto_, JSObject *parent_,
                            const ProxyOptions &options);

    const Value &private_() {
        return GetReservedSlot(this, PRIVATE_SLOT);
    }

    void initCrossCompartmentPrivate(HandleValue priv);

    HeapSlot *slotOfPrivate() {
        return &getReservedSlotRef(PRIVATE_SLOT);
    }

    JSObject *target() const {
        return const_cast<ProxyObject*>(this)->private_().toObjectOrNull();
    }

    BaseProxyHandler *handler() {
        return static_cast<BaseProxyHandler*>(GetReservedSlot(this, HANDLER_SLOT).toPrivate());
    }

    void initHandler(BaseProxyHandler *handler);

    void setHandler(BaseProxyHandler *handler) {
        SetReservedSlot(this, HANDLER_SLOT, PrivateValue(handler));
    }

    static size_t offsetOfHandler() {
        return getFixedSlotOffset(HANDLER_SLOT);
    }

    const Value &extra(size_t n) const {
        JS_ASSERT(n == 0 || n == 1);
        return GetReservedSlot(const_cast<ProxyObject*>(this), EXTRA_SLOT + n);
    }

    void setExtra(size_t n, const Value &extra) {
        JS_ASSERT(n == 0 || n == 1);
        SetReservedSlot(this, EXTRA_SLOT + n, extra);
    }

  private:
    HeapSlot *slotOfExtra(size_t n) {
        JS_ASSERT(n == 0 || n == 1);
        return &getReservedSlotRef(EXTRA_SLOT + n);
    }

  public:
    static unsigned grayLinkSlot(JSObject *obj);

    void renew(JSContext *cx, BaseProxyHandler *handler, Value priv);

    static void trace(JSTracer *trc, JSObject *obj);

    void nuke(BaseProxyHandler *handler);

    static const Class callableClass_;
    static const Class uncallableClass_;
};

class OuterWindowProxyObject : public ProxyObject
{
  public:
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

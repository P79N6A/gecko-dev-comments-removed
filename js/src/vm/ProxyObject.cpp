





#include "vm/ProxyObject.h"

#include "jscompartment.h"
#include "jsgcinlines.h"
#include "jsobjinlines.h"

using namespace js;

 ProxyObject *
ProxyObject::New(JSContext *cx, const BaseProxyHandler *handler, HandleValue priv, TaggedProto proto_,
                 JSObject *parent_, const ProxyOptions &options)
{
    Rooted<TaggedProto> proto(cx, proto_);
    RootedObject parent(cx, parent_);

    const Class *clasp = options.clasp();

    MOZ_ASSERT(isValidProxyClass(clasp));
    MOZ_ASSERT_IF(proto.isObject(), cx->compartment() == proto.toObject()->compartment());
    MOZ_ASSERT_IF(parent, cx->compartment() == parent->compartment());

    






    if (proto.isObject() && !options.singleton() && !clasp->isDOMClass()) {
        RootedObject protoObj(cx, proto.toObject());
        if (!JSObject::setNewGroupUnknown(cx, clasp, protoObj))
            return nullptr;
    }

    NewObjectKind newKind = options.singleton() ? SingletonObject : GenericObject;
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);

    if (handler->finalizeInBackground(priv))
        allocKind = GetBackgroundAllocKind(allocKind);

    ProxyValueArray *values = cx->zone()->new_<ProxyValueArray>();
    if (!values)
        return nullptr;

    
    
    RootedObject obj(cx, NewObjectWithGivenTaggedProto(cx, clasp, proto, parent, allocKind,
                                                       newKind));
    if (!obj) {
        js_free(values);
        return nullptr;
    }

    Rooted<ProxyObject*> proxy(cx, &obj->as<ProxyObject>());

    proxy->data.values = values;
    proxy->data.handler = handler;

    proxy->setCrossCompartmentPrivate(priv);

    
    if (newKind != SingletonObject && !clasp->isDOMClass())
        MarkObjectGroupUnknownProperties(cx, proxy->group());

    return proxy;
}

void
ProxyObject::setCrossCompartmentPrivate(const Value &priv)
{
    *slotOfPrivate() = priv;
}

void
ProxyObject::setSameCompartmentPrivate(const Value &priv)
{
    MOZ_ASSERT(IsObjectValueInCompartment(priv, compartment()));
    *slotOfPrivate() = priv;
}

void
ProxyObject::nuke(const BaseProxyHandler *handler)
{
    setSameCompartmentPrivate(NullValue());
    for (size_t i = 0; i < PROXY_EXTRA_SLOTS; i++)
        SetProxyExtra(this, i, NullValue());

    
    setHandler(handler);
}

JS_FRIEND_API(void)
js::SetValueInProxy(Value *slot, const Value &value)
{
    
    
    *reinterpret_cast<HeapValue *>(slot) = value;
}

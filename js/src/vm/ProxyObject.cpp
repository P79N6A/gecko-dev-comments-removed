





#include "vm/ProxyObject.h"

#include "jscompartment.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

using namespace js;

 ProxyObject *
ProxyObject::New(JSContext *cx, BaseProxyHandler *handler, HandleValue priv, TaggedProto proto_,
                 JSObject *parent_, const ProxyOptions &options)
{
    Rooted<TaggedProto> proto(cx, proto_);
    RootedObject parent(cx, parent_);

    JS_ASSERT_IF(proto.isObject(), cx->compartment() == proto.toObject()->compartment());
    JS_ASSERT_IF(parent, cx->compartment() == parent->compartment());
    const Class *clasp;
    if (handler->isOuterWindow())
        clasp = &OuterWindowProxyObject::class_;
    else
        clasp = options.callable() ? &ProxyObject::callableClass_
                                   : &ProxyObject::uncallableClass_;

    




    if (proto.isObject() && !options.singleton()) {
        RootedObject protoObj(cx, proto.toObject());
        if (!JSObject::setNewTypeUnknown(cx, clasp, protoObj))
            return NULL;
    }

    NewObjectKind newKind =
        (clasp == &OuterWindowProxyObject::class_ || options.singleton()) ? SingletonObject : GenericObject;
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    if (handler->finalizeInBackground(priv))
        allocKind = GetBackgroundAllocKind(allocKind);
    RootedObject obj(cx, NewObjectWithGivenProto(cx, clasp, proto, parent, allocKind, newKind));
    if (!obj)
        return NULL;

    Rooted<ProxyObject*> proxy(cx, &obj->as<ProxyObject>());
    proxy->initHandler(handler);
    proxy->initCrossCompartmentPrivate(priv);

    
    if (newKind != SingletonObject)
        MarkTypeObjectUnknownProperties(cx, proxy->type());

    return proxy;
}

void
ProxyObject::initCrossCompartmentPrivate(HandleValue priv)
{
    initCrossCompartmentSlot(PRIVATE_SLOT, priv);
}

void
ProxyObject::initHandler(BaseProxyHandler *handler)
{
    initSlot(HANDLER_SLOT, PrivateValue(handler));
}

static void
NukeSlot(ProxyObject *proxy, uint32_t slot)
{
    Value old = proxy->getSlot(slot);
    if (old.isMarkable()) {
        Zone *zone = ZoneOfValue(old);
        AutoMarkInDeadZone amd(zone);
        proxy->setReservedSlot(slot, NullValue());
    } else {
        proxy->setReservedSlot(slot, NullValue());
    }
}

void
ProxyObject::nuke(BaseProxyHandler *handler)
{
    NukeSlot(this, PRIVATE_SLOT);
    setHandler(handler);

    NukeSlot(this, EXTRA_SLOT + 0);
    NukeSlot(this, EXTRA_SLOT + 1);
}

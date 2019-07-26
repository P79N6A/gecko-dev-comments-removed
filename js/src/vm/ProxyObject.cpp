





#include "vm/ProxyObject.h"

#include "jscompartment.h"
#include "jsgcinlines.h"

using namespace js;

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

    if (is<FunctionProxyObject>())
        as<FunctionProxyObject>().nukeExtra();
}

void
FunctionProxyObject::nukeExtra()
{
    NukeSlot(this, CALL_SLOT);
    NukeSlot(this, CONSTRUCT_SLOT);
}

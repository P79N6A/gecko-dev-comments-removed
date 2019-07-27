






#include "jsfriendapi.h"
#include "xpcprivate.h"
#include "CPOWTimer.h"

CPOWTimer::~CPOWTimer()
{
    
    nsIGlobalObject *global = mozilla::dom::GetIncumbentGlobal();
    if (!global)
        return;
    JSObject *obj = global->GetGlobalJSObject();
    if (!obj)
        return;
    JSCompartment *compartment = js::GetObjectCompartment(obj);
    xpc::CompartmentPrivate *compartmentPrivate = xpc::CompartmentPrivate::Get(compartment);
    if (!compartmentPrivate)
        return;
    PRIntervalTime time = PR_IntervalNow() - startInterval;
    compartmentPrivate->CPOWTime += time;
}

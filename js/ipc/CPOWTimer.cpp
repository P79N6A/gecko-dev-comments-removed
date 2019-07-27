






#include "jsfriendapi.h"
#include "xpcprivate.h"
#include "CPOWTimer.h"

CPOWTimer::~CPOWTimer() {
    
    xpc::CompartmentPrivate* compartment = xpc::CompartmentPrivate::Get(js::GetObjectCompartment(mozilla::dom::GetIncumbentGlobal()
                                                                                                 ->GetGlobalJSObject()));
    PRIntervalTime time = PR_IntervalNow() - startInterval;
    compartment->CPOWTime += time;
}

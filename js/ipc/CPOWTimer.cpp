






#include "jsfriendapi.h"
#include "xpcprivate.h"
#include "CPOWTimer.h"

CPOWTimer::~CPOWTimer() {
    PRIntervalTime time = PR_IntervalNow() - startInterval;
    xpc::CompartmentPrivate *compartment = xpc::CompartmentPrivate::Get(js::GetObjectCompartment(jsobj));
    compartment->CPOWTime += time;
}

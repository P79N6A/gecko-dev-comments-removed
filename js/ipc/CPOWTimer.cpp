






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
    if (!compartment)
        return;
    js::PerformanceData *performance = js::GetPerformanceData(compartment);
    if (!performance)
        return;
    uint64_t time = PR_IntervalToMicroseconds(PR_IntervalNow() - startInterval);
    performance->cpowTime += time;
}

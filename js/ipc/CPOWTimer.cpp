






#include "jsfriendapi.h"
#include "nsContentUtils.h"
#include "CPOWTimer.h"

CPOWTimer::CPOWTimer(JSContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
    : cx_(nullptr)
    , startInterval_(0)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    JSRuntime* runtime = JS_GetRuntime(cx);
    if (!js::GetStopwatchIsMonitoringCPOW(runtime))
        return;
    cx_ = cx;
    startInterval_ = PR_IntervalNow();
}
CPOWTimer::~CPOWTimer()
{
    if (!cx_) {
        
        return;
    }

    JSRuntime* runtime = JS_GetRuntime(cx_);
    if (!js::GetStopwatchIsMonitoringCPOW(runtime)) {
        
        return;
    }

    js::PerformanceData* performance = js::GetPerformanceData(runtime);
    uint64_t duration = PR_IntervalToMicroseconds(PR_IntervalNow() - startInterval_);
    performance->totalCPOWTime += duration;
}








#include "jsfriendapi.h"
#include "nsContentUtils.h"
#include "CPOWTimer.h"

CPOWTimer::~CPOWTimer()
{
    JSContext* cx = nsContentUtils::GetCurrentJSContextForThread();
    if (!cx)
        return;

    JSRuntime* runtime = JS_GetRuntime(cx);
    if (!js::IsStopwatchActive(runtime))
        return;

    js::PerformanceData* performance = js::GetPerformanceData(runtime);
    uint64_t duration = PR_IntervalToMicroseconds(PR_IntervalNow() - startInterval);
    performance->totalCPOWTime += duration;
}

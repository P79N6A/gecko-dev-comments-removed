



#include "StartupTimeline.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "nsXULAppAPI.h"

namespace mozilla {

TimeStamp StartupTimeline::sStartupTimeline[StartupTimeline::MAX_EVENT_ID];
const char *StartupTimeline::sStartupTimelineDesc[StartupTimeline::MAX_EVENT_ID] = {
#define mozilla_StartupTimeline_Event(ev, desc) desc,
#include "StartupTimeline.h"
#undef mozilla_StartupTimeline_Event
};

} 

using mozilla::StartupTimeline;
using mozilla::TimeStamp;









void
XRE_StartupTimelineRecord(int aEvent, TimeStamp aWhen)
{
  StartupTimeline::Record((StartupTimeline::Event)aEvent, aWhen);
}

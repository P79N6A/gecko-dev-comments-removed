



#include "StartupTimeline.h"
#include "nsXULAppAPI.h"

namespace mozilla {

TimeStamp StartupTimeline::sStartupTimeline[StartupTimeline::MAX_EVENT_ID];
const char *StartupTimeline::sStartupTimelineDesc[StartupTimeline::MAX_EVENT_ID] = {
#define mozilla_StartupTimeline_Event(ev, desc) desc,
#include "StartupTimeline.h"
#undef mozilla_StartupTimeline_Event
};

} 





void
XRE_StartupTimelineRecord(int aEvent, PRTime aWhen)
{
  

  mozilla::StartupTimeline::Record((mozilla::StartupTimeline::Event) aEvent,
    mozilla::TimeStamp());
}

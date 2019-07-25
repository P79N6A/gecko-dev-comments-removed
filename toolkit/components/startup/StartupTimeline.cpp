




































#include "StartupTimeline.h"

namespace mozilla {

PRTime StartupTimeline::sStartupTimeline[StartupTimeline::MAX_EVENT_ID];
const char *StartupTimeline::sStartupTimelineDesc[StartupTimeline::MAX_EVENT_ID] = {
#define mozilla_StartupTimeline_Event(ev, desc) desc,
#include "StartupTimeline.h"
#undef mozilla_StartupTimeline_Event
};

}

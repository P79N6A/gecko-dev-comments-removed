



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







void
StartupTimelineRecordExternal(int aEvent, uint64_t aWhen)
{
#if XP_WIN
  TimeStamp ts = TimeStampValue(aWhen, 0, 0);
#else
  TimeStamp ts = TimeStampValue(aWhen);
#endif
  bool error = false;

  
  
  if (ts < TimeStamp::ProcessCreation(error)) {
    Telemetry::Accumulate(Telemetry::STARTUP_MEASUREMENT_ERRORS,
      (StartupTimeline::Event)aEvent);
  } else {
    StartupTimeline::Record((StartupTimeline::Event)aEvent, ts);
  }
}

} 

















void
XRE_StartupTimelineRecord(int aEvent, PRTime aWhen)
{
  mozilla::StartupTimelineRecordExternal(aEvent, aWhen);
}

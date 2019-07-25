




































#ifdef mozilla_StartupTimeline_Event
mozilla_StartupTimeline_Event(PROCESS_CREATION, "process")
mozilla_StartupTimeline_Event(MAIN, "main")
mozilla_StartupTimeline_Event(FIRST_PAINT, "firstPaint")
mozilla_StartupTimeline_Event(SESSION_RESTORED, "sessionRestored")
mozilla_StartupTimeline_Event(CREATE_TOP_LEVEL_WINDOW, "createTopLevelWindow")
mozilla_StartupTimeline_Event(LINKER_INITIALIZED, "linkerInitialized")
mozilla_StartupTimeline_Event(LIBRARIES_LOADED, "librariesLoaded")
#else

#ifndef mozilla_StartupTimeline
#define mozilla_StartupTimeline

#include "prtime.h"
#include "nscore.h"

namespace mozilla {

class StartupTimeline {
public:
  enum Event {
    #define mozilla_StartupTimeline_Event(ev, z) ev,
    #include __FILE__
    #undef mozilla_StartupTimeline_Event
    MAX_EVENT_ID
  };

  static PRTime Get(Event ev) {
    return sStartupTimeline[ev];
  }

  static const char *Describe(Event ev) {
    return sStartupTimelineDesc[ev];
  }

  static void Record(Event ev, PRTime when = PR_Now()) {
    sStartupTimeline[ev] = when;
  }

  static void RecordOnce(Event ev, PRTime when = PR_Now()) {
    if (!sStartupTimeline[ev])
      sStartupTimeline[ev] = when;
  }

private:
  static NS_EXTERNAL_VIS_(PRTime) sStartupTimeline[MAX_EVENT_ID];
  static NS_EXTERNAL_VIS_(const char *) sStartupTimelineDesc[MAX_EVENT_ID];
};

}

#endif

#endif

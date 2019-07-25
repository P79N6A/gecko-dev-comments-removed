



#ifndef TestCommon_h__
#define TestCommon_h__

#include <stdlib.h>
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"

inline int test_common_init(int *argc, char ***argv)
{
  return 0;
}



static bool gKeepPumpingEvents = false;

class nsQuitPumpingEvent MOZ_FINAL : public nsIRunnable {
public:
  NS_DECL_ISUPPORTS
  NS_IMETHOD Run() {
    gKeepPumpingEvents = false;
    return NS_OK;
  }
};
NS_IMPL_THREADSAFE_ISUPPORTS1(nsQuitPumpingEvent, nsIRunnable)

static inline void PumpEvents()
{
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

  gKeepPumpingEvents = true;
  while (gKeepPumpingEvents)
    NS_ProcessNextEvent(thread);

  NS_ProcessPendingEvents(thread);
}

static inline void QuitPumpingEvents()
{
  
  
  nsCOMPtr<nsIRunnable> event = new nsQuitPumpingEvent();
  NS_DispatchToMainThread(event);
}

#endif





































#ifndef TestCommon_h__
#define TestCommon_h__

#include <stdlib.h>
#include "nsThreadUtils.h"

inline int test_common_init(int *argc, char ***argv)
{
  return 0;
}



static PRBool gKeepPumpingEvents = PR_FALSE;

class nsQuitPumpingEvent : public nsIRunnable {
public:
  NS_DECL_ISUPPORTS
  NS_IMETHOD Run() {
    gKeepPumpingEvents = PR_FALSE;
    return NS_OK;
  }
};
NS_IMPL_THREADSAFE_ISUPPORTS1(nsQuitPumpingEvent, nsIRunnable)

static inline void PumpEvents()
{
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

  gKeepPumpingEvents = PR_TRUE;
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

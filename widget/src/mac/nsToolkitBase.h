






































#include <IOKit/IOKitLib.h>


#include "nsIToolkit.h"

#include "nsCOMPtr.h"
#include "nsIObserver.h"

class nsToolkitBase : public nsIToolkit, public nsIObserver
{
public:
                    nsToolkitBase();
  virtual           ~nsToolkitBase();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSITOOLKIT
  NS_DECL_NSIOBSERVER
  
  static void       PostSleepWakeNotification(const char* aNotification);

protected:

  virtual nsresult  InitEventQueue(PRThread * aThread) = 0;
  
  nsresult          RegisterForSleepWakeNotifcations();
  void              RemoveSleepWakeNotifcations();

protected:

  static int        QuartzChangedCallback(const char* pref, void* data);
  static void       SetupQuartzRendering();

  
protected:

  bool              mInited;

  CFRunLoopSourceRef   mSleepWakeNotificationRLS;
  io_object_t       mPowerNotifier;
};

extern nsToolkitBase* NS_CreateToolkitInstance();

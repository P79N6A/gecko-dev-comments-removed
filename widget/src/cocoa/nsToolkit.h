





































#ifndef nsToolkit_h_
#define nsToolkit_h_

#include "nsIToolkit.h"

#import <IOKit/IOKitLib.h>
























#define MAC_OS_X_VERSION_10_2_HEX 0x00001020
#define MAC_OS_X_VERSION_10_3_HEX 0x00001030
#define MAC_OS_X_VERSION_10_4_HEX 0x00001040
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050

class nsToolkit : public nsIToolkit
{
public:
                     nsToolkit();
  virtual            ~nsToolkit();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITOOLKIT

  
  static long        OSXVersion();
  
  
  static PRBool      OnPantherOrLater();
  static PRBool      OnTigerOrLater();
  static PRBool      OnLeopardOrLater();
  
  static void        PostSleepWakeNotification(const char* aNotification);

protected:

  nsresult           RegisterForSleepWakeNotifcations();
  void               RemoveSleepWakeNotifcations();

  void               RegisterForAllProcessMouseEvents();

protected:

  bool               mInited;

  CFRunLoopSourceRef mSleepWakeNotificationRLS;
  io_object_t        mPowerNotifier;
};

extern nsToolkit* NS_CreateToolkitInstance();

#endif 

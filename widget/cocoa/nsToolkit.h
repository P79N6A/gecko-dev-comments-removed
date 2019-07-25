





































#ifndef nsToolkit_h_
#define nsToolkit_h_

#include "nscore.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <objc/Object.h>
#import <IOKit/IOKitLib.h>

#define MAC_OS_X_VERSION_10_5_HEX 0x00001050
#define MAC_OS_X_VERSION_10_6_HEX 0x00001060
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

class nsToolkit
{
public:
                     nsToolkit();
  virtual            ~nsToolkit();

  static nsToolkit* GetToolkit();

  static void Shutdown() {
    delete gToolkit;
    gToolkit = nsnull;
  }

  static PRInt32     OSXVersion();
  static bool        OnSnowLeopardOrLater();
  static bool        OnLionOrLater();

  static void        PostSleepWakeNotification(const char* aNotification);

  static nsresult    SwizzleMethods(Class aClass, SEL orgMethod, SEL posedMethod,
                                    bool classMethods = false);

protected:

  nsresult           RegisterForSleepWakeNotifcations();
  void               RemoveSleepWakeNotifcations();

  void               RegisterForAllProcessMouseEvents();
  void               UnregisterAllProcessMouseEventHandlers();

protected:

  static nsToolkit* gToolkit;

  CFRunLoopSourceRef mSleepWakeNotificationRLS;
  io_object_t        mPowerNotifier;

  CFMachPortRef      mEventTapPort;
  CFRunLoopSourceRef mEventTapRLS;
};

#endif 







































#ifndef nsToolkit_h_
#define nsToolkit_h_

#include "nsIToolkit.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <objc/Object.h>
#import <IOKit/IOKitLib.h>

#define MAC_OS_X_VERSION_10_4_HEX 0x00001040
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050
#define MAC_OS_X_VERSION_10_6_HEX 0x00001060

class nsToolkit : public nsIToolkit
{
public:
                     nsToolkit();
  virtual            ~nsToolkit();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITOOLKIT

  
  static PRInt32     OSXVersion();

  
  static PRBool      OnLeopardOrLater();
  static PRBool      OnSnowLeopardOrLater();

  static void        PostSleepWakeNotification(const char* aNotification);

  static nsresult    SwizzleMethods(Class aClass, SEL orgMethod, SEL posedMethod,
                                    PRBool classMethods = PR_FALSE);

protected:

  nsresult           RegisterForSleepWakeNotifcations();
  void               RemoveSleepWakeNotifcations();

  void               RegisterForAllProcessMouseEvents();
  void               UnregisterAllProcessMouseEventHandlers();

protected:

  bool               mInited;

  CFRunLoopSourceRef mSleepWakeNotificationRLS;
  io_object_t        mPowerNotifier;

  EventHandlerRef    mEventMonitorHandler;
  CFMachPortRef      mEventTapPort;
  CFRunLoopSourceRef mEventTapRLS;
};

extern nsToolkit* NS_CreateToolkitInstance();

#endif 

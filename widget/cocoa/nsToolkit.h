




#ifndef nsToolkit_h_
#define nsToolkit_h_

#include "nscore.h"
#include "nsCocoaFeatures.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <objc/Object.h>
#import <IOKit/IOKitLib.h>

class nsToolkit
{
public:
                     nsToolkit();
  virtual            ~nsToolkit();

  static nsToolkit*  GetToolkit();

  static void Shutdown() {
    delete gToolkit;
    gToolkit = nullptr;
  }

  static void        PostSleepWakeNotification(const char* aNotification);

  static nsresult    SwizzleMethods(Class aClass, SEL orgMethod, SEL posedMethod,
                                    bool classMethods = false);

  void               RegisterForAllProcessMouseEvents();
  void               UnregisterAllProcessMouseEventHandlers();

protected:

  nsresult           RegisterForSleepWakeNotifications();
  void               RemoveSleepWakeNotifications();

protected:

  static nsToolkit*  gToolkit;

  CFRunLoopSourceRef mSleepWakeNotificationRLS;
  io_object_t        mPowerNotifier;

  CFMachPortRef      mEventTapPort;
  CFRunLoopSourceRef mEventTapRLS;
};

#endif 

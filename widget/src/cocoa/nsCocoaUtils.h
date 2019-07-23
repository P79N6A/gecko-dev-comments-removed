







































#ifndef nsCocoaUtils_h_
#define nsCocoaUtils_h_

#import <Cocoa/Cocoa.h>

#include "nsRect.h"
#include "nsIWidget.h"
#include "nsObjCExceptions.h"









#ifndef NSINTEGER_DEFINED

typedef int NSInteger;
typedef unsigned int NSUInteger;

#define NSIntegerMax    LONG_MAX
#define NSIntegerMin    LONG_MIN
#define NSUIntegerMax   ULONG_MAX

#define NSINTEGER_DEFINED 1

#endif  



class nsAutoRetainCocoaObject {
public:
nsAutoRetainCocoaObject(id anObject)
{
  mObject = NS_OBJC_TRY_EXPR_ABORT([anObject retain]);
}
~nsAutoRetainCocoaObject()
{
  NS_OBJC_TRY_ABORT([mObject release]);
}
private:
  id mObject;  
};


@interface NSApplication (Undocumented)


- (BOOL)_isRunningModal;
- (BOOL)_isRunningAppModal;





- (void)_removeWindowFromCache:(NSWindow *)aWindow;

@end

class nsCocoaUtils
{
  public:
  
  
  static float MenuBarScreenHeight();

  
  
  static float FlippedScreenY(float y);
  
  
  
  
  
  
  static NSRect GeckoRectToCocoaRect(const nsRect &geckoRect);
  
  
  static nsRect CocoaRectToGeckoRect(const NSRect &cocoaRect);
  
  
  
  static NSPoint ScreenLocationForEvent(NSEvent* anEvent);
  
  
  
  static BOOL IsEventOverWindow(NSEvent* anEvent, NSWindow* aWindow);
  
  
  
  
  
  static NSPoint EventLocationForWindow(NSEvent* anEvent, NSWindow* aWindow);
  
  
  static NSWindow* FindWindowUnderPoint(NSPoint aPoint);

  static nsIWidget* GetHiddenWindowWidget();

  static void PrepareForNativeAppModalDialog();
  static void CleanUpAfterNativeAppModalDialog();

  
  
  
  static unsigned short GetCocoaEventKeyCode(NSEvent *theEvent);
  static NSUInteger GetCocoaEventModifierFlags(NSEvent *theEvent);
};

#endif 

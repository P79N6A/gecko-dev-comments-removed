







































#ifndef nsCocoaUtils_h_
#define nsCocoaUtils_h_

#import <Cocoa/Cocoa.h>

#include "nsRect.h"
#include "nsIWidget.h"

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
};

#endif 

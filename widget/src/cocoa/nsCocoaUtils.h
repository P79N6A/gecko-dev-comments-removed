







































#ifndef nsCocoaUtils_h_
#define nsCocoaUtils_h_

#import <Cocoa/Cocoa.h>

#include "nsRect.h"
#include "nsObjCExceptions.h"
#include "imgIContainer.h"

class nsIWidget;


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



- (void)_modalSession:(NSModalSession)aSession sendEvent:(NSEvent *)theEvent;




- (void)setHelpMenu:(NSMenu *)helpMenu;

@end

class nsCocoaUtils
{
  public:
  
  
  static float MenuBarScreenHeight();

  
  
  static float FlippedScreenY(float y);
  
  
  
  
  
  
  static NSRect GeckoRectToCocoaRect(const nsIntRect &geckoRect);
  
  
  static nsIntRect CocoaRectToGeckoRect(const NSRect &cocoaRect);
  
  
  
  
  static NSPoint ScreenLocationForEvent(NSEvent* anEvent);
  
  
  
  static BOOL IsEventOverWindow(NSEvent* anEvent, NSWindow* aWindow);

  
  
  
  
  static NSPoint EventLocationForWindow(NSEvent* anEvent, NSWindow* aWindow);

  
  static void HideOSChromeOnScreen(PRBool aShouldHide, NSScreen* aScreen);

  static nsIWidget* GetHiddenWindowWidget();

  static void PrepareForNativeAppModalDialog();
  static void CleanUpAfterNativeAppModalDialog();

  
  
  
  







  static nsresult CreateCGImageFromImageContainer(imgIContainer *aImage, PRUint32 aWhichFrame, CGImageRef *aResult);
  
  






  static nsresult CreateNSImageFromCGImage(CGImageRef aInputImage, NSImage **aResult);

  





  
  static nsresult CreateNSImageFromImageContainer(imgIContainer *aImage, PRUint32 aWhichFrame, NSImage **aResult);
};

#endif 

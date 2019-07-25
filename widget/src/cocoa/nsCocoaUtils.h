







































#ifndef nsCocoaUtils_h_
#define nsCocoaUtils_h_

#import <Cocoa/Cocoa.h>

#include "nsRect.h"
#include "nsObjCExceptions.h"
#include "imgIContainer.h"
#include "nsEvent.h"
#include "npapi.h"

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


class nsAutoreleasePool {
public:
  nsAutoreleasePool()
  {
    mLocalPool = [[NSAutoreleasePool alloc] init];
  }
  ~nsAutoreleasePool()
  {
    [mLocalPool release];
  }
private:
  NSAutoreleasePool *mLocalPool;
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

  static BOOL IsMomentumScrollEvent(NSEvent* aEvent);

  
  static void HideOSChromeOnScreen(bool aShouldHide, NSScreen* aScreen);

  static nsIWidget* GetHiddenWindowWidget();

  static void PrepareForNativeAppModalDialog();
  static void CleanUpAfterNativeAppModalDialog();

  
  
  
  






  static nsresult CreateCGImageFromSurface(gfxImageSurface *aFrame, CGImageRef *aResult);
  
  






  static nsresult CreateNSImageFromCGImage(CGImageRef aInputImage, NSImage **aResult);

  





  
  static nsresult CreateNSImageFromImageContainer(imgIContainer *aImage, PRUint32 aWhichFrame, NSImage **aResult);

  


  static void GetStringForNSString(const NSString *aSrc, nsAString& aDist);

  


  static NSString* ToNSString(const nsAString& aString);

  


  static void GeckoRectToNSRect(const nsIntRect& aGeckoRect,
                                       NSRect& aOutCocoaRect);

  


  static NSEvent* MakeNewCocoaEventWithType(NSEventType aEventType,
                                            NSEvent *aEvent);

  


  static void InitNPCocoaEvent(NPCocoaEvent* aNPCocoaEvent);

  


  static void InitPluginEvent(nsPluginEvent &aPluginEvent,
                              NPCocoaEvent &aCocoaEvent);
};

#endif 

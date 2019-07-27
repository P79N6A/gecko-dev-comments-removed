




#ifndef nsCocoaUtils_h_
#define nsCocoaUtils_h_

#import <Cocoa/Cocoa.h>

#include "nsRect.h"
#include "imgIContainer.h"
#include "npapi.h"
#include "nsTArray.h"


#include "nsObjCExceptions.h"

#include "mozilla/EventForwards.h"



@interface NSObject (BackingScaleFactorCategory)
- (CGFloat)backingScaleFactor;
@end


#if !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
enum {
  NSEventPhaseNone        = 0,
  NSEventPhaseBegan       = 0x1 << 0,
  NSEventPhaseStationary  = 0x1 << 1,
  NSEventPhaseChanged     = 0x1 << 2,
  NSEventPhaseEnded       = 0x1 << 3,
  NSEventPhaseCancelled   = 0x1 << 4,
};
typedef NSUInteger NSEventPhase;
#endif 

#if !defined(MAC_OS_X_VERSION_10_8) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_8
enum {
  NSEventPhaseMayBegin    = 0x1 << 5
};
#endif 

class nsIWidget;

namespace mozilla {
namespace gfx {
class SourceSurface;
}
}


class nsAutoRetainCocoaObject {
public:
explicit nsAutoRetainCocoaObject(id anObject)
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

struct KeyBindingsCommand
{
  SEL selector;
  id data;
};

@interface NativeKeyBindingsRecorder : NSResponder
{
@private
  nsTArray<KeyBindingsCommand>* mCommands;
}

- (void)startRecording:(nsTArray<KeyBindingsCommand>&)aCommands;

- (void)doCommandBySelector:(SEL)aSelector;

- (void)insertText:(id)aString;

@end 

class nsCocoaUtils
{
  typedef mozilla::gfx::SourceSurface SourceSurface;

public:

  
  
  static CGFloat
  GetBackingScaleFactor(id aObject)
  {
    if (HiDPIEnabled() &&
        [aObject respondsToSelector:@selector(backingScaleFactor)]) {
      return [aObject backingScaleFactor];
    }
    return 1.0;
  }

  
  
  static int32_t
  CocoaPointsToDevPixels(CGFloat aPts, CGFloat aBackingScale)
  {
    return NSToIntRound(aPts * aBackingScale);
  }

  static nsIntPoint
  CocoaPointsToDevPixels(const NSPoint& aPt, CGFloat aBackingScale)
  {
    return nsIntPoint(NSToIntRound(aPt.x * aBackingScale),
                      NSToIntRound(aPt.y * aBackingScale));
  }

  static nsIntRect
  CocoaPointsToDevPixels(const NSRect& aRect, CGFloat aBackingScale)
  {
    return nsIntRect(NSToIntRound(aRect.origin.x * aBackingScale),
                     NSToIntRound(aRect.origin.y * aBackingScale),
                     NSToIntRound(aRect.size.width * aBackingScale),
                     NSToIntRound(aRect.size.height * aBackingScale));
  }

  static CGFloat
  DevPixelsToCocoaPoints(int32_t aPixels, CGFloat aBackingScale)
  {
    return (CGFloat)aPixels / aBackingScale;
  }

  static NSPoint
  DevPixelsToCocoaPoints(const nsIntPoint& aPt, CGFloat aBackingScale)
  {
    return NSMakePoint((CGFloat)aPt.x / aBackingScale,
                       (CGFloat)aPt.y / aBackingScale);
  }

  static NSRect
  DevPixelsToCocoaPoints(const nsIntRect& aRect, CGFloat aBackingScale)
  {
    return NSMakeRect((CGFloat)aRect.x / aBackingScale,
                      (CGFloat)aRect.y / aBackingScale,
                      (CGFloat)aRect.width / aBackingScale,
                      (CGFloat)aRect.height / aBackingScale);
  }

  
  
  static float FlippedScreenY(float y);

  
  
  
  
  

  
  
  
  
  
  
  
  static NSRect GeckoRectToCocoaRect(const nsIntRect &geckoRect);

  
  static NSRect GeckoRectToCocoaRectDevPix(const nsIntRect &aGeckoRect,
                                           CGFloat aBackingScale);

  
  static nsIntRect CocoaRectToGeckoRect(const NSRect &cocoaRect);

  static nsIntRect CocoaRectToGeckoRectDevPix(const NSRect &aCocoaRect,
                                              CGFloat aBackingScale);

  
  
  
  static NSPoint ScreenLocationForEvent(NSEvent* anEvent);
  
  
  
  static BOOL IsEventOverWindow(NSEvent* anEvent, NSWindow* aWindow);

  
  
  
  
  static NSPoint EventLocationForWindow(NSEvent* anEvent, NSWindow* aWindow);

  
  
  
  
  static NSEventPhase EventPhase(NSEvent* aEvent);
  static NSEventPhase EventMomentumPhase(NSEvent* aEvent);
  static BOOL IsMomentumScrollEvent(NSEvent* aEvent);
  static BOOL HasPreciseScrollingDeltas(NSEvent* aEvent);
  static void GetScrollingDeltas(NSEvent* aEvent, CGFloat* aOutDeltaX, CGFloat* aOutDeltaY);

  
  static void HideOSChromeOnScreen(bool aShouldHide, NSScreen* aScreen);

  static nsIWidget* GetHiddenWindowWidget();

  static void PrepareForNativeAppModalDialog();
  static void CleanUpAfterNativeAppModalDialog();

  
  
  
  






  static nsresult CreateCGImageFromSurface(SourceSurface* aSurface,
                                           CGImageRef* aResult);
  
  






  static nsresult CreateNSImageFromCGImage(CGImageRef aInputImage, NSImage **aResult);

  






  
  static nsresult CreateNSImageFromImageContainer(imgIContainer *aImage, uint32_t aWhichFrame, NSImage **aResult, CGFloat scaleFactor);

  


  static void GetStringForNSString(const NSString *aSrc, nsAString& aDist);

  


  static NSString* ToNSString(const nsAString& aString);

  




  static void GeckoRectToNSRect(const nsIntRect& aGeckoRect,
                                NSRect& aOutCocoaRect);

  




  static void NSRectToGeckoRect(const NSRect& aCocoaRect,
                                nsIntRect& aOutGeckoRect);

  


  static NSEvent* MakeNewCocoaEventWithType(NSEventType aEventType,
                                            NSEvent *aEvent);

  


  static void InitNPCocoaEvent(NPCocoaEvent* aNPCocoaEvent);

  


  static void InitPluginEvent(mozilla::WidgetPluginEvent &aPluginEvent,
                              NPCocoaEvent &aCocoaEvent);
  


  static void InitInputEvent(mozilla::WidgetInputEvent &aInputEvent,
                             NSEvent* aNativeEvent);
  static void InitInputEvent(mozilla::WidgetInputEvent &aInputEvent,
                             NSUInteger aModifiers);

  




  static UInt32 ConvertToCarbonModifier(NSUInteger aCocoaModifier);

  



  static bool HiDPIEnabled();

  




  static void GetCommandsFromKeyEvent(NSEvent* aEvent,
                                      nsTArray<KeyBindingsCommand>& aCommands);

  



  static uint32_t ConvertGeckoNameToMacCharCode(const nsAString& aKeyCodeName);

  



  static uint32_t ConvertGeckoKeyCodeToMacCharCode(uint32_t aKeyCode);
};

#endif 

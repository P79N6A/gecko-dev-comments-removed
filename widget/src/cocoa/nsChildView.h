





































#ifndef nsChildView_h_
#define nsChildView_h_


#include "mozView.h"
#ifdef ACCESSIBILITY
#include "nsAccessible.h"
#include "mozAccessibleProtocol.h"
#endif

#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "nsBaseWidget.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIPluginWidget.h"
#include "nsWeakPtr.h"
#include "TextInputHandler.h"
#include "nsCocoaUtils.h"

#include "nsString.h"
#include "nsIDragService.h"

#include "npapi.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/NSOpenGL.h>







#ifdef __cplusplus
extern "C" {
#endif
  #if !defined(__QUICKDRAWAPI__)

  extern void SetPort(GrafPtr port)
    __attribute__((weak_import));
  extern void SetOrigin(short h, short v)
    __attribute__((weak_import));
  extern RgnHandle NewRgn(void)
    __attribute__((weak_import));
  extern void DisposeRgn(RgnHandle rgn)
    __attribute__((weak_import));
  extern void RectRgn(RgnHandle rgn, const Rect * r)
    __attribute__((weak_import));
  extern GDHandle GetMainDevice(void)
    __attribute__((weak_import));
  extern Boolean IsPortOffscreen(CGrafPtr port)
    __attribute__((weak_import));
  extern void SetPortVisibleRegion(CGrafPtr port, RgnHandle visRgn)
    __attribute__((weak_import));
  extern void SetPortClipRegion(CGrafPtr port, RgnHandle clipRgn)
    __attribute__((weak_import));
  extern CGrafPtr GetQDGlobalsThePort(void)
    __attribute__((weak_import));

  #endif

  #if !defined(__QDOFFSCREEN__)

  extern void GetGWorld(CGrafPtr *  port, GDHandle *  gdh)
    __attribute__((weak_import));
  extern void SetGWorld(CGrafPtr port, GDHandle gdh)
    __attribute__((weak_import));

  #endif
#ifdef __cplusplus
}
#endif

class gfxASurface;
class nsChildView;
class nsCocoaWindow;
union nsPluginPort;

namespace mozilla {
namespace gl {
class TextureImage;
}
}

#ifndef NP_NO_CARBON
enum {
  
  
  
  
  kFocusedChildViewTSMDocPropertyTag  = 'GKFV', 
};












extern "C" long TSMProcessRawKeyEvent(EventRef carbonEvent);
#endif 

@interface NSEvent (Undocumented)




- (EventRef)_eventRef;

@end



@interface NSEvent (DeviceDelta)
  - (CGFloat)deviceDeltaX;
  - (CGFloat)deviceDeltaY;
@end



@interface NSEvent (ScrollPhase)
- (long long)_scrollPhase;
@end















#if !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
#ifdef __LP64__
enum {
  NSEventPhaseNone        = 0,
  NSEventPhaseBegan       = 0x1 << 0,
  NSEventPhaseStationary  = 0x1 << 1,
  NSEventPhaseChanged     = 0x1 << 2,
  NSEventPhaseEnded       = 0x1 << 3,
  NSEventPhaseCancelled   = 0x1 << 4,
};
typedef NSUInteger NSEventPhase;

enum {
  NSEventSwipeTrackingLockDirection = 0x1 << 0,
  NSEventSwipeTrackingClampGestureAmount = 0x1 << 1
};
typedef NSUInteger NSEventSwipeTrackingOptions;

enum {
  NSEventGestureAxisNone = 0,
  NSEventGestureAxisHorizontal,
  NSEventGestureAxisVertical
};
typedef NSInteger NSEventGestureAxis;

@interface NSEvent (FluidSwipeTracking)
+ (BOOL)isSwipeTrackingFromScrollEventsEnabled;
- (BOOL)hasPreciseScrollingDeltas;
- (CGFloat)scrollingDeltaX;
- (CGFloat)scrollingDeltaY;
- (NSEventPhase)phase;
- (void)trackSwipeEventWithOptions:(NSEventSwipeTrackingOptions)options
          dampenAmountThresholdMin:(CGFloat)minDampenThreshold
                               max:(CGFloat)maxDampenThreshold
                      usingHandler:(void (^)(CGFloat gestureAmount, NSEventPhase phase, BOOL isComplete, BOOL *stop))trackingHandler;
@end
#endif 
#endif 

@interface ChildView : NSView<
#ifdef ACCESSIBILITY
                              mozAccessible,
#endif
                              mozView, NSTextInput>
{
@private
  
  
  nsChildView* mGeckoChild;

  
  
  
  
  
  
  
  mozilla::widget::TextInputHandler* mTextInputHandler;  

  BOOL mIsPluginView;
  NPEventModel mPluginEventModel;
  NPDrawingModel mPluginDrawingModel;

  
  NSEvent* mLastMouseDownEvent;

  
  BOOL mBlockedLastMouseDown;

  
  NSEvent* mClickThroughMouseDownEvent;

  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;
  BOOL mPendingDisplay;

  
  
  
  
  
  nsIDragService* mDragService;

  NSOpenGLContext *mGLContext;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum {
    eGestureState_None,
    eGestureState_StartGesture,
    eGestureState_MagnifyGesture,
    eGestureState_RotateGesture
  } mGestureState;
  float mCumulativeMagnification;
  float mCumulativeRotation;

  BOOL mDidForceRefreshOpenGL;

  
#ifdef __LP64__
  BOOL *mSwipeAnimationCancelled;
#endif
}


+ (void)initialize;


- (void)viewsWindowDidBecomeKey;
- (void)viewsWindowDidResignKey;


- (void)delayedTearDown;

- (void)sendFocusEvent:(PRUint32)eventType;

- (void)handleMouseMoved:(NSEvent*)aEvent;

- (void)drawRect:(NSRect)aRect inTitlebarContext:(CGContextRef)aContext;

- (void)sendMouseEnterOrExitEvent:(NSEvent*)aEvent
                            enter:(BOOL)aEnter
                             type:(nsMouseEvent::exitType)aType;

- (void)update;
- (void)lockFocus;
- (void) _surfaceNeedsUpdate:(NSNotification*)notification;

- (BOOL)isPluginView;



- (BOOL)isInFailingLeftClickThrough;











- (void)swipeWithEvent:(NSEvent *)anEvent;
- (void)beginGestureWithEvent:(NSEvent *)anEvent;
- (void)magnifyWithEvent:(NSEvent *)anEvent;
- (void)rotateWithEvent:(NSEvent *)anEvent;
- (void)endGestureWithEvent:(NSEvent *)anEvent;


#ifdef __LP64__
- (void)maybeTrackScrollEventAsSwipe:(NSEvent *)anEvent
                      scrollOverflow:(PRInt32)overflow;
#endif
@end

class ChildViewMouseTracker {

public:

  static void MouseMoved(NSEvent* aEvent);
  static void MouseScrolled(NSEvent* aEvent);
  static void OnDestroyView(ChildView* aView);
  static void OnDestroyWindow(NSWindow* aWindow);
  static BOOL WindowAcceptsEvent(NSWindow* aWindow, NSEvent* aEvent,
                                 ChildView* aView, BOOL isClickThrough = NO);
  static void MouseExitedWindow(NSEvent* aEvent);
  static void MouseEnteredWindow(NSEvent* aEvent);
  static void ReEvaluateMouseEnterState(NSEvent* aEvent = nil);
  static void ResendLastMouseMoveEvent();
  static ChildView* ViewForEvent(NSEvent* aEvent);

  static ChildView* sLastMouseEventView;
  static NSEvent* sLastMouseMoveEvent;
  static NSWindow* sWindowUnderMouse;
  static NSPoint sLastScrollEventScreenLocation;
};







class nsChildView : public nsBaseWidget,
                    public nsIPluginWidget
{
private:
  typedef nsBaseWidget Inherited;

public:
                          nsChildView();
  virtual                 ~nsChildView();
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD              Create(nsIWidget *aParent,
                                 nsNativeWidget aNativeParent,
                                 const nsIntRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsDeviceContext *aContext,
                                 nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD              Destroy();

  NS_IMETHOD              Show(bool aState);
  NS_IMETHOD              IsVisible(bool& outState);

  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual float           GetDPI();

  NS_IMETHOD              ConstrainPosition(bool aAllowSlop,
                                            PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, bool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight, bool aRepaint);

  NS_IMETHOD              Enable(bool aState);
  NS_IMETHOD              IsEnabled(bool *aState);
  NS_IMETHOD              SetFocus(bool aRaise);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);

  NS_IMETHOD              Invalidate(const nsIntRect &aRect, bool aIsSynchronous);

  virtual void*           GetNativeData(PRUint32 aDataType);
  virtual nsresult        ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  virtual nsIntPoint      WidgetToScreenOffset();
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect);

  static  bool            ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);

  NS_IMETHOD              Update();
  virtual bool            GetShouldAccelerate();

  NS_IMETHOD        SetCursor(nsCursor aCursor);
  NS_IMETHOD        SetCursor(imgIContainer* aCursor, PRUint32 aHotspotX, PRUint32 aHotspotY);
  
  NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, bool aDoCapture, bool aConsumeRollupEvent);
  NS_IMETHOD        SetTitle(const nsAString& title);

  NS_IMETHOD        GetAttention(PRInt32 aCycleCount);

  virtual bool HasPendingInputEvent();

  NS_IMETHOD        ActivateNativeMenuItemAt(const nsAString& indexString);
  NS_IMETHOD        ForceUpdateNativeMenuAt(const nsAString& indexString);

  NS_IMETHOD        ResetInputState();
  NS_IMETHOD        SetIMEOpenState(bool aState);
  NS_IMETHOD        GetIMEOpenState(bool* aState);
  NS_IMETHOD        SetInputMode(const IMEContext& aContext);
  NS_IMETHOD        GetInputMode(IMEContext& aContext);
  NS_IMETHOD        CancelIMEComposition();
  NS_IMETHOD        GetToggledKeyState(PRUint32 aKeyCode,
                                       bool* aLEDState);
  NS_IMETHOD        OnIMEFocusChange(bool aFocus);

  
  NS_IMETHOD        GetPluginClipRect(nsIntRect& outClipRect, nsIntPoint& outOrigin, bool& outWidgetVisible);
  NS_IMETHOD        StartDrawPlugin();
  NS_IMETHOD        EndDrawPlugin();
  NS_IMETHOD        SetPluginInstanceOwner(nsIPluginInstanceOwner* aInstanceOwner);

  NS_IMETHOD        SetPluginEventModel(int inEventModel);
  NS_IMETHOD        GetPluginEventModel(int* outEventModel);
  NS_IMETHOD        SetPluginDrawingModel(int inDrawingModel);

  NS_IMETHOD        StartComplexTextInputForCurrentEvent();

  virtual nsTransparencyMode GetTransparencyMode();
  virtual void                SetTransparencyMode(nsTransparencyMode aMode);

  virtual nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                            PRInt32 aNativeKeyCode,
                                            PRUint32 aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters);

  virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                              PRUint32 aNativeMessage,
                                              PRUint32 aModifierFlags);
  
  
  
  virtual bool      DispatchWindowEvent(nsGUIEvent& event);
  
#ifdef ACCESSIBILITY
  already_AddRefed<nsAccessible> GetDocumentAccessible();
#endif

  virtual gfxASurface* GetThebesSurface();
  virtual void DrawOver(LayerManager* aManager, nsIntRect aRect);

  virtual void UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries);

  NS_IMETHOD BeginSecureKeyboardInput();
  NS_IMETHOD EndSecureKeyboardInput();

  void              HidePlugin();
  void              UpdatePluginPort();

  void              ResetParent();

  static bool DoHasPendingInputEvent();
  static PRUint32 GetCurrentInputEventCount();
  static void UpdateCurrentInputEventCount();

  NSView<mozView>* GetEditorView();

  bool IsPluginView() { return (mWindowType == eWindowType_plugin); }

  void PaintQD();

  nsCocoaWindow*    GetXULWindowWidget();

  NS_IMETHOD        ReparentNativeWidget(nsIWidget* aNewParent);

  mozilla::widget::TextInputHandler* GetTextInputHandler()
  {
    return mTextInputHandler;
  }

protected:

  bool              ReportDestroyEvent();
  bool              ReportMoveEvent();
  bool              ReportSizeEvent();

  
  
  virtual NSView*   CreateCocoaView(NSRect inFrame);
  void              TearDownView();

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_POPUP_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

protected:

  NSView<mozView>*      mView;      
  nsRefPtr<mozilla::widget::TextInputHandler> mTextInputHandler;
  IMEContext            mIMEContext;

  NSView<mozView>*      mParentView;
  nsIWidget*            mParentWidget;

#ifdef ACCESSIBILITY
  
  
  nsWeakPtr             mAccessible;
#endif

  nsRefPtr<gfxASurface> mTempThebesSurface;
  nsRefPtr<mozilla::gl::TextureImage> mResizerImage;

  bool                  mVisible;
  bool                  mDrawing;
  bool                  mPluginDrawing;
  bool                  mIsDispatchPaint; 

  NP_CGContext          mPluginCGContext;
#ifndef NP_NO_QUICKDRAW
  NP_Port               mPluginQDPort;
#endif
  nsIPluginInstanceOwner* mPluginInstanceOwner; 

  static PRUint32 sLastInputEventCount;
};

void NS_InstallPluginKeyEventsHandler();
void NS_RemovePluginKeyEventsHandler();

#endif 

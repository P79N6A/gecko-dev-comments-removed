





































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
#include "nsCocoaTextInputHandler.h"
#include "nsCocoaUtils.h"

#include "nsIAppShell.h"

#include "nsString.h"
#include "nsIDragService.h"

#include "npapi.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/NSOpenGL.h>

class gfxASurface;
class nsChildView;
class nsCocoaWindow;
union nsPluginPort;

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

@interface ChildView : NSView<
#ifdef ACCESSIBILITY
                              mozAccessible,
#endif
                              mozView, NSTextInput>
{
@private
  
  
  nsChildView* mGeckoChild;

  BOOL mIsPluginView;
  NPEventModel mPluginEventModel;
  NPDrawingModel mPluginDrawingModel;

  
  
  
  
  
  NSEvent* mCurKeyEvent;
  PRBool mKeyDownHandled;
  
  
  
  BOOL mKeyPressSent;
  
  PRBool mKeyPressHandled;

  
  NSRange mMarkedRange;
  
  
  NSEvent* mLastMouseDownEvent;

  
  BOOL mBlockedLastMouseDown;

  
  NSEvent* mClickThroughMouseDownEvent;

  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;
  BOOL mPendingDisplay;

  
  
  
  
  
  nsIDragService* mDragService;

#ifndef NP_NO_CARBON
  
  
  
  TSMDocumentID mPluginTSMDoc;
#endif
  BOOL mPluginComplexTextInputRequested;

  NSOpenGLContext *mGLContext;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum {
    eGestureState_None,
    eGestureState_StartGesture,
    eGestureState_MagnifyGesture,
    eGestureState_RotateGesture
  } mGestureState;
  float mCumulativeMagnification;
  float mCumulativeRotation;
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

#ifndef NP_NO_CARBON
- (void) processPluginKeyEvent:(EventRef)aKeyEvent;
#endif
- (void)pluginRequestsComplexTextInputForCurrentEvent;

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
@end

class ChildViewMouseTracker {

public:

  static void MouseMoved(NSEvent* aEvent);
  static void OnDestroyView(ChildView* aView);
  static BOOL WindowAcceptsEvent(NSWindow* aWindow, NSEvent* aEvent,
                                 ChildView* aView, BOOL isClickThrough = NO);
  static void ReEvaluateMouseEnterState(NSEvent* aEvent = nil);
  static ChildView* ViewForEvent(NSEvent* aEvent);

  static ChildView* sLastMouseEventView;

private:

  static NSWindow* WindowForEvent(NSEvent* aEvent);
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
                                 nsIDeviceContext *aContext,
                                 nsIAppShell *aAppShell = nsnull,
                                 nsIToolkit *aToolkit = nsnull,
                                 nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD              Destroy();

  NS_IMETHOD              Show(PRBool aState);
  NS_IMETHOD              IsVisible(PRBool& outState);

  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual float           GetDPI();

  NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
                                            PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);

  NS_IMETHOD              Enable(PRBool aState);
  NS_IMETHOD              IsEnabled(PRBool *aState);
  NS_IMETHOD              SetFocus(PRBool aRaise);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);

  NS_IMETHOD              Invalidate(const nsIntRect &aRect, PRBool aIsSynchronous);

  virtual void*           GetNativeData(PRUint32 aDataType);
  virtual nsresult        ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  virtual nsIntPoint      WidgetToScreenOffset();
  virtual PRBool          ShowsResizeIndicator(nsIntRect* aResizerRect);

  static  PRBool          ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);

  NS_IMETHOD              Update();
  virtual PRBool          GetShouldAccelerate();

  NS_IMETHOD        SetCursor(nsCursor aCursor);
  NS_IMETHOD        SetCursor(imgIContainer* aCursor, PRUint32 aHotspotX, PRUint32 aHotspotY);
  
  NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, nsIMenuRollup * aMenuRollup, 
                                        PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD        SetTitle(const nsAString& title);

  NS_IMETHOD        GetAttention(PRInt32 aCycleCount);

  virtual PRBool HasPendingInputEvent();

  NS_IMETHOD        ActivateNativeMenuItemAt(const nsAString& indexString);
  NS_IMETHOD        ForceUpdateNativeMenuAt(const nsAString& indexString);

  NS_IMETHOD        ResetInputState();
  NS_IMETHOD        SetIMEOpenState(PRBool aState);
  NS_IMETHOD        GetIMEOpenState(PRBool* aState);
  NS_IMETHOD        SetInputMode(const IMEContext& aContext);
  NS_IMETHOD        GetIMEEnabled(PRUint32* aState);
  NS_IMETHOD        CancelIMEComposition();
  NS_IMETHOD        GetToggledKeyState(PRUint32 aKeyCode,
                                       PRBool* aLEDState);
  NS_IMETHOD        OnIMEFocusChange(PRBool aFocus);

  
  NS_IMETHOD        GetPluginClipRect(nsIntRect& outClipRect, nsIntPoint& outOrigin, PRBool& outWidgetVisible);
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
  
  
  
  virtual PRBool    DispatchWindowEvent(nsGUIEvent& event);
  
#ifdef ACCESSIBILITY
  already_AddRefed<nsAccessible> GetDocumentAccessible();
#endif

  virtual gfxASurface* GetThebesSurface();

  NS_IMETHOD BeginSecureKeyboardInput();
  NS_IMETHOD EndSecureKeyboardInput();

  void              HidePlugin();
  void              UpdatePluginPort();

  void              ResetParent();

  static PRBool DoHasPendingInputEvent();
  static PRUint32 GetCurrentInputEventCount();
  static void UpdateCurrentInputEventCount();

  nsCocoaTextInputHandler* TextInputHandler() { return &mTextInputHandler; }
  NSView<mozView>* GetEditorView();

  PRBool IsPluginView() { return (mWindowType == eWindowType_plugin); }

  void PaintQD();

  nsCocoaWindow*    GetXULWindowWidget();

  NS_IMETHOD        ReparentNativeWidget(nsIWidget* aNewParent);
protected:

  PRBool            ReportDestroyEvent();
  PRBool            ReportMoveEvent();
  PRBool            ReportSizeEvent();

  
  
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
  nsCocoaTextInputHandler mTextInputHandler;

  NSView<mozView>*      mParentView;
  nsIWidget*            mParentWidget;

#ifdef ACCESSIBILITY
  
  
  nsWeakPtr             mAccessible;
#endif

  nsRefPtr<gfxASurface> mTempThebesSurface;

  PRPackedBool          mVisible;
  PRPackedBool          mDrawing;
  PRPackedBool          mPluginDrawing;
  PRPackedBool          mIsDispatchPaint; 

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

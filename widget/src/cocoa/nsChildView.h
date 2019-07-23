





































#ifndef nsChildView_h_
#define nsChildView_h_


#include "mozView.h"
#ifdef ACCESSIBILITY
#include "nsIAccessible.h"
#include "mozAccessibleProtocol.h"
#endif

#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "nsBaseWidget.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIPluginWidget.h"
#include "nsIScrollableView.h"
#include "nsWeakPtr.h"
#include "nsCocoaTextInputHandler.h"
#include "nsCocoaUtils.h"

#include "nsIAppShell.h"

#include "nsString.h"
#include "nsIDragService.h"

#include "npapi.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

class gfxASurface;
class nsChildView;
class nsCocoaWindow;
union nsPluginPort;

enum {
  
  
  
  
  kFocusedChildViewTSMDocPropertyTag  = 'GKFV', 
};












extern "C" long TSMProcessRawKeyEvent(EventRef carbonEvent);

@interface NSEvent (Undocumented)




- (EventRef)_eventRef;

@end






enum {
  mozkEventMouseScroll             = 11
};



@interface NSEvent (DeviceDelta)
  - (float)deviceDeltaX;
  - (float)deviceDeltaY;
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

  
  
  
  
  
  NSEvent* mCurKeyEvent;
  PRBool mKeyDownHandled;
  
  
  
  BOOL mKeyPressSent;
  
  PRBool mKeyPressHandled;

  
  NSRange mMarkedRange;
  
  
  NSEvent* mLastMouseDownEvent;
  
  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;

  
  
  BOOL mIsTransparent;
  PRIntervalTime mLastShadowInvalidation;
  BOOL mNeedsShadowInvalidation;

  
  
  
  
  
  nsIDragService* mDragService;
  
  
  
  
  TSMDocumentID mPluginTSMDoc;

  
  NSTrackingRectTag mTrackingRect;

  enum MouseEnterState {
    eMouseEnterState_Unknown,
    eMouseEnterState_Inside,
    eMouseEnterState_Outside
  } mMouseEnterState;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
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

- (void)setTransparent:(BOOL)transparent;

- (void)sendFocusEvent:(PRUint32)eventType;

- (MouseEnterState)mouseEnterState;

- (NSTrackingRectTag)trackingRect;

- (void)handleMouseMoved:(NSEvent*)aEvent;

- (void)sendMouseEnterOrExitEvent:(NSEvent*)aEvent
                            enter:(BOOL)aEnter
                             type:(nsMouseEvent::exitType)aType;

#ifndef NP_NO_CARBON
- (void) processPluginKeyEvent:(EventRef)aKeyEvent;
#endif











- (void)swipeWithEvent:(NSEvent *)anEvent;
- (void)beginGestureWithEvent:(NSEvent *)anEvent;
- (void)magnifyWithEvent:(NSEvent *)anEvent;
- (void)rotateWithEvent:(NSEvent *)anEvent;
- (void)endGestureWithEvent:(NSEvent *)anEvent;
@end



#ifndef NS_LEOPARD_AND_LATER






class nsTSMManager {
public:
  static PRBool IsComposing() { return sComposingView ? PR_TRUE : PR_FALSE; }
  static PRBool IsIMEEnabled() { return sIsIMEEnabled; }
  static PRBool IgnoreCommit() { return sIgnoreCommit; }

  static void OnDestroyView(NSView<mozView>* aDestroyingView);

  
  
  static PRBool IsRomanKeyboardsOnly() { return sIsRomanKeyboardsOnly; }

  static PRBool GetIMEOpenState();

  static void InitTSMDocument(NSView<mozView>* aViewForCaret);
  static void StartComposing(NSView<mozView>* aComposingView);
  static void UpdateComposing(NSString* aComposingString);
  static void EndComposing();
  static void EnableIME(PRBool aEnable);
  static void SetIMEOpenState(PRBool aOpen);
  static void SetRomanKeyboardsOnly(PRBool aRomanOnly);

  static void CommitIME();
  static void CancelIME();
private:
  static PRBool sIsIMEEnabled;
  static PRBool sIsRomanKeyboardsOnly;
  static PRBool sIgnoreCommit;
  static NSView<mozView>* sComposingView;
  static TSMDocumentID sDocumentID;
  static NSString* sComposingString;

  static void KillComposing();
};
#endif 

class ChildViewMouseTracker {

public:

  static void MouseMoved(NSEvent* aEvent);
  static void OnDestroyView(ChildView* aView);
  static BOOL WindowAcceptsEvent(NSWindow* aWindow, NSEvent* aEvent);

  static ChildView* sLastMouseEventView;

private:

  static NSWindow* WindowForEvent(NSEvent* aEvent);
  static ChildView* ViewForEvent(NSEvent* aEvent);
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
  virtual void            Scroll(const nsIntPoint& aDelta,
                                 const nsTArray<nsIntRect>& aDestRects,
                                 const nsTArray<Configuration>& aConfigurations);
  virtual nsIntPoint      WidgetToScreenOffset();
  virtual PRBool          ShowsResizeIndicator(nsIntRect* aResizerRect);

  static  PRBool          ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);

  NS_IMETHOD              Update();

  NS_IMETHOD        SetCursor(nsCursor aCursor);
  NS_IMETHOD        SetCursor(imgIContainer* aCursor, PRUint32 aHotspotX, PRUint32 aHotspotY);
  
  NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD        SetTitle(const nsAString& title);

  NS_IMETHOD        GetAttention(PRInt32 aCycleCount);

  virtual PRBool HasPendingInputEvent();

  NS_IMETHOD        ActivateNativeMenuItemAt(const nsAString& indexString);
  NS_IMETHOD        ForceUpdateNativeMenuAt(const nsAString& indexString);

  NS_IMETHOD        ResetInputState();
  NS_IMETHOD        SetIMEOpenState(PRBool aState);
  NS_IMETHOD        GetIMEOpenState(PRBool* aState);
  NS_IMETHOD        SetIMEEnabled(PRUint32 aState);
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
  void              GetDocumentAccessible(nsIAccessible** aAccessible);
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

  static void ApplyConfiguration(nsIWidget* aExpectedParent,
                                 const nsIWidget::Configuration& aConfiguration,
                                 PRBool aRepaint);

#ifdef NS_LEOPARD_AND_LATER
  nsCocoaTextInputHandler* TextInputHandler() { return &mTextInputHandler; }
  NSView<mozView>* GetEditorView();
#endif

  
  void IME_OnDestroyView(NSView<mozView> *aDestroyingView)
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.OnDestroyView(aDestroyingView);
#else
    nsTSMManager::OnDestroyView(aDestroyingView);
#endif
  }

  void IME_OnStartComposition(NSView<mozView>* aComposingView)
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.OnStartIMEComposition(aComposingView);
#else
    nsTSMManager::StartComposing(aComposingView);
#endif
  }

  void IME_OnUpdateComposition(NSString* aCompositionString)
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.OnUpdateIMEComposition(aCompositionString);
#else
    nsTSMManager::UpdateComposing(aCompositionString);
#endif
  }

  void IME_OnEndComposition()
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.OnEndIMEComposition();
#else
    nsTSMManager::EndComposing();
#endif
  }

  PRBool IME_IsComposing()
  {
#ifdef NS_LEOPARD_AND_LATER
    return mTextInputHandler.IsIMEComposing();
#else
    return nsTSMManager::IsComposing();
#endif
  }

  PRBool IME_IsASCIICapableOnly()
  {
#ifdef NS_LEOPARD_AND_LATER
    return mTextInputHandler.IsASCIICapableOnly();
#else
    return nsTSMManager::IsRomanKeyboardsOnly();
#endif
  }

  PRBool IME_IsOpened()
  {
#ifdef NS_LEOPARD_AND_LATER
    return mTextInputHandler.IsIMEOpened();
#else
    return nsTSMManager::GetIMEOpenState();
#endif
  }

  PRBool IME_IsEnabled()
  {
#ifdef NS_LEOPARD_AND_LATER
    return mTextInputHandler.IsIMEEnabled();
#else
    return nsTSMManager::IsIMEEnabled();
#endif
  }

  PRBool IME_IgnoreCommit()
  {
#ifdef NS_LEOPARD_AND_LATER
    return mTextInputHandler.IgnoreIMECommit();
#else
    return nsTSMManager::IgnoreCommit();
#endif
  }

  void IME_CommitComposition()
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.CommitIMEComposition();
#else
    nsTSMManager::CommitIME();
#endif
  }

  void IME_CancelComposition()
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.CancelIMEComposition();
#else
    nsTSMManager::CancelIME();
#endif
  }

  void IME_SetASCIICapableOnly(PRBool aASCIICapableOnly)
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.SetASCIICapableOnly(aASCIICapableOnly);
#else
    nsTSMManager::SetRomanKeyboardsOnly(aASCIICapableOnly);
#endif
  }

  void IME_SetOpenState(PRBool aOpen)
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.SetIMEOpenState(aOpen);
#else
    nsTSMManager::SetIMEOpenState(aOpen);
#endif
  }

  void IME_Enable(PRBool aEnable)
  {
#ifdef NS_LEOPARD_AND_LATER
    mTextInputHandler.EnableIME(aEnable);
#else
    nsTSMManager::EnableIME(aEnable);
#endif
  }

protected:

  PRBool            ReportDestroyEvent();
  PRBool            ReportMoveEvent();
  PRBool            ReportSizeEvent();

  
  
  virtual NSView*   CreateCocoaView(NSRect inFrame);
  void              TearDownView();
  nsCocoaWindow*    GetXULWindowWidget();

protected:

  NSView<mozView>*      mView;      
#ifdef NS_LEOPARD_AND_LATER
  nsCocoaTextInputHandler mTextInputHandler;
#endif

  NSView<mozView>*      mParentView;
  nsIWidget*            mParentWidget;

#ifdef ACCESSIBILITY
  
  
  nsWeakPtr             mAccessible;
#endif

  nsRefPtr<gfxASurface> mTempThebesSurface;

  PRPackedBool          mVisible;
  PRPackedBool          mDrawing;
  PRPackedBool          mPluginDrawing;
  PRPackedBool          mPluginIsCG; 

  NP_CGContext          mPluginCGContext;
  NP_Port               mPluginQDPort;
  nsIPluginInstanceOwner* mPluginInstanceOwner; 

  static PRUint32 sLastInputEventCount;
};

void NS_InstallPluginKeyEventsHandler();
void NS_RemovePluginKeyEventsHandler();

#endif 






































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

#include "nsIWidget.h"
#include "nsIAppShell.h"

#include "nsIEventListener.h"
#include "nsString.h"
#include "nsIDragService.h"

#include "nsplugindefs.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

class gfxASurface;
class nsChildView;
union nsPluginPort;

enum {
  
  
  
  
  kFocusedChildViewTSMDocPropertyTag  = 'GKFV', 
};












extern "C" long TSMProcessRawKeyEvent(EventRef carbonEvent);

@interface NSEvent (Undocumented)




- (EventRef)_eventRef;

@end

@interface ChildView : NSView<
#ifdef ACCESSIBILITY
                              mozAccessible,
#endif
                              mozView, NSTextInput>
{
@private
  NSWindow* mWindow; 
  
  
  
  nsChildView* mGeckoChild;
    
  
  NSTrackingRectTag mMouseEnterExitTag;

  
  BOOL mIsPluginView;

  
  
  
  
  
  NSEvent* mCurKeyEvent;
  PRBool mKeyDownHandled;
  
  
  
  BOOL mKeyPressSent;
  
  PRBool mKeyPressHandled;

  
  NSRange mMarkedRange;

  BOOL mInHandScroll; 
  
  NSPoint mHandScrollStartMouseLoc;
  nscoord mHandScrollStartScrollX, mHandScrollStartScrollY;
  
  
  NSEvent* mLastMouseDownEvent;
  
  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;

  
  
  BOOL mIsTransparent;

  
  
  
  
  
  nsIDragService* mDragService;
  
  PRUint32 mLastModifierState;

  
  
  
  TSMDocumentID mPluginTSMDoc;
}


- (void)viewsWindowDidBecomeKey;
- (void)viewsWindowDidResignKey;


- (void)delayedTearDown;

- (void)setTransparent:(BOOL)transparent;

- (void)sendFocusEvent:(PRUint32)eventType;

- (void) processPluginKeyEvent:(EventRef)aKeyEvent;
@end









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
                                 const nsRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsIDeviceContext *aContext,
                                 nsIAppShell *aAppShell = nsnull,
                                 nsIToolkit *aToolkit = nsnull,
                                 nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD              Create(nsNativeWidget aNativeParent,
                                 const nsRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsIDeviceContext *aContext,
                                 nsIAppShell *aAppShell = nsnull,
                                 nsIToolkit *aToolkit = nsnull,
                                 nsWidgetInitData *aInitData = nsnull);

   
   

  virtual nsresult        StandardCreate(nsIWidget *aParent,
                              const nsRect &aRect,
                              EVENT_CALLBACK aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell *aAppShell,
                              nsIToolkit *aToolkit,
                              nsWidgetInitData *aInitData,
                              nsNativeWidget aNativeParent = nsnull);

  NS_IMETHOD              Destroy();

  NS_IMETHOD              Show(PRBool aState);
  NS_IMETHOD              IsVisible(PRBool& outState);

  virtual nsIWidget*      GetParent(void);
  nsIWidget*              GetTopLevelWidget();

  NS_IMETHOD              ModalEventFilter(PRBool aRealEvent, void *aEvent,
                                           PRBool *aForWindow);

  NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
                                            PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);

  NS_IMETHOD              Enable(PRBool aState);
  NS_IMETHOD              IsEnabled(PRBool *aState);
  NS_IMETHOD              SetFocus(PRBool aRaise);
  NS_IMETHOD              SetBounds(const nsRect &aRect);
  NS_IMETHOD              GetBounds(nsRect &aRect);

  NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD              Invalidate(const nsRect &aRect,PRBool aIsSynchronous);
  NS_IMETHOD              InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);
  NS_IMETHOD              Validate();

  virtual void*           GetNativeData(PRUint32 aDataType);
  NS_IMETHOD              SetColorMap(nsColorMap *aColorMap);
  NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
  NS_IMETHOD              WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD              ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD              BeginResizingChildren(void);
  NS_IMETHOD              EndResizingChildren(void);
  virtual PRBool          ShowsResizeIndicator(nsIntRect* aResizerRect);

  static  PRBool          ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);

  NS_IMETHOD              Update();

  virtual void      ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY);
  void              LocalToWindowCoordinate(nsPoint& aPoint)            { ConvertToDeviceCoordinates(aPoint.x, aPoint.y); }
  void              LocalToWindowCoordinate(nscoord& aX, nscoord& aY)   { ConvertToDeviceCoordinates(aX, aY); }
  void              LocalToWindowCoordinate(nsRect& aRect)              { ConvertToDeviceCoordinates(aRect.x, aRect.y); }

  NS_IMETHOD        SetMenuBar(void* aMenuBar);
  NS_IMETHOD        ShowMenuBar(PRBool aShow);

  NS_IMETHOD        GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
  NS_IMETHOD        SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);
  
  NS_IMETHOD        SetCursor(nsCursor aCursor);
  NS_IMETHOD        SetCursor(imgIContainer* aCursor, PRUint32 aHotspotX, PRUint32 aHotspotY);
  
  NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD        SetTitle(const nsAString& title);

  NS_IMETHOD        GetAttention(PRInt32 aCycleCount);

  NS_IMETHOD        ActivateNativeMenuItemAt(const nsAString& indexString);
  NS_IMETHOD        ForceNativeMenuReload();

  NS_IMETHOD        ResetInputState();
  NS_IMETHOD        SetIMEOpenState(PRBool aState);
  NS_IMETHOD        GetIMEOpenState(PRBool* aState);
  NS_IMETHOD        SetIMEEnabled(PRUint32 aState);
  NS_IMETHOD        GetIMEEnabled(PRUint32* aState);
  NS_IMETHOD        CancelIMEComposition();
  NS_IMETHOD        GetToggledKeyState(PRUint32 aKeyCode,
                                       PRBool* aLEDState);

  
  NS_IMETHOD        GetPluginClipRect(nsRect& outClipRect, nsPoint& outOrigin, PRBool& outWidgetVisible);
  NS_IMETHOD        StartDrawPlugin();
  NS_IMETHOD        EndDrawPlugin();
  NS_IMETHOD        SetPluginInstanceOwner(nsIPluginInstanceOwner* aInstanceOwner);
  
  virtual nsTransparencyMode GetTransparencyMode();
  virtual void                SetTransparencyMode(nsTransparencyMode aMode);
  
  
  virtual PRBool    PointInWidget(Point aThePoint);
  
  virtual PRBool    DispatchWindowEvent(nsGUIEvent& event);
  
  void              LiveResizeStarted();
  void              LiveResizeEnded();
  
#ifdef ACCESSIBILITY
  void              GetDocumentAccessible(nsIAccessible** aAccessible);
#endif

  virtual gfxASurface* GetThebesSurface();

  NS_IMETHOD BeginSecureKeyboardInput();
  NS_IMETHOD EndSecureKeyboardInput();

  void              HidePlugin();

protected:

  PRBool            ReportDestroyEvent();
  PRBool            ReportMoveEvent();
  PRBool            ReportSizeEvent();

  NS_IMETHOD        CalcOffset(PRInt32 &aX,PRInt32 &aY);

  virtual PRBool    OnPaint(nsPaintEvent & aEvent);

  
  
  virtual NSView*   CreateCocoaView(NSRect inFrame);
  void              TearDownView();

  virtual nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                            PRInt32 aNativeKeyCode,
                                            PRUint32 aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters);

protected:

  NSView<mozView>*      mView;      

  NSView<mozView>*      mParentView;
  nsIWidget*            mParentWidget;

#ifdef ACCESSIBILITY
  
  
  nsWeakPtr             mAccessible;
#endif

  nsRefPtr<gfxASurface> mTempThebesSurface;

  PRPackedBool          mVisible;
  PRPackedBool          mDrawing;
  PRPackedBool          mLiveResizeInProgress;
  PRPackedBool          mIsPluginView; 
  PRPackedBool          mPluginDrawing;
  PRPackedBool          mPluginIsCG; 

  PRPackedBool          mInSetFocus;

  nsPluginPort          mPluginPort;
  nsIPluginInstanceOwner* mPluginInstanceOwner; 
};

void NS_InstallPluginKeyEventsHandler();
void NS_RemovePluginKeyEventsHandler();

#endif 

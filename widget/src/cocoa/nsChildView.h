




































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
#include "nsIPluginWidget.h"
#include "nsIEventSink.h"
#include "nsIScrollableView.h"
#include "nsWeakPtr.h"

#include "nsIWidget.h"
#include "nsIKBStateControl.h"
#include "nsIAppShell.h"

#include "nsIMouseListener.h"
#include "nsIEventListener.h"
#include "nsString.h"
#include "nsIDragService.h"
#include "nsIMenuBar.h"

#include "nsplugindefs.h"
#import <Quickdraw.h>

class gfxASurface;

#define NSRGB_2_COLOREF(color) \
            RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))

union nsPluginPort;

#undef DARWIN
#import <Cocoa/Cocoa.h>

class nsChildView;

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
  PRBool  mKeyHandled;
  
  
  NSRange mMarkedRange;
  NSRange mSelectedRange;
  BOOL mInComposition;
  BOOL mIgnoreDoCommand;

  BOOL mInHandScroll; 
  
  NSPoint mHandScrollStartMouseLoc;
  nscoord mHandScrollStartScrollX, mHandScrollStartScrollY;
  
  
  NSEvent* mLastMenuForEventEvent;
  
  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;

  
  
  
  
  
  nsIDragService* mDragService;
  
  PRUint32 mLastModifierState;
}


- (void)viewsWindowDidBecomeKey;
- (void)viewsWindowDidResignKey;

- (BOOL)isComposing;
@end









class nsChildView : public nsBaseWidget,
                    public nsIPluginWidget,
                    public nsIKBStateControl,
                    public nsIEventSink
{
private:
  typedef nsBaseWidget Inherited;

public:
                          nsChildView();
  virtual                 ~nsChildView();
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIEVENTSINK 

  
  NS_IMETHOD              ResetInputState();
  NS_IMETHOD              SetIMEOpenState(PRBool aState);
  NS_IMETHOD              GetIMEOpenState(PRBool* aState);
  NS_IMETHOD              SetIMEEnabled(PRBool aState);
  NS_IMETHOD              GetIMEEnabled(PRBool* aState);
  NS_IMETHOD              CancelIMEComposition();
 
  
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
  
  NS_IMETHOD              ModalEventFilter(PRBool aRealEvent, void *aEvent,
                                           PRBool *aForWindow);

  NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
                                            PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              MoveWithRepaintOption(PRInt32 aX, PRInt32 aY, PRBool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);

  NS_IMETHOD              Enable(PRBool aState);
  NS_IMETHOD              IsEnabled(PRBool *aState);
  NS_IMETHOD              SetFocus(PRBool aRaise);
  NS_IMETHOD              SetBounds(const nsRect &aRect);
  NS_IMETHOD              GetBounds(nsRect &aRect);

  virtual nsIFontMetrics* GetFont(void);
  NS_IMETHOD              SetFont(const nsFont &aFont);
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

  static  PRBool          ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
  virtual PRBool          DispatchMouseEvent(nsMouseEvent &aEvent);

  NS_IMETHOD              Update();

  virtual void      ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY);
  void              LocalToWindowCoordinate(nsPoint& aPoint)            { ConvertToDeviceCoordinates(aPoint.x, aPoint.y); }
  void              LocalToWindowCoordinate(nscoord& aX, nscoord& aY)   { ConvertToDeviceCoordinates(aX, aY); }
  void              LocalToWindowCoordinate(nsRect& aRect)              { ConvertToDeviceCoordinates(aRect.x, aRect.y); }

  NS_IMETHOD        SetMenuBar(nsIMenuBar * aMenuBar);
  NS_IMETHOD        ShowMenuBar(PRBool aShow);
  virtual nsIMenuBar*   GetMenuBar();
  NS_IMETHOD        GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
  NS_IMETHOD        SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);
  
  NS_IMETHOD        SetCursor(nsCursor aCursor);
  NS_IMETHOD        SetCursor(imgIContainer* aCursor, PRUint32 aHotspotX, PRUint32 aHotspotY);
  
  NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD        SetTitle(const nsAString& title);

  NS_IMETHOD        GetAttention(PRInt32 aCycleCount);

  NS_IMETHOD        SetAnimatedResize(PRUint16 aAnimation);
  NS_IMETHOD        GetAnimatedResize(PRUint16* aAnimation);

  
  NS_IMETHOD        GetPluginClipRect(nsRect& outClipRect, nsPoint& outOrigin, PRBool& outWidgetVisible);
  NS_IMETHOD        StartDrawPlugin();
  NS_IMETHOD        EndDrawPlugin();
  
  
  virtual void      CalcWindowRegions();

  virtual PRBool    PointInWidget(Point aThePoint);
  
  virtual PRBool    DispatchWindowEvent(nsGUIEvent& event);
  virtual PRBool    DispatchWindowEvent(nsGUIEvent &event,nsEventStatus &aStatus);
  virtual void      AcceptFocusOnClick(PRBool aBool) { mAcceptFocusOnClick = aBool;};
  PRBool            AcceptFocusOnClick() { return mAcceptFocusOnClick;};
  
  void              LiveResizeStarted();
  void              LiveResizeEnded();
  
#ifdef ACCESSIBILITY
  void              GetDocumentAccessible(nsIAccessible** aAccessible);
#endif

  virtual gfxASurface* GetThebesSurface();

protected:

  PRBool            ReportDestroyEvent();
  PRBool            ReportMoveEvent();
  PRBool            ReportSizeEvent();

  NS_IMETHOD        CalcOffset(PRInt32 &aX,PRInt32 &aY);

  virtual PRBool    OnPaint(nsPaintEvent & aEvent);

  
  
  virtual NSView*   CreateCocoaView(NSRect inFrame);
  void              TearDownView();

  
  GrafPtr           GetChildViewQuickDrawPort();

protected:

  NSView<mozView>*      mView;      

  NSView<mozView>*      mParentView;
  nsIWidget*            mParentWidget;

#ifdef ACCESSIBILITY
  
  
  nsWeakPtr             mAccessible;
#endif

  nsRefPtr<gfxASurface> mTempThebesSurface;

  PRPackedBool          mDestructorCalled;
  PRPackedBool          mVisible;

  PRPackedBool          mDrawing;
    
  PRPackedBool          mAcceptFocusOnClick;
  PRPackedBool          mLiveResizeInProgress;
  PRPackedBool          mIsPluginView; 
  PRPackedBool          mPluginDrawing;
  PRPackedBool          mPluginIsCG; 
  
  nsPluginPort          mPluginPort;
  RgnHandle             mVisRgn;
};


#endif 

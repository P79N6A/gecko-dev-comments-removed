



































#ifndef Window_h__
#define Window_h__

#include "nsISupports.h"
#include "nsIPluginWidget.h"
#include "nsBaseWidget.h"
#include "nsDeleteObserver.h"

#include "nsIWidget.h"
#include "nsIKBStateControl.h"
#include "nsIAppShell.h"

#include "nsIMouseListener.h"
#include "nsIEventListener.h"
#include "nsString.h"

#include "nsIMenuBar.h"

#include "nsplugindefs.h"
#include <Quickdraw.h>

#define NSRGB_2_COLOREF(color) \
            RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))

struct nsPluginPort;
struct TRectArray;


class CursorSpinner {
public:
    CursorSpinner();
    ~CursorSpinner();
    void StartSpinCursor();
    void StopSpinCursor();    

private:
    short                mSpinCursorFrame;
    EventLoopTimerUPP    mTimerUPP;
    EventLoopTimerRef    mTimerRef;
    
    short                GetNextCursorFrame();
    static pascal void   SpinCursor(EventLoopTimerRef inTimer, void *inUserData);
};







class nsWindow :    public nsBaseWidget,
                    public nsDeleteObserved,
                    public nsIKBStateControl,
                    public nsIPluginWidget
{
private:
  typedef nsBaseWidget Inherited;

public:
                            nsWindow();
    virtual                 ~nsWindow();
    
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
    virtual nsIWidget*      GetParent(void);

    NS_IMETHOD              Show(PRBool aState);
    NS_IMETHOD              IsVisible(PRBool & aState);

    NS_IMETHOD              ModalEventFilter(PRBool aRealEvent, void *aEvent,
                                             PRBool *aForWindow);

    NS_IMETHOD                  ConstrainPosition(PRBool aAllowSlop,
                                                  PRInt32 *aX, PRInt32 *aY);
    NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
    NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
    NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);

    NS_IMETHOD              Enable(PRBool aState);
    NS_IMETHOD              IsEnabled(PRBool *aState);
    NS_IMETHOD              SetFocus(PRBool aRaise);
    NS_IMETHOD              SetBounds(const nsRect &aRect);
    NS_IMETHOD              GetBounds(nsRect &aRect);

    virtual nsIFontMetrics* GetFont(void);
    NS_IMETHOD              SetFont(const nsFont &aFont);
    NS_IMETHOD              Validate();
    NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
    NS_IMETHOD              Invalidate(const nsRect &aRect,PRBool aIsSynchronous);
    NS_IMETHOD              InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);

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

    virtual void          StartDraw(nsIRenderingContext* aRenderingContext = nsnull);
    virtual void          EndDraw();
    PRBool                IsDrawing() const { return mDrawing; }
    
    NS_IMETHOD            Update();
    virtual void          UpdateWidget(nsRect& aRect, nsIRenderingContext* aContext);
    
    virtual void          ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY);
    void                  LocalToWindowCoordinate(nsPoint& aPoint)            { ConvertToDeviceCoordinates(aPoint.x, aPoint.y); }
    void                  LocalToWindowCoordinate(nscoord& aX, nscoord& aY)       { ConvertToDeviceCoordinates(aX, aY); }
    void                  LocalToWindowCoordinate(nsRect& aRect)              { ConvertToDeviceCoordinates(aRect.x, aRect.y); }

    NS_IMETHOD            SetMenuBar(nsIMenuBar * aMenuBar);
    NS_IMETHOD            ShowMenuBar(PRBool aShow);
    virtual nsIMenuBar*   GetMenuBar();
    NS_IMETHOD        GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
    NS_IMETHOD        SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);
    
    NS_IMETHOD        SetCursor(nsCursor aCursor);
    static void       SetCursorResource(short aCursorResourceNum);

    NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
    NS_IMETHOD        SetTitle(const nsAString& title);
  
    NS_IMETHOD        GetAttention(PRInt32 aCycleCount);

    
    static void         nsRectToMacRect(const nsRect& aRect, Rect& aMacRect);
    PRBool              RgnIntersects(RgnHandle aTheRegion,RgnHandle aIntersectRgn);
    virtual void        CalcWindowRegions();

    virtual PRBool      PointInWidget(Point aThePoint);
    virtual nsWindow*   FindWidgetHit(Point aThePoint);

    virtual PRBool      DispatchWindowEvent(nsGUIEvent& event);
    virtual PRBool      DispatchWindowEvent(nsGUIEvent &event,nsEventStatus &aStatus);
    virtual nsresult    HandleUpdateEvent(RgnHandle regionToValidate);
    virtual void        AcceptFocusOnClick(PRBool aBool) { mAcceptFocusOnClick = aBool;};
    PRBool              AcceptFocusOnClick() { return mAcceptFocusOnClick;};
    void                Flash(nsPaintEvent  &aEvent);
    PRBool              IsTopLevelWidgetWindow() const { return mIsTopWidgetWindow; };

    

    NS_IMETHOD              GetPluginClipRect(nsRect& outClipRect, nsPoint& outOrigin, PRBool& outWidgetVisible);
    NS_IMETHOD              StartDrawPlugin(void);
    NS_IMETHOD              EndDrawPlugin(void);

    
    NS_IMETHOD ResetInputState();
    NS_IMETHOD SetIMEOpenState(PRBool aState);
    NS_IMETHOD GetIMEOpenState(PRBool* aState);
    NS_IMETHOD SetIMEEnabled(PRUint32 aState);
    NS_IMETHOD GetIMEEnabled(PRUint32* aState);
    NS_IMETHOD CancelIMEComposition();
    NS_IMETHOD GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState);

protected:

  PRBool          ReportDestroyEvent();
  PRBool          ReportMoveEvent();
  PRBool          ReportSizeEvent();

  void            CalcOffset(PRInt32 &aX,PRInt32 &aY);
  PRBool          ContainerHierarchyIsVisible();
  
  virtual PRBool      OnPaint(nsPaintEvent & aEvent);

  
  
  void          ScrollBits ( Rect & foo, PRInt32 inLeftDelta, PRInt32 inTopDelta ) ;

  void          CombineRects ( TRectArray & inRectArray ) ;
  void          SortRectsLeftToRight ( TRectArray & inRectArray ) ;

protected:
#if DEBUG
  const char*       gInstanceClassName;
#endif

  nsIWidget*        mParent;
  PRPackedBool      mIsTopWidgetWindow;
  PRPackedBool      mResizingChildren;
  PRPackedBool      mSaveVisible;
  PRPackedBool      mVisible;
  PRPackedBool      mEnabled;
  PRInt32           mPreferredWidth;
  PRInt32           mPreferredHeight;
  nsIFontMetrics*   mFontMetrics;
  nsIMenuBar*       mMenuBar;

  RgnHandle         mWindowRegion;
  RgnHandle         mVisRegion;
  WindowPtr         mWindowPtr;

  PRPackedBool      mDestructorCalled;

  PRPackedBool      mAcceptFocusOnClick;

  PRPackedBool      mDrawing;
  PRPackedBool      mInUpdate;    
  PRPackedBool      mTempRenderingContextMadeHere;

  nsIRenderingContext*    mTempRenderingContext;
    
  nsPluginPort*     mPluginPort;

  
  
  static OSStatus AddRectToArrayProc(UInt16 message, RgnHandle rgn,
                                     const Rect* rect, void* refCon);
  static void PaintUpdateRect(Rect* r, void* data);

};


#if DEBUG
#define WIDGET_SET_CLASSNAME(n)   gInstanceClassName = (n)
#else
#define WIDGET_SET_CLASSNAME(n)   
#endif











#include "nsChildWindow.h"

#define ChildWindow   nsChildWindow



#endif 

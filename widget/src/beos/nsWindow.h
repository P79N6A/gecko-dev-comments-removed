






































#ifndef Window_h__
#define Window_h__
#define BeIME

#include "nsBaseWidget.h"
#include "nsdefs.h"
#include "nsSwitchToUIThread.h"
#include "nsToolkit.h"

#include "nsIWidget.h"

#include "nsString.h"
#include "nsRegion.h"

#include <Window.h>
#include <View.h>
#include <Region.h>

#if defined(BeIME)
#include <Messenger.h>
#endif

#include <gfxBeOSSurface.h>

#define NSRGB_2_COLOREF(color) \
            RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))


class nsViewBeOS;
class nsIRollupListener;
#if defined(BeIME)
class nsIMEBeOS;
#endif





class nsWindow : public nsSwitchToUIThread,
                 public nsBaseWidget
{

public:
	nsWindow();
	virtual ~nsWindow();

	
	
	
	
	
	
	NS_IMETHOD_(nsrefcnt) AddRef(void);                                       
	NS_IMETHOD_(nsrefcnt) Release(void);          

	
	NS_IMETHOD              Create(nsIWidget *aParent,
	                               const nsRect &aRect,
	                               EVENT_CALLBACK aHandleEventFunction,
	                               nsIDeviceContext *aContext,
	                               nsIAppShell *aAppShell = nsnull,
	                               nsIToolkit *aToolkit = nsnull,
	                               nsWidgetInitData *aInitData = nsnull);
	NS_IMETHOD              Create(nsNativeWidget aParent,
	                               const nsRect &aRect,
	                               EVENT_CALLBACK aHandleEventFunction,
	                               nsIDeviceContext *aContext,
	                               nsIAppShell *aAppShell = nsnull,
	                               nsIToolkit *aToolkit = nsnull,
	                               nsWidgetInitData *aInitData = nsnull);

	
	

	NS_IMETHOD          PreCreateWidget(nsWidgetInitData *aWidgetInitData);

	virtual nsresult        StandardWindowCreate(nsIWidget *aParent,
	                                             const nsRect &aRect,
	                                             EVENT_CALLBACK aHandleEventFunction,
	                                             nsIDeviceContext *aContext,
	                                             nsIAppShell *aAppShell,
	                                             nsIToolkit *aToolkit,
	                                             nsWidgetInitData *aInitData,
	                                             nsNativeWidget aNativeParent = nsnull);

	gfxASurface*            GetThebesSurface();

	NS_IMETHOD              Destroy();
	virtual nsIWidget*        GetParent(void);
	NS_IMETHOD              Show(PRBool bState);
 	NS_IMETHOD              CaptureMouse(PRBool aCapture);
	NS_IMETHOD              CaptureRollupEvents(nsIRollupListener *aListener,
	                                            PRBool aDoCapture,
	                                            PRBool aConsumeRollupEvent);
	NS_IMETHOD              IsVisible(PRBool & aState);

	NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
	                                          PRInt32 *aX, PRInt32 *aY);
	NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
	NS_IMETHOD              Resize(PRInt32 aWidth,
	                               PRInt32 aHeight,
	                               PRBool   aRepaint);
	NS_IMETHOD              Resize(PRInt32 aX,
	                               PRInt32 aY,
	                               PRInt32 aWidth,
	                               PRInt32 aHeight,
	                               PRBool   aRepaint);
	NS_IMETHOD              SetModal(PRBool aModal);
	NS_IMETHOD              Enable(PRBool aState);
	NS_IMETHOD              IsEnabled(PRBool *aState);
	NS_IMETHOD              SetFocus(PRBool aRaise);
	NS_IMETHOD              GetScreenBounds(nsRect &aRect);
	NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
	NS_IMETHOD              SetCursor(nsCursor aCursor);
	NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
	NS_IMETHOD              Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
	NS_IMETHOD              InvalidateRegion(const nsIRegion *aRegion,
	                                         PRBool aIsSynchronous);
	NS_IMETHOD              Update();
	virtual void*           GetNativeData(PRUint32 aDataType);
	NS_IMETHOD              SetColorMap(nsColorMap *aColorMap);
	NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
	NS_IMETHOD              SetTitle(const nsAString& aTitle);
	NS_IMETHOD              SetMenuBar(void * aMenuBar) { return NS_ERROR_FAILURE; }
	NS_IMETHOD              ShowMenuBar(PRBool aShow) { return NS_ERROR_FAILURE; }
	NS_IMETHOD              WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
	NS_IMETHOD              ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
	NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
	NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);

	virtual void            ConvertToDeviceCoordinates(nscoord	&aX,nscoord	&aY) {}


	
	virtual bool            CallMethod(MethodInfo *info);
	virtual PRBool          DispatchMouseEvent(PRUint32 aEventType, 
	                                           nsPoint aPoint, 
	                                           PRUint32 clicks, 
	                                           PRUint32 mod,
	                                           PRUint16 aButton = nsMouseEvent::eLeftButton);


	void                   InitEvent(nsGUIEvent& event, nsPoint* aPoint = nsnull);

protected:

	static PRBool           EventIsInsideWindow(nsWindow* aWindow, nsPoint pos) ;
	static PRBool           DealWithPopups(uint32 methodID, nsPoint pos);
    
	void                    OnDestroy();
	void                    OnWheel(PRInt32 aDirection, uint32 aButtons, BPoint aPoint, nscoord aDelta);
	PRBool                  OnMove(PRInt32 aX, PRInt32 aY);
	nsresult                OnPaint(BRegion *breg);
	PRBool                  OnResize(nsRect &aWindowRect);
	PRBool                  OnKeyDown(PRUint32 aEventType, 
	                                  const char *bytes, 
	                                  int32 numBytes, 
	                                  PRUint32 mod, 
	                                  PRUint32 bekeycode, 
	                                  int32 rawcode);
	PRBool                  OnKeyUp(PRUint32 aEventType, 
	                                const char *bytes, 
	                                int32 numBytes, 
	                                PRUint32 mod, 
	                                PRUint32 bekeycode, 
	                                int32 rawcode);
	PRBool                  DispatchKeyEvent(PRUint32 aEventType, PRUint32 aCharCode,
                                           PRUint32 aKeyCode, PRUint32 aFlags = 0);
	PRBool                  DispatchFocus(PRUint32 aEventType);
	static PRBool           ConvertStatus(nsEventStatus aStatus)
	                        { return aStatus == nsEventStatus_eConsumeNoDefault; }
	PRBool                  DispatchStandardEvent(PRUint32 aMsg);

	PRBool                  DispatchWindowEvent(nsGUIEvent* event);
	void                    HideKids(PRBool state);


	nsCOMPtr<nsIWidget> mParent;
	nsWindow*        mWindowParent;
	nsCOMPtr<nsIRegion> mUpdateArea;
	nsIFontMetrics*  mFontMetrics;

	nsViewBeOS*      mView;
	window_feel      mBWindowFeel;
	window_look      mBWindowLook;

	nsRefPtr<gfxBeOSSurface> mThebesSurface;

	
	PRPackedBool           mIsTopWidgetWindow;
	PRPackedBool           mIsMetaDown;
	PRPackedBool           mIsShiftDown;
	PRPackedBool           mIsControlDown;
	PRPackedBool           mIsAltDown;
	PRPackedBool           mIsDestroying;
	PRPackedBool           mIsVisible;
	PRPackedBool           mEnabled;
	PRPackedBool           mIsScrolling;
	PRPackedBool           mListenForResizes;
	
public:	

	nsToolkit *GetToolkit() { return (nsToolkit *)nsBaseWidget::GetToolkit(); }
};




class nsIWidgetStore
{
public:
	                        nsIWidgetStore(nsIWidget *aWindow);
	virtual                ~nsIWidgetStore();

	virtual nsIWidget      *GetMozillaWidget(void);

private:
	nsIWidget       *mWidget;
};




class nsWindowBeOS : public BWindow, public nsIWidgetStore
{
public:
	                        nsWindowBeOS(nsIWidget *aWidgetWindow,  
	                                     BRect aFrame, 
	                                     const char *aName, 
	                                     window_look aLook,
	                                     window_feel aFeel, 
	                                     int32 aFlags, 
	                                     int32 aWorkspace = B_CURRENT_WORKSPACE);
	virtual                ~nsWindowBeOS();

	virtual bool            QuitRequested( void );
	virtual void            MessageReceived(BMessage *msg);
	virtual void            DispatchMessage(BMessage *msg, BHandler *handler);
	virtual void            WindowActivated(bool active);
	virtual void            FrameMoved(BPoint origin);
	virtual void            WorkspacesChanged(uint32 oldworkspace, uint32 newworkspace);
	virtual void            FrameResized(float width, float height);
	bool                    fJustGotBounds;	
private:
	BPoint          lastWindowPoint;
};




class nsViewBeOS : public BView, public nsIWidgetStore
{
public:
	                        nsViewBeOS(nsIWidget *aWidgetWindow, 
	                                   BRect aFrame, 
	                                   const char *aName,
	                                   uint32 aResizingMode, 
	                                   uint32 aFlags);

	virtual void            Draw(BRect updateRect);
	virtual void            MouseDown(BPoint point);
	virtual void            MouseMoved(BPoint point, 
	                                   uint32 transit, 
	                                   const BMessage *message);
	virtual void            MouseUp(BPoint point);
	bool                    GetPaintRegion(BRegion *breg);
	void                    Validate(BRegion *reg);
	BPoint                  GetWheel();
	void                    KeyDown(const char *bytes, int32 numBytes);
	void                    KeyUp(const char *bytes, int32 numBytes);
	virtual void            MakeFocus(bool focused);
	virtual void            MessageReceived(BMessage *msg);
	void                    SetVisible(bool visible);
	bool                    Visible();
	BRegion                 paintregion;
	uint32                  buttons;

private:
#if defined(BeIME)
 	void                 DoIME(BMessage *msg);
#endif
	BPoint               mousePos;
	uint32               mouseMask;
	
	BPoint               wheel;
	bool                 fRestoreMouseMask;	
	bool                 fJustValidated;
	bool                 fWheelDispatched;
	bool                 fVisible;
};

#if defined(BeIME)
class nsIMEBeOS 
{
public:
	nsIMEBeOS();

	void	RunIME(uint32 *args, nsWindow *owner, BView* view);
	void	DispatchText(nsString &text, PRUint32 txtCount, nsTextRange* txtRuns);
	void	DispatchIME(PRUint32 what);
	void	DispatchCancelIME();
	PRBool	DispatchWindowEvent(nsGUIEvent* event);
	
	static  nsIMEBeOS *GetIME();

private:
	nsWindow*	imeTarget;
	BMessenger	imeMessenger;
	nsString	imeText;
	BPoint		imeCaret;
	PRUint32	imeState, imeWidth, imeHeight;
	static	    nsIMEBeOS *beosIME;
};
#endif
#endif 

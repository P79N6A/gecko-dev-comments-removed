



































#ifndef MacWindow_h__
#define MacWindow_h__

#include <memory>	
#include <Carbon/Carbon.h>

using std::auto_ptr;

#include "nsRegionPool.h"
#include "nsWindow.h"
#include "nsMacEventHandler.h"
#include "nsIEventSink.h"
#include "nsIMacTextInputEventSink.h"
#include "nsPIWidgetMac.h"
#include "nsPIEventSinkStandalone.h"

class nsMacEventHandler;








class nsMacWindow : public nsChildWindow, public nsIEventSink, public nsPIWidgetMac, 
                    public nsPIEventSinkStandalone, public nsIMacTextInputEventSink
{
private:
	typedef nsChildWindow Inherited;

public:
    nsMacWindow();
    virtual ~nsMacWindow();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIEVENTSINK 
    NS_DECL_NSPIWIDGETMAC
    NS_DECL_NSPIEVENTSINKSTANDALONE
    NS_DECL_NSIMACTEXTINPUTEVENTSINK
    










    NS_IMETHOD              Create(nsNativeWidget aParent,
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

    NS_IMETHOD              Show(PRBool aState);
    NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
                                              PRInt32 *aX, PRInt32 *aY);
    NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
    NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, PRBool aActivate);
    NS_IMETHOD              SetSizeMode(PRInt32 aMode);

    NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
    virtual nsresult        Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint, PRBool aFromUI);
    NS_IMETHOD            	GetScreenBounds(nsRect &aRect);
    virtual PRBool          OnPaint(nsPaintEvent &event);

    NS_IMETHOD              SetTitle(const nsAString& aTitle);

    void                    UserStateForResize();

  	
  	NS_IMETHOD ResetInputState();

    void              		MoveToGlobalPoint(PRInt32 aX, PRInt32 aY);

    void IsActive(PRBool* aActive);
    void SetIsActive(PRBool aActive);
    WindowPtr GetWindowTop(WindowPtr baseWindowRef);
    void UpdateWindowMenubar(WindowPtr parentWindow, PRBool enableFlag);

    NS_IMETHOD              Update();

protected:
  
	pascal static OSErr DragTrackingHandler ( DragTrackingMessage theMessage, WindowPtr theWindow, 
										void *handlerRefCon, DragReference theDrag );
	pascal static OSErr DragReceiveHandler (WindowPtr theWindow,
												void *handlerRefCon, DragReference theDragRef) ;
	DragTrackingHandlerUPP mDragTrackingHandlerUPP;
	DragReceiveHandlerUPP mDragReceiveHandlerUPP;


  pascal static OSStatus WindowEventHandler ( EventHandlerCallRef inHandlerChain, 
                                               EventRef inEvent, void* userData ) ;
  pascal static OSStatus ScrollEventHandler(EventHandlerCallRef aHandlerCallRef,
                                            EventRef            aEvent,
                                            void*               aUserData);
  pascal static OSStatus KeyEventHandler(EventHandlerCallRef aHandlerCallRef,
                                         EventRef            aEvent,
                                         void*               aUserData);
  nsresult GetDesktopRect(Rect* desktopRect);

	PRPackedBool                    mWindowMadeHere; 
	PRPackedBool                    mIsSheet;        
	PRPackedBool                    mAcceptsActivation;
	PRPackedBool                    mIsActive;
	PRPackedBool                    mZoomOnShow;
	PRPackedBool                    mZooming;
	PRPackedBool                    mResizeIsFromUs;    
	PRPackedBool                    mShown;             
	PRPackedBool                    mSheetNeedsShow;    
	PRPackedBool			mInPixelMouseScroll;
	Point                           mBoundsOffset;      
	auto_ptr<nsMacEventHandler>     mMacEventHandler;
	nsIWidget                      *mOffsetParent;
        nsCOMPtr<nsIWidget>             mDeathGripDuringTransition;
	
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
	PRPackedBool                    mNeedsResize;
	struct {
	  PRInt32      width;
	  PRInt32      height;
	  PRPackedBool repaint;
	  PRPackedBool fromUI;
	}                               mResizeTo;
#endif
};

#endif 






































#ifndef nsWindow_h__
#define nsWindow_h__



#include "nsISupports.h"

#include "nsWidget.h"

#include "nsString.h"

#include <Pt.h>

class nsFont;
class nsIAppShell;

#define NSRGB_2_COLOREF(color)				RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))





class nsWindow : public nsWidget
{

public:
  

  nsWindow();
  virtual ~nsWindow();

  NS_IMETHOD           WidgetToScreen(const nsRect &aOldRect, nsRect &aNewRect);

  virtual void*        GetNativeData(PRUint32 aDataType);

  NS_IMETHOD           Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
  NS_IMETHOD           ScrollWidgets(PRInt32 aDx, PRInt32 aDy);
  inline NS_IMETHOD    ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy)
		{
		NS_WARNING("nsWindow::ScrollRect Not Implemented\n");
		return NS_OK;
		}

  NS_IMETHOD           SetTitle(const nsAString& aTitle);
 
  NS_IMETHOD           Move(PRInt32 aX, PRInt32 aY);

  NS_IMETHOD           Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD           Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
		{
		Move(aX,aY);
		Resize(aWidth,aHeight,aRepaint);
		return NS_OK;
		}

  NS_IMETHOD           CaptureRollupEvents(nsIRollupListener * aListener,
                                           PRBool aDoCapture,
                                           PRBool aConsumeRollupEvent);

	NS_IMETHOD SetFocus(PRBool aRaise);

  inline NS_IMETHOD    GetAttention(PRInt32 aCycleCount)
		{
		if( mWidget ) PtWindowToFront( mWidget );
		return NS_OK;
		}

  virtual PRBool       IsChild() const;


  
  inline PRBool         OnKey(nsKeyEvent &aEvent)
		{
		if( mEventCallback ) return DispatchWindowEvent(&aEvent);
		return PR_FALSE;
		}

  inline NS_IMETHOD			GetClientBounds( nsRect &aRect )
		{
		aRect.x = 0;
		aRect.y = 0;
		aRect.width = mBounds.width;
		aRect.height = mBounds.height;
		return NS_OK;
		}

  NS_IMETHOD            SetModal(PRBool aModal);
	NS_IMETHOD            MakeFullScreen(PRBool aFullScreen);

 
 static void            RawDrawFunc( PtWidget_t *pWidget, PhTile_t *damage );

 inline nsIRegion              *GetRegion();

private:
  
  
  virtual void          DestroyNative(void);
  void                  DestroyNativeChildren(void);

  static int            MenuRegionCallback(PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo);  

  NS_IMETHOD            CreateNative(PtWidget_t *parentWidget);

  static int            ResizeHandler( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
	static int            EvInfo( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
  static int            WindowWMHandler( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
  static int            MenuRegionDestroyed( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );

  inline NS_IMETHOD     ModalEventFilter(PRBool aRealEvent, void *aEvent, PRBool *aForWindow)
		{
		*aForWindow = PR_TRUE;
		return NS_OK;
		}

private:
  PtWidget_t *mClientWidget, *mLastMenu;
  PRBool mIsTooSmall;
  PRBool mIsDestroying;
	static nsIRollupListener *gRollupListener;
	static nsIWidget *gRollupWidget;
};




class ChildWindow : public nsWindow {
  public:
    ChildWindow()
			{
			mBorderStyle     = eBorderStyle_none;
			mWindowType      = eWindowType_child;
			}
    ~ChildWindow() { }
	inline PRBool IsChild() const { return PR_TRUE; }
};

#endif 

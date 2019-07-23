




































#ifndef nsWidget_h__
#define nsWidget_h__

#include "nsBaseWidget.h"
#include "nsIRegion.h"
#ifdef PHOTON_DND
#include "nsIDragService.h"
#endif
#include "nsClipboard.h"

class nsILookAndFeel;
class nsIAppShell;
class nsIToolkit;

#include <Pt.h>

class nsWidget;

#define NS_TO_PH_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)
#define PH_TO_NS_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)





class nsWidget : public nsBaseWidget
{
public:
  nsWidget();
  virtual ~nsWidget();

  NS_DECL_ISUPPORTS_INHERITED

  

  inline NS_IMETHOD            Create(nsIWidget *aParent,
                               nsNativeWidget aNativeParent,
                               const nsRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell = nsnull,
                               nsIToolkit *aToolkit = nsnull,
                               nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD Destroy(void);
  inline nsIWidget* GetParent(void)
		{
		if( mIsDestroying ) return nsnull;
		nsIWidget* result = mParent;
		if( mParent ) NS_ADDREF( result );
		return result;
		}

  virtual void OnDestroy();

  NS_IMETHOD SetModal(PRBool aModal);
  NS_IMETHOD Show(PRBool state);
  inline NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent) { return NS_OK; }

  inline NS_IMETHOD IsVisible(PRBool &aState) { aState = mShown; return NS_OK; }

  inline NS_IMETHOD ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY) { return NS_OK; }
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  inline NS_IMETHOD Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
		{
		Move(aX,aY);
		Resize(aWidth,aHeight,aRepaint);
		return NS_OK;
		}

  inline NS_IMETHOD Enable(PRBool aState)
		{
		if( mWidget ) PtSetResource( mWidget, Pt_ARG_FLAGS, aState ? 0 : Pt_BLOCKED, Pt_BLOCKED );
		return NS_OK;
		}

  inline NS_IMETHOD IsEnabled(PRBool *aState)
		{
		if(PtWidgetFlags(mWidget) & Pt_BLOCKED) *aState = PR_FALSE;
		else *aState = PR_TRUE;
		return NS_OK;
		}

  PRBool OnResize(nsSizeEvent event);
  virtual PRBool OnResize(nsRect &aRect);
  virtual PRBool OnMove(PRInt32 aX, PRInt32 aY);

  inline nsIFontMetrics *GetFont(void) { return nsnull; }
  NS_IMETHOD SetFont(const nsFont &aFont);

  inline NS_IMETHOD SetBackgroundColor(const nscolor &aColor)
		{
		nsBaseWidget::SetBackgroundColor( aColor );
		if( mWidget ) PtSetResource( mWidget, Pt_ARG_FILL_COLOR, NS_TO_PH_RGB( aColor ), 0 );
		return NS_OK;
		}

  NS_IMETHOD SetCursor(nsCursor aCursor);

  inline NS_IMETHOD SetColorMap(nsColorMap *aColorMap) { return NS_OK; }

  inline void* GetNativeData(PRUint32 aDataType) { return (void *)mWidget; }

  NS_IMETHOD WidgetToScreen(const nsRect &aOldRect, nsRect &aNewRect);
  NS_IMETHOD ScreenToWidget(const nsRect &aOldRect, nsRect &aNewRect);

  
  inline NS_IMETHOD SetTitle(const nsAString& aTitle) { return NS_OK; }

  inline void ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY) { }

  
  inline NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect) { return NS_OK; }
  inline NS_IMETHOD ScrollWidgets(PRInt32 aDx, PRInt32 aDy) { return NS_OK; }
 
  inline NS_IMETHOD SetMenuBar(nsIMenuBar *aMenuBar) { return NS_ERROR_FAILURE; }
  inline NS_IMETHOD ShowMenuBar(PRBool aShow) { return NS_ERROR_FAILURE; }
  
  NS_IMETHOD CaptureMouse(PRBool aCapture) { return NS_ERROR_FAILURE; }


  NS_IMETHOD Invalidate(const nsRect &aRect, PRBool aIsSynchronous);
  NS_IMETHOD InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);
  inline NS_IMETHOD Update(void)
		{
		
		PtFlush();
		return NS_OK;
		}

  NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);

  inline void InitEvent(nsGUIEvent& event, PRUint32 aEventType, nsPoint* aPoint = nsnull)
		{
		if( aPoint == nsnull ) {
		  event.refPoint.x = 0;
		  event.refPoint.y = 0;
		  }
		else {
		  event.refPoint.x = aPoint->x;
		  event.refPoint.y = aPoint->y;
		  }
		event.widget = this;
		event.time = PR_IntervalNow();
		event.message = aEventType;
		}

  

  inline PRBool     ConvertStatus(nsEventStatus aStatus)
		{
		return aStatus == nsEventStatus_eConsumeNoDefault;
		}

  PRBool     DispatchMouseEvent(nsMouseEvent& aEvent);
  inline PRBool     DispatchStandardEvent(PRUint32 aMsg)
		{
		nsGUIEvent event(PR_TRUE, 0, nsnull);
		InitEvent(event, aMsg);
		return DispatchWindowEvent(&event);
		}

  
  PRBool     mIsToplevel;

  
  
  static inline void SetInstance( PtWidget_t *pWidget, nsWidget * inst ) { PtSetResource( pWidget, Pt_ARG_POINTER, (void *)inst, 0 ); }
  static inline nsWidget*  GetInstance( PtWidget_t *pWidget )
		{
		nsWidget *data;
		PtGetResource( pWidget, Pt_ARG_POINTER, &data, 0 );
		return data;
		}

protected:
  NS_IMETHOD CreateNative(PtWidget_t *parentWindow) { return NS_OK; }

  inline PRBool DispatchWindowEvent(nsGUIEvent* event)
		{
		nsEventStatus status;
		DispatchEvent(event, status);
		return ConvertStatus(status);
		}

#ifdef PHOTON_DND
	void DispatchDragDropEvent( PhEvent_t *phevent, PRUint32 aEventType, PhPoint_t *pos );
	void ProcessDrag( PhEvent_t *event, PRUint32 aEventType, PhPoint_t *pos );
#endif

  
  
  virtual void DestroyNative(void);

  
  
  
  
  
  static int      RawEventHandler( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
  inline PRBool		HandleEvent( PtWidget_t *, PtCallbackInfo_t* aCbInfo );
  PRBool          DispatchKeyEvent(PhKeyEvent_t *aPhKeyEvent);

  inline void ScreenToWidgetPos( PhPoint_t &pt )
		{
		
		short x, y;
		PtGetAbsPosition( mWidget, &x, &y );
		pt.x -= x; pt.y -= y;
		}

	inline void InitKeyEvent(PhKeyEvent_t *aPhKeyEvent, nsKeyEvent &anEvent );
	inline void InitKeyPressEvent(PhKeyEvent_t *aPhKeyEvent, nsKeyEvent &anEvent );
  void InitMouseEvent( PhPointerEvent_t * aPhButtonEvent,
                       nsWidget         * aWidget,
                       nsMouseEvent     & anEvent,
                       PRUint32         aEventType,
                       PRInt16          aButton);


  
  PRUint32 nsConvertKey( PhKeyEvent_t *aPhKeyEvent );

#if 0
  
	inline void EnableDamage( PtWidget_t *widget, PRBool enable )
		{
		PtWidget_t *top = PtFindDisjoint( widget );
		if( top ) {
			if( PR_TRUE == enable ) PtEndFlux( top );
			else PtStartFlux( top );
			}
		}
#endif


  
  
  
  
  
  static int  GotFocusCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
  static int  LostFocusCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
  static int  DestroyedCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
#ifdef PHOTON_DND
  static int  DndCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );
#endif

  PtWidget_t          *mWidget;
  nsIWidget						*mParent;
  PRBool mShown;

  PRBool       mListenForResizes;
   
  
  static nsWidget* sFocusWidget; 
  
  static nsILookAndFeel *sLookAndFeel;
#ifdef PHOTON_DND
	static nsIDragService *sDragService;
#endif
	static nsClipboard *sClipboard;
  static PRUint32 sWidgetCount;
};

#endif 

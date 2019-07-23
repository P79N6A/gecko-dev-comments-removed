








































#include "nsWidget.h"

#include "nsIAppShell.h"
#include "nsIComponentManager.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsILookAndFeel.h"
#include "nsToolkit.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include <Pt.h>
#include "PtRawDrawContainer.h"
#include "nsIRollupListener.h"
#include "nsIServiceManager.h"
#include "nsWindow.h"
#ifdef PHOTON_DND
#include "nsDragService.h"
#endif
#include "nsReadableUtils.h"

#include "nsClipboard.h"

#include <errno.h>
#include <photon/PtServer.h>

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);


#define NSCOLOR_TO_PHCOLOR(g,n) \
  g.red=NS_GET_B(n); \
  g.green=NS_GET_G(n); \
  g.blue=NS_GET_R(n);











nsILookAndFeel     *nsWidget::sLookAndFeel = nsnull;
#ifdef PHOTON_DND
nsIDragService     *nsWidget::sDragService = nsnull;
#endif
nsClipboard        *nsWidget::sClipboard = nsnull;
PRUint32            nsWidget::sWidgetCount = 0;
nsWidget*						nsWidget::sFocusWidget = 0;

nsWidget::nsWidget()
{
  if (!sLookAndFeel) {
    CallGetService(kLookAndFeelCID, &sLookAndFeel);
  }

  if( sLookAndFeel )
    sLookAndFeel->GetColor( nsILookAndFeel::eColor_WindowBackground, mBackground );

#ifdef PHOTON_DND
	if( !sDragService ) {
		nsresult rv;
		nsCOMPtr<nsIDragService> s;
		s = do_GetService( "@mozilla.org/widget/dragservice;1", &rv );
		sDragService = ( nsIDragService * ) s;
		if( NS_FAILED( rv ) ) sDragService = 0;
		}
#endif

	if( !sClipboard ) {
		nsresult rv;
		nsCOMPtr<nsClipboard> s;
		s = do_GetService( kCClipboardCID, &rv );
		sClipboard = ( nsClipboard * ) s;
		if( NS_FAILED( rv ) ) sClipboard = 0;
		}

  mWidget = nsnull;
  mParent = nsnull;
  mShown = PR_FALSE;
  mBounds.x = 0;
  mBounds.y = 0;
  mBounds.width = 0;
  mBounds.height = 0;
  mIsDestroying = PR_FALSE;
  mOnDestroyCalled = PR_FALSE;
  mIsToplevel = PR_FALSE;
  mListenForResizes = PR_FALSE;
  sWidgetCount++;
}


nsWidget::~nsWidget( ) {

	if( sFocusWidget == this ) sFocusWidget = 0;

  
  Destroy();

  if( !sWidgetCount-- ) {
    NS_IF_RELEASE( sLookAndFeel );
  	}
	}






NS_IMPL_ISUPPORTS_INHERITED0(nsWidget, nsBaseWidget)

NS_METHOD nsWidget::WidgetToScreen( const nsRect& aOldRect, nsRect& aNewRect ) {
  if( mWidget ) {
    
    aNewRect.x = aOldRect.x;
    aNewRect.y = aOldRect.y;
  	}
  return NS_OK;
	}


NS_METHOD nsWidget::ScreenToWidget( const nsRect& aOldRect, nsRect& aNewRect ) {
  return NS_OK;
	}







NS_IMETHODIMP nsWidget::Destroy( void ) {

 
  if( mIsDestroying ) return NS_OK;

  
  mIsDestroying = PR_TRUE;

  
  
  nsBaseWidget::Destroy();

  
  DestroyNative();

  
  if( mOnDestroyCalled == PR_FALSE ) OnDestroy();

  
  mEventCallback = nsnull;

  return NS_OK;
	}




void nsWidget::DestroyNative( void ) {
  if( mWidget ) {
    
    mEventCallback = nsnull;
	  
	  PtDestroyWidget( mWidget );
	  
    mWidget = nsnull;
  	}
	}



void nsWidget::OnDestroy( ) {
  mOnDestroyCalled = PR_TRUE;
  
  nsBaseWidget::OnDestroy();
  DispatchStandardEvent(NS_DESTROY);
	}







NS_METHOD nsWidget::Show( PRBool bState ) {

  if( !mWidget ) return NS_OK; 

  PtArg_t   arg;

  if( bState ) {

		if( mWindowType != eWindowType_child ) {

		  if (PtWidgetIsRealized(mWidget)) {
			  mShown = PR_TRUE; 
			  return NS_OK;
		  	}

		  
		  PtRealizeWidget(mWidget);

		  if( mWidget->rid == -1 ) {
			  
			  NS_ERROR("nsWidget::Show mWidget's rid == -1\n");
			  mShown = PR_FALSE; 
			  return NS_ERROR_FAILURE;
		  	}

		  PtSetArg(&arg, Pt_ARG_FLAGS, 0, Pt_DELAY_REALIZE);
		  PtSetResources(mWidget, 1, &arg);
		  
		  PtDamageWidget(mWidget);
#ifdef Ph_REGION_NOTIFY			
		  PhRegion_t region;
		  PtWidget_t *mWgt;
		  mWgt = (PtWidget_t*) GetNativeData( NS_NATIVE_WIDGET );
		  region.flags = Ph_REGION_NOTIFY | Ph_FORCE_BOUNDARY;
		  region.rid = PtWidgetRid(mWgt);
		  PhRegionChange(Ph_REGION_FLAGS, 0, &region, NULL, NULL);
#endif
		}
		else {
			PtWidgetToFront( mWidget );
			if( !mShown || !( mWidget->flags & Pt_REALIZED ) ) PtRealizeWidget( mWidget );
			}
  	}
  else {
		if( mWindowType != eWindowType_child ) {
      
      PtUnrealizeWidget(mWidget);

      

      PtSetArg(&arg, Pt_ARG_FLAGS, Pt_DELAY_REALIZE, Pt_DELAY_REALIZE);
      PtSetResources(mWidget, 1, &arg);
			}
		else {
			
			PtWidgetToBack( mWidget );
			if( mShown ) PtUnrealizeWidget( mWidget );
			
			}
  	}

  mShown = bState;
  return NS_OK;
	}



NS_IMETHODIMP nsWidget::SetModal( PRBool aModal ) {
  return NS_ERROR_FAILURE;
	}






NS_METHOD nsWidget::Move( PRInt32 aX, PRInt32 aY ) {

	if( ( mBounds.x == aX ) && ( mBounds.y == aY ) ) return NS_OK; 

	mBounds.x = aX;
	mBounds.y = aY;
	
	if( mWidget ) {
		if(( mWidget->area.pos.x != aX ) || ( mWidget->area.pos.y != aY )) {
			PhPoint_t pos = { aX, aY };
			PtSetResource( mWidget, Pt_ARG_POS, &pos, 0 );
		}
	}
  return NS_OK;
}


NS_METHOD nsWidget::Resize( PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint ) {

	if( ( mBounds.width == aWidth ) && ( mBounds.height == aHeight ) ) 
	   return NS_OK;

  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  if( mWidget ) {
		PhDim_t dim = { aWidth, aHeight };
		
		PtSetResource( mWidget, Pt_ARG_DIM, &dim, 0 );
		
		}

	return NS_OK;
}







PRBool nsWidget::OnResize( nsRect &aRect ) {

  PRBool result = PR_FALSE;

  
  if( mEventCallback ) {
		nsSizeEvent event(PR_TRUE, 0, nsnull);

	  InitEvent(event, NS_SIZE);

		nsRect *foo = new nsRect(0, 0, aRect.width, aRect.height);
		event.windowSize = foo;

		event.refPoint.x = 0;
		event.refPoint.y = 0;
		event.mWinWidth = aRect.width;
		event.mWinHeight = aRect.height;
  
		NS_ADDREF_THIS();
		result = DispatchWindowEvent(&event);
		NS_RELEASE_THIS();
		delete foo;
		}

	return result;
	}




PRBool nsWidget::OnMove( PRInt32 aX, PRInt32 aY ) {
  nsGUIEvent event(PR_TRUE, 0, nsnull);
  InitEvent(event, NS_MOVE);
  event.refPoint.x = aX;
  event.refPoint.y = aY;
  return DispatchWindowEvent(&event);
	}







NS_METHOD nsWidget::SetFont( const nsFont &aFont ) {

  nsIFontMetrics* mFontMetrics;
  mContext->GetMetricsFor(aFont, mFontMetrics);

	if( mFontMetrics ) {
		PtArg_t arg;

		nsFontHandle aFontHandle;
		mFontMetrics->GetFontHandle(aFontHandle);
		nsString *aString;
		aString = (nsString *) aFontHandle;
		char *str = ToNewCString(*aString);

		PtSetArg( &arg, Pt_ARG_TEXT_FONT, str, 0 );
		PtSetResources( mWidget, 1, &arg );

		delete [] str;
		NS_RELEASE(mFontMetrics);
		}
	return NS_OK;
	}






NS_METHOD nsWidget::SetCursor( nsCursor aCursor ) {

  
  if( aCursor != mCursor ) {

  	unsigned short curs = Ph_CURSOR_POINTER;
  	PgColor_t color = Ph_CURSOR_DEFAULT_COLOR;

	switch( aCursor ) {
		case eCursor_nw_resize:
			curs = Ph_CURSOR_DRAG_TL;
			break;
		case eCursor_se_resize:
			curs = Ph_CURSOR_DRAG_BR;
			break;
		case eCursor_ne_resize:
			curs = Ph_CURSOR_DRAG_TL;
			break;
		case eCursor_sw_resize:
			curs = Ph_CURSOR_DRAG_BL;
			break;

		case eCursor_crosshair:
			curs = Ph_CURSOR_CROSSHAIR;
			break;

		case eCursor_copy:
		case eCursor_alias:
		case eCursor_context_menu:
			
			break;

		case eCursor_cell:
			
			break;

		case eCursor_spinning:
			
			break;

		case eCursor_move:
		  curs = Ph_CURSOR_MOVE;
		  break;
	  
		case eCursor_help:
		  curs = Ph_CURSOR_QUESTION_POINT;
		  break;
	  
		case eCursor_grab:
		case eCursor_grabbing:
		  curs = Ph_CURSOR_FINGER;
		  break;
	  
		case eCursor_select:
		  curs = Ph_CURSOR_INSERT;
		  color = Pg_BLACK;
		  break;
	  
		case eCursor_wait:
		  curs = Ph_CURSOR_LONG_WAIT;
		  break;

		case eCursor_hyperlink:
		  curs = Ph_CURSOR_FINGER;
		  break;

		case eCursor_standard:
		  curs = Ph_CURSOR_POINTER;
		  break;

		case eCursor_n_resize:
		case eCursor_s_resize:
		  curs = Ph_CURSOR_DRAG_VERTICAL;
		  break;

		case eCursor_w_resize:
		case eCursor_e_resize:
		  curs = Ph_CURSOR_DRAG_HORIZONTAL;
		  break;

		case eCursor_zoom_in:
		case eCursor_zoom_out:
		  
		  break;

		case eCursor_not_allowed:
		case eCursor_no_drop:
		  curs = Ph_CURSOR_DONT;
		  break;

		case eCursor_col_resize:
		  
		  curs = Ph_CURSOR_DRAG_HORIZONTAL;
		  break;

		case eCursor_row_resize:
		  
		  curs = Ph_CURSOR_DRAG_VERTICAL;
		  break;

		case eCursor_vertical_text:
		  curs = Ph_CURSOR_INSERT;
		  color = Pg_BLACK;
		  break;

		case eCursor_all_scroll:
		  
		  break;

		case eCursor_nesw_resize:
		  curs = Ph_CURSOR_DRAG_FOREDIAG;
		  break;

		case eCursor_nwse_resize:
		  curs = Ph_CURSOR_DRAG_BACKDIAG;
		  break;

		case eCursor_ns_resize:
		  curs = Ph_CURSOR_DRAG_VERTICAL;
		  break;

		case eCursor_ew_resize:
		  curs = Ph_CURSOR_DRAG_HORIZONTAL;
		  break;

		case eCursor_none:
		  
		  break;

		default:
		  NS_ERROR("Invalid cursor type");
		  break;
  		}

  	if( mWidget ) {
  	  PtArg_t args[2];

			PtSetArg( &args[0], Pt_ARG_CURSOR_TYPE, curs, 0 );
			PtSetArg( &args[1], Pt_ARG_CURSOR_COLOR, color, 0 );
			PtSetResources( mWidget, 2, args );
  		}
		mCursor = aCursor;
		}

	return NS_OK;
	}


NS_METHOD nsWidget::Invalidate( const nsRect & aRect, PRBool aIsSynchronous ) {

  if( !mWidget ) return NS_OK; 
  if( !PtWidgetIsRealized( mWidget ) ) return NS_OK;

	PhRect_t prect;
	prect.ul.x = aRect.x;
	prect.ul.y = aRect.y;
	prect.lr.x = prect.ul.x + aRect.width - 1;
	prect.lr.y = prect.ul.y + aRect.height - 1;
	if( ! ( mWidget->class_rec->flags & Pt_DISJOINT ) )
		PhTranslateRect( &prect, &mWidget->area.pos );
	PtDamageExtent( mWidget, &prect );
	if( aIsSynchronous ) PtFlush( );
	return NS_OK;
	}

NS_IMETHODIMP nsWidget::InvalidateRegion( const nsIRegion *aRegion, PRBool aIsSynchronous ) {
	PhTile_t *tiles = NULL;
	aRegion->GetNativeRegion( ( void*& ) tiles );
	if( tiles ) {
		PhTranslateTiles( tiles, &mWidget->area.pos );
		PtDamageTiles( mWidget, tiles );
		if( aIsSynchronous ) PtFlush( );
		}
  return NS_OK;
	}

nsresult nsWidget::CreateWidget(nsIWidget *aParent,
                                const nsRect &aRect,
                                EVENT_CALLBACK aHandleEventFunction,
                                nsIDeviceContext *aContext,
                                nsIAppShell *aAppShell,
                                nsIToolkit *aToolkit,
                                nsWidgetInitData *aInitData,
                                nsNativeWidget aNativeParent)
{

  PtWidget_t *parentWidget = nsnull;



  if( aNativeParent ) {
    parentWidget = (PtWidget_t*)aNativeParent;
    
    mListenForResizes = PR_TRUE;
  	}
  else if( aParent ) {
    parentWidget = (PtWidget_t*) (aParent->GetNativeData(NS_NATIVE_WIDGET));
		mListenForResizes = aInitData ? aInitData->mListenForResizes : PR_FALSE;
  	}

	if( aInitData->mWindowType == eWindowType_child && !parentWidget ) return NS_ERROR_FAILURE;

  nsIWidget *baseParent = aInitData &&
                            (aInitData->mWindowType == eWindowType_dialog ||
                             aInitData->mWindowType == eWindowType_toplevel ||
                             aInitData->mWindowType == eWindowType_invisible) ?
                          nsnull : aParent;

  BaseCreate( baseParent, aRect, aHandleEventFunction, aContext, aAppShell, aToolkit, aInitData );

  mParent = aParent;
  mBounds = aRect;

  CreateNative (parentWidget);

	if( aRect.width > 1 && aRect.height > 1 ) Resize(aRect.width, aRect.height, PR_FALSE);

  if( mWidget ) {
    SetInstance(mWidget, this);
    PtAddCallback( mWidget, Pt_CB_GOT_FOCUS, GotFocusCallback, this );
    PtAddCallback( mWidget, Pt_CB_LOST_FOCUS, LostFocusCallback, this );
    PtAddCallback( mWidget, Pt_CB_IS_DESTROYED, DestroyedCallback, this );
#ifdef PHOTON_DND
    PtAddCallback( mWidget, Pt_CB_DND, DndCallback, this );
#endif
  	}

  DispatchStandardEvent(NS_CREATE);

  return NS_OK;
	}








NS_IMETHODIMP nsWidget::DispatchEvent( nsGUIEvent *aEvent, nsEventStatus &aStatus ) {

  NS_ADDREF(aEvent->widget);

  if( nsnull != mMenuListener ) {
    if( NS_MENU_EVENT == aEvent->eventStructType )
      aStatus = mMenuListener->MenuSelected(static_cast<nsMenuEvent&>(*aEvent));
  	}

  aStatus = nsEventStatus_eIgnore;



  if( nsnull != mEventCallback ) aStatus = (*mEventCallback)(aEvent);

  
  if( (aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener) )
    aStatus = mEventListener->ProcessEvent(*aEvent);

   NS_IF_RELEASE(aEvent->widget);

  return NS_OK;
	}


void nsWidget::InitMouseEvent(PhPointerEvent_t *aPhButtonEvent,
                              nsWidget * aWidget,
                              nsMouseEvent &anEvent,
                              PRUint32   aEventType,
                              PRInt16    aButton)
{
  anEvent.message = aEventType;
  anEvent.widget  = aWidget;

  if (aPhButtonEvent != nsnull) {
    anEvent.time =      PR_IntervalNow();
    anEvent.isShift =   ( aPhButtonEvent->key_mods & Pk_KM_Shift ) ? PR_TRUE : PR_FALSE;
    anEvent.isControl = ( aPhButtonEvent->key_mods & Pk_KM_Ctrl )  ? PR_TRUE : PR_FALSE;
    anEvent.isAlt =     ( aPhButtonEvent->key_mods & Pk_KM_Alt )   ? PR_TRUE : PR_FALSE;
		anEvent.isMeta =		PR_FALSE;
    anEvent.refPoint.x =   aPhButtonEvent->pos.x; 
    anEvent.refPoint.y =   aPhButtonEvent->pos.y;
    anEvent.clickCount = aPhButtonEvent->click_count;
    anEvent.button = aButton;
  	}
	}






PRBool nsWidget::DispatchMouseEvent( nsMouseEvent& aEvent ) {

  PRBool result = PR_FALSE;

  
  if (nsnull != mEventCallback) {
    result = DispatchWindowEvent(&aEvent);
    return result;
  }

  return result;
}

struct nsKeyConverter {
  PRUint32       vkCode; 
  unsigned long  keysym; 
};

static struct nsKeyConverter nsKeycodes[] = {
  { NS_VK_PAGE_UP,    Pk_Pg_Up },
  { NS_VK_PAGE_DOWN,  Pk_Pg_Down },
  { NS_VK_UP,         Pk_Up },
  { NS_VK_DOWN,       Pk_Down },
  { NS_VK_TAB,        Pk_Tab },
  { NS_VK_TAB,        Pk_KP_Tab },
  { NS_VK_HOME,       Pk_Home },
  { NS_VK_END,        Pk_End },
  { NS_VK_LEFT,       Pk_Left },
  { NS_VK_RIGHT,      Pk_Right },
  { NS_VK_DELETE,     Pk_Delete },
  { NS_VK_CANCEL,     Pk_Cancel },
  { NS_VK_BACK,       Pk_BackSpace },
  { NS_VK_CLEAR,      Pk_Clear },
  { NS_VK_RETURN,     Pk_Return },
  { NS_VK_SHIFT,      Pk_Shift_L },
  { NS_VK_SHIFT,      Pk_Shift_R },
  { NS_VK_CONTROL,    Pk_Control_L },
  { NS_VK_CONTROL,    Pk_Control_R },
  { NS_VK_ALT,        Pk_Alt_L },
  { NS_VK_ALT,        Pk_Alt_R },
  { NS_VK_INSERT,     Pk_Insert },
  { NS_VK_PAUSE,      Pk_Pause },
  { NS_VK_CAPS_LOCK,  Pk_Caps_Lock },
  { NS_VK_ESCAPE,     Pk_Escape },
  { NS_VK_PRINTSCREEN,Pk_Print },
  { NS_VK_RETURN,     Pk_KP_Enter },
  { NS_VK_INSERT,     Pk_KP_0 },
  { NS_VK_END,        Pk_KP_1 },
  { NS_VK_DOWN,       Pk_KP_2 },
  { NS_VK_PAGE_DOWN,  Pk_KP_3 },
  { NS_VK_LEFT,       Pk_KP_4 },
  { NS_VK_NUMPAD5,    Pk_KP_5 },
  { NS_VK_RIGHT,      Pk_KP_6 },
  { NS_VK_HOME,       Pk_KP_7 },
  { NS_VK_UP,         Pk_KP_8 },
  { NS_VK_PAGE_UP,    Pk_KP_9 },
  { NS_VK_COMMA,      Pk_KP_Separator }
  };



PRUint32 nsWidget::nsConvertKey( PhKeyEvent_t *aPhKeyEvent ) {

	unsigned long keysym, keymods;

  const int length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);

	keymods = aPhKeyEvent->key_mods;
	if( aPhKeyEvent->key_flags & Pk_KF_Sym_Valid )
		keysym = aPhKeyEvent->key_sym;
	else if( aPhKeyEvent->key_flags & Pk_KF_Cap_Valid )
		keysym = aPhKeyEvent->key_cap;
	else return 0;
	
  
  if (keysym >= Pk_a && keysym <= Pk_z)
     return keysym - Pk_a + NS_VK_A;

  if (keysym >= Pk_A && keysym <= Pk_Z)
     return keysym - Pk_A + NS_VK_A;

  if (keysym >= Pk_0 && keysym <= Pk_9)
     return keysym - Pk_0 + NS_VK_0;
		  
  if (keysym >= Pk_F1 && keysym <= Pk_F24) {
     return keysym - Pk_F1 + NS_VK_F1;
  	}

	if( keymods & Pk_KM_Num_Lock ) {
  	if( keysym >= Pk_KP_0 && keysym <= Pk_KP_9 )
     	return keysym - Pk_0 + NS_VK_0;
		}

  for (int i = 0; i < length; i++) {
    if( nsKeycodes[i].keysym == keysym ) {
      return (nsKeycodes[i].vkCode);
    	}
  	}

  return((int) 0);
	}

PRBool  nsWidget::DispatchKeyEvent( PhKeyEvent_t *aPhKeyEvent ) {
  NS_ASSERTION(aPhKeyEvent, "nsWidget::DispatchKeyEvent a NULL PhKeyEvent was passed in");

  if( !(aPhKeyEvent->key_flags & (Pk_KF_Cap_Valid|Pk_KF_Sym_Valid) ) ) {
		
		
		return PR_FALSE;
		}

  if ( PtIsFocused(mWidget) != 2) {
     
     return PR_FALSE;
  	}
  
  if ( ( aPhKeyEvent->key_cap == Pk_Shift_L )
       || ( aPhKeyEvent->key_cap == Pk_Shift_R )
       || ( aPhKeyEvent->key_cap == Pk_Control_L )
       || ( aPhKeyEvent->key_cap ==  Pk_Control_R )
       || ( aPhKeyEvent->key_cap ==  Pk_Num_Lock )
       || ( aPhKeyEvent->key_cap ==  Pk_Scroll_Lock )
     )
    return PR_TRUE;

  nsWindow *w = (nsWindow *) this;

  w->AddRef();
 
  if (aPhKeyEvent->key_flags & Pk_KF_Key_Down) {
		nsKeyEvent keyDownEvent(PR_TRUE, NS_KEY_DOWN, w);
		InitKeyEvent(aPhKeyEvent, keyDownEvent);
		PRBool noDefault = w->OnKey(keyDownEvent);

		nsKeyEvent keyPressEvent(PR_TRUE, NS_KEY_PRESS, w);
		InitKeyPressEvent(aPhKeyEvent, keyPressEvent);
		if (noDefault) {  
			keyPressEvent.flags = NS_EVENT_FLAG_NO_DEFAULT;
		}
		w->OnKey(keyPressEvent);
  	}
  else if (aPhKeyEvent->key_flags & Pk_KF_Key_Repeat) {
		nsKeyEvent keyPressEvent(PR_TRUE, NS_KEY_PRESS, w);
		InitKeyPressEvent(aPhKeyEvent, keyPressEvent);
		w->OnKey(keyPressEvent);
  	}
  else if (PkIsKeyDown(aPhKeyEvent->key_flags) == 0) {
		nsKeyEvent kevent(PR_TRUE, NS_KEY_UP, w);
		InitKeyEvent(aPhKeyEvent, kevent);
		w->OnKey(kevent);
  	}

  w->Release();

  return PR_TRUE;
	}

inline void nsWidget::InitKeyEvent(PhKeyEvent_t *aPhKeyEvent, nsKeyEvent &anEvent )
{
	anEvent.keyCode = 	nsConvertKey( aPhKeyEvent );
	anEvent.time = 			PR_IntervalNow();
	anEvent.isShift =   ( aPhKeyEvent->key_mods & Pk_KM_Shift ) ? PR_TRUE : PR_FALSE;
	anEvent.isControl = ( aPhKeyEvent->key_mods & Pk_KM_Ctrl )  ? PR_TRUE : PR_FALSE;
	anEvent.isAlt =     ( aPhKeyEvent->key_mods & Pk_KM_Alt )   ? PR_TRUE : PR_FALSE;
	anEvent.isMeta =    PR_FALSE;
}


inline int key_sym_displayable(const PhKeyEvent_t *kevent)
{
  if(kevent->key_flags & Pk_KF_Sym_Valid) {
    unsigned long const sym = kevent->key_sym;
    if  ( sym >= 0xF000
      ? sym >= 0xF100 && ( sizeof(wchar_t) > 2 || sym < 0x10000 )
      : ( sym & ~0x9F ) != 0 
        ) return 1;
  }
  return 0;
}


inline int key_cap_displayable(const PhKeyEvent_t *kevent)
{
  if(kevent->key_flags & Pk_KF_Cap_Valid) {
    unsigned long const cap = kevent->key_cap;
    if  ( cap >= 0xF000
      ? cap >= 0xF100 && ( sizeof(wchar_t) > 2 || cap < 0x10000 )
      : ( cap & ~0x9F ) != 0 
        ) return 1;
  }
  return 0;
}

inline void nsWidget::InitKeyPressEvent(PhKeyEvent_t *aPhKeyEvent, nsKeyEvent &anEvent )
{
	anEvent.isShift =   ( aPhKeyEvent->key_mods & Pk_KM_Shift ) ? PR_TRUE : PR_FALSE;
	anEvent.isControl = ( aPhKeyEvent->key_mods & Pk_KM_Ctrl )  ? PR_TRUE : PR_FALSE;
	anEvent.isAlt =     ( aPhKeyEvent->key_mods & Pk_KM_Alt )   ? PR_TRUE : PR_FALSE;
	anEvent.isMeta =    PR_FALSE;

	if( key_sym_displayable( aPhKeyEvent ) ) anEvent.charCode = aPhKeyEvent->key_sym;
	else {
		

		if( ( anEvent.isControl || anEvent.isAlt ) && key_cap_displayable( aPhKeyEvent ) )
			anEvent.charCode = aPhKeyEvent->key_cap;
		else anEvent.keyCode = nsConvertKey( aPhKeyEvent );
		}

	anEvent.time = 			PR_IntervalNow();
}


inline PRBool nsWidget::HandleEvent( PtWidget_t *widget, PtCallbackInfo_t* aCbInfo ) {
  PRBool  result = PR_TRUE; 
  PhEvent_t* event = aCbInfo->event;

	if (event->processing_flags & Ph_CONSUMED) return PR_TRUE;

	switch ( event->type ) {

      case Ph_EV_PTR_MOTION_NOBUTTON:
       	{
	    	PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
	    	nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull, nsMouseEvent::eReal);

        if( ptrev ) {

					if( ptrev->flags & Ph_PTR_FLAG_Z_ONLY ) break; 



        	ScreenToWidgetPos( ptrev->pos );
 	      	InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_MOVE );
        	result = DispatchMouseEvent(theMouseEvent);
        	}
       	}
	    	break;

      case Ph_EV_BUT_PRESS:
       {

	    	PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
   		  nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull, nsMouseEvent::eReal);

				
				
				PtWidget_t *disjoint = PtFindDisjoint( widget );
 				if( PtWidgetIsClassMember( disjoint, PtServer ) )
					PtContainerGiveFocus( widget, aCbInfo->event );

        if( ptrev ) {
          ScreenToWidgetPos( ptrev->pos );

          if( ptrev->buttons & Ph_BUTTON_SELECT ) 
						InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_BUTTON_DOWN,
							       nsMouseEvent::eLeftButton);
          else if( ptrev->buttons & Ph_BUTTON_MENU ) 
						InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_BUTTON_DOWN,
	  						       nsMouseEvent::eRightButton);
          else 
						InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_BUTTON_DOWN,
							       nsMouseEvent::eMiddleButton);

		  		result = DispatchMouseEvent(theMouseEvent);

					
					if( ptrev->buttons & Ph_BUTTON_MENU ) {
						nsMouseEvent contextMenuEvent(PR_TRUE, 0, nsnull,
                                                      nsMouseEvent::eReal);
						InitMouseEvent(ptrev, this, contextMenuEvent, NS_CONTEXTMENU,
							       nsMouseEvent::eRightButton);
						result = DispatchMouseEvent( contextMenuEvent );
						}
      	  }

      	 }
		break;		
		
		case Ph_EV_BUT_RELEASE:
		  {
			  PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
			  nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull,
                                         nsMouseEvent::eReal);
			  
			  
			  
			  
			  
			  
			  if (sClipboard)
			  	sClipboard->SetInputGroup(event->input_group);

			  if (event->subtype==Ph_EV_RELEASE_REAL || event->subtype==Ph_EV_RELEASE_PHANTOM) {
				  if (ptrev) {
					  ScreenToWidgetPos( ptrev->pos );
					  if ( ptrev->buttons & Ph_BUTTON_SELECT ) 
						 InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_BUTTON_UP,
						 	        nsMouseEvent::eLeftButton);
					  else if( ptrev->buttons & Ph_BUTTON_MENU ) 
						 InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_BUTTON_UP,
						 	        nsMouseEvent::eRightButton);
					  else 
						 InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE__BUTTON_UP,
						 	        nsMouseEvent::eMiddleButton);
					  
					  result = DispatchMouseEvent(theMouseEvent);
				  }
			  }
			  else if (event->subtype==Ph_EV_RELEASE_OUTBOUND) {
				  PhRect_t rect = {{0,0},{0,0}};
				  PhRect_t boundary = {{-10000,-10000},{10000,10000}};
				  PhInitDrag( PtWidgetRid(mWidget), ( Ph_DRAG_KEY_MOTION | Ph_DRAG_TRACK | Ph_TRACK_DRAG),&rect, &boundary, aCbInfo->event->input_group , NULL, NULL, NULL, NULL, NULL);
			  }
		  }
	    break;

		case Ph_EV_PTR_MOTION_BUTTON:
      	{
        PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
	    	nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull, nsMouseEvent::eReal);

        if( ptrev ) {

					if( ptrev->flags & Ph_PTR_FLAG_Z_ONLY ) break; 

#ifdef PHOTON_DND
					if( sDragService ) {
						nsDragService *d;
						nsIDragService *s = sDragService;
						d = ( nsDragService * )s;
						d->SetNativeDndData( widget, event );
						}
#endif

          ScreenToWidgetPos( ptrev->pos );
 	      	InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_MOVE );
          result = DispatchMouseEvent(theMouseEvent);
        	}
      	}
      	break;

      case Ph_EV_KEY:
        
        
        if (sClipboard)
          sClipboard->SetInputGroup(event->input_group);
				result = DispatchKeyEvent( (PhKeyEvent_t*) PhGetData( event ) );
        break;

      case Ph_EV_DRAG:
	    	{
          nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull, nsMouseEvent::eReal);

          switch(event->subtype) {

		  			case Ph_EV_DRAG_COMPLETE: 
            	{  
 		      		nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull,
                                               nsMouseEvent::eReal);
              PhPointerEvent_t* ptrev2 = (PhPointerEvent_t*) PhGetData( event );
              ScreenToWidgetPos( ptrev2->pos );
              InitMouseEvent(ptrev2, this, theMouseEvent, NS_MOUSE_BUTTON_UP,
                             nsMouseEvent::eLeftButton);
              result = DispatchMouseEvent(theMouseEvent);
            	}
							break;
		  			case Ph_EV_DRAG_MOTION_EVENT: {
      		    PhPointerEvent_t* ptrev2 = (PhPointerEvent_t*) PhGetData( event );
      		    ScreenToWidgetPos( ptrev2->pos );
  	  		    InitMouseEvent(ptrev2, this, theMouseEvent, NS_MOUSE_MOVE );
      		    result = DispatchMouseEvent(theMouseEvent);
							}
							break;
		  			}
					}
        break;

      case Ph_EV_BOUNDARY:
				PRUint32 evtype;

        switch( event->subtype ) {
          case Ph_EV_PTR_ENTER:
					case Ph_EV_PTR_ENTER_FROM_CHILD:
						evtype = NS_MOUSE_ENTER;
            break;
					case Ph_EV_PTR_LEAVE_TO_CHILD:
          case Ph_EV_PTR_LEAVE:
						evtype = NS_MOUSE_EXIT;
            break;
          default:
						evtype = 0;
            break;
        	}

				if( evtype != 0 ) {
					PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
					nsMouseEvent theMouseEvent(PR_TRUE, 0, nsnull,
                                               nsMouseEvent::eReal);
					ScreenToWidgetPos( ptrev->pos );
					InitMouseEvent( ptrev, this, theMouseEvent, evtype );
					result = DispatchMouseEvent( theMouseEvent );
					}
        break;
    	}

  return result;
	}








int nsWidget::RawEventHandler( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) {

  
  nsWidget *someWidget = (nsWidget*) data;

  if( someWidget && someWidget->mIsDestroying == PR_FALSE && someWidget->HandleEvent( widget, cbinfo ) )
		return Pt_END; 

  return Pt_CONTINUE;
	}


int nsWidget::GotFocusCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) 
{
  nsWidget *pWidget = (nsWidget *) data;

	if( PtWidgetIsClass( widget, PtWindow ) ) {
		if( pWidget->mEventCallback ) {
			
			pWidget->DispatchStandardEvent(NS_ACTIVATE);
			return Pt_CONTINUE;
			}
		}

  return Pt_CONTINUE;
}

int nsWidget::LostFocusCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) 
{
  return Pt_CONTINUE;
}

int nsWidget::DestroyedCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) {
  nsWidget *pWidget = (nsWidget *) data;
  if( !pWidget->mIsDestroying ) pWidget->OnDestroy();
  return Pt_CONTINUE;
	}

#ifdef PHOTON_DND
void nsWidget::ProcessDrag( PhEvent_t *event, PRUint32 aEventType, PhPoint_t *pos ) {
	nsCOMPtr<nsIDragSession> currSession;
	sDragService->GetCurrentSession ( getter_AddRefs(currSession) );
	if( !currSession ) return;

	int action = nsIDragService::DRAGDROP_ACTION_NONE;
	nsDragService *d = ( nsDragService * ) sDragService;
	
  if( d->mActionType & nsIDragService::DRAGDROP_ACTION_MOVE )
    action = nsIDragService::DRAGDROP_ACTION_MOVE;
  else if( d->mActionType & nsIDragService::DRAGDROP_ACTION_LINK )
    action = nsIDragService::DRAGDROP_ACTION_LINK;
  else if( d->mActionType & nsIDragService::DRAGDROP_ACTION_COPY )
    action = nsIDragService::DRAGDROP_ACTION_COPY;

	currSession->SetDragAction( action );

	DispatchDragDropEvent( event, aEventType, pos );

	int old_subtype = event->subtype;
	event->subtype = Ph_EV_DND_ENTER;

	PRBool canDrop;
	currSession->GetCanDrop(&canDrop);
	if(!canDrop) {
		static PhCharacterCursorDescription_t nodrop_cursor = {  { Ph_CURSOR_NOINPUT, sizeof(PhCharacterCursorDescription_t) }, PgRGB( 255, 255, 224 ) };
		PhAckDnd( event, Ph_EV_DND_MOTION, ( PhCursorDescription_t * ) &nodrop_cursor );
		}
	else {
		static PhCharacterCursorDescription_t drop_cursor = { { Ph_CURSOR_PASTE, sizeof(PhCharacterCursorDescription_t) }, PgRGB( 255, 255, 224 ) };
		PhAckDnd( event, Ph_EV_DND_MOTION, ( PhCursorDescription_t * ) &drop_cursor );
		}

	event->subtype = old_subtype;

	
	currSession->SetCanDrop(PR_FALSE);
	}

void nsWidget::DispatchDragDropEvent( PhEvent_t *phevent, PRUint32 aEventType, PhPoint_t *pos ) {
  nsEventStatus status;
  nsDragEvent event(PR_TRUE, 0, nsnull);

  InitEvent( event, aEventType );

  event.refPoint.x = pos->x;
  event.refPoint.y = pos->y;

	PhDndEvent_t *dnd = ( PhDndEvent_t * ) PhGetData( phevent );
  event.isControl = ( dnd->key_mods & Pk_KM_Ctrl ) ? PR_TRUE : PR_FALSE;
  event.isShift   = ( dnd->key_mods & Pk_KM_Shift ) ? PR_TRUE : PR_FALSE;
  event.isAlt     = ( dnd->key_mods & Pk_KM_Alt ) ? PR_TRUE : PR_FALSE;
  event.isMeta    = PR_FALSE;

	event.widget = this;



  DispatchEvent( &event, status );
	}

int nsWidget::DndCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) {
	nsWidget *pWidget = (nsWidget *) data;
	PtDndCallbackInfo_t *cbdnd = (  PtDndCallbackInfo_t * ) cbinfo->cbdata;

	static PtDndFetch_t dnd_data_template = { "Mozilla", "dnddata", Ph_TRANSPORT_INLINE, Pt_DND_SELECT_MOTION,
                        NULL, NULL, NULL, NULL, NULL };



	PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( cbinfo->event );

	pWidget->ScreenToWidgetPos( ptrev->pos );



	switch( cbinfo->reason_subtype ) {
		case Ph_EV_DND_ENTER: {
			sDragService->StartDragSession();
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_ENTER, &ptrev->pos );

			PtDndSelect( widget, &dnd_data_template, 1, NULL, NULL, cbinfo );
			}
			break;

		case Ph_EV_DND_MOTION: {
			sDragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_OVER, &ptrev->pos );
			}
			break;
		case Ph_EV_DND_DROP:
			nsDragService *d;
			d = ( nsDragService * )sDragService;
			if( d->SetDropData( (char*)cbdnd->data ) != NS_OK ) break;
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_DROP, &ptrev->pos );
			sDragService->EndDragSession(PR_TRUE);
			((nsDragService*) sDragService)->SourceEndDrag();
			break;

		case Ph_EV_DND_LEAVE:
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_EXIT, &ptrev->pos );
			sDragService->EndDragSession(PR_FALSE);
			break;

		case Ph_EV_DND_CANCEL:
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_EXIT, &ptrev->pos );
			sDragService->EndDragSession(PR_TRUE);
			((nsDragService*) sDragService)->SourceEndDrag();
			break;
		}

	return Pt_CONTINUE;
	}
#endif 
















































#include "nsWindow.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsCOMPtr.h"
#include "nsToolkit.h"
#include "nsIEnumerator.h"
#include "prmem.h"

#include <Appearance.h>
#include <Timer.h>
#include <Icons.h>
#include <Errors.h>

#include "nsplugindefs.h"
#include "nsMacEventHandler.h"
#include "nsMacResources.h"
#include "nsIRegion.h"
#include "nsIRollupListener.h"
#include "nsPIWidgetMac.h"

#include "nsCarbonHelpers.h"
#include "nsGfxUtils.h"
#include "nsRegionPool.h"

#include <Gestalt.h>

#ifdef MAC_OS_X_VERSION_10_3
const short PANTHER_RESIZE_UP_CURSOR     = 19;
const short PANTHER_RESIZE_DOWN_CURSOR   = 20;
const short PANTHER_RESIZE_UPDOWN_CURSOR = 21;
#else

const short PANTHER_RESIZE_UP_CURSOR     = 19; 
const short PANTHER_RESIZE_DOWN_CURSOR   = 20; 
const short PANTHER_RESIZE_UPDOWN_CURSOR = 21; 
#endif

const short JAGUAR_RESIZE_UP_CURSOR      = 135;
const short JAGUAR_RESIZE_DOWN_CURSOR    = 136;
const short JAGUAR_RESIZE_UPDOWN_CURSOR  = 141;


nsIRollupListener * gRollupListener = nsnull;
nsIWidget         * gRollupWidget   = nsnull;



static NMRec	gNMRec;
static Boolean	gNotificationInstalled = false;


static CursorSpinner *gCursorSpinner = nsnull;
static const int kSpinCursorFirstFrame = 200;


static RegionToRectsUPP sAddRectToArrayProc = nsnull;

#pragma mark -





#if defined(INVALIDATE_DEBUGGING) || defined(PAINT_DEBUGGING)
static void blinkRect(const Rect* r, PRBool isPaint);
static void blinkRgn(RgnHandle rgn, PRBool isPaint);
#endif

#if defined(INVALIDATE_DEBUGGING) || defined(PAINT_DEBUGGING) || defined (PINK_PROFILING)
static Boolean KeyDown(const UInt8 theKey)
{
	KeyMap map;
	GetKeys(map);
	return ((*((UInt8 *)map + (theKey >> 3)) >> (theKey & 7)) & 1) != 0;
}
#endif

#if defined(INVALIDATE_DEBUGGING) || defined(PAINT_DEBUGGING)

static Boolean caps_lock()
{
  return KeyDown(0x39);
}


static void FlushCurPortBuffer()
{
    CGrafPtr    curPort;
    ::GetPort((GrafPtr*)&curPort);
    ::QDFlushPortBuffer(curPort, nil);      
}

static void blinkRect(const Rect* r, PRBool isPaint)
{
	StRegionFromPool oldClip;
	UInt32 endTicks;

    if (oldClip != NULL)
    ::GetClip(oldClip);

    ::ClipRect(r);

    if (isPaint)
    {
        Pattern grayPattern;

        ::GetQDGlobalsGray(&grayPattern);

        ::ForeColor(blackColor);
        ::BackColor(whiteColor);

        ::PenMode(patXor);
        ::PenPat(&grayPattern);
        ::PaintRect(r);
        FlushCurPortBuffer();
        ::Delay(5, &endTicks);
        ::PaintRect(r);
        FlushCurPortBuffer();
        ::PenNormal();
    }
    else
    {
        ::InvertRect(r);
        FlushCurPortBuffer();
        ::Delay(5, &endTicks);
        ::InvertRect(r);
        FlushCurPortBuffer();
    }

	if (oldClip != NULL)
		::SetClip(oldClip);
}

static void blinkRgn(RgnHandle rgn, PRBool isPaint)
{
    StRegionFromPool oldClip;
    UInt32 endTicks;

    if (oldClip != NULL)
        ::GetClip(oldClip);

    ::SetClip(rgn);

    if (isPaint)
    {
        Pattern grayPattern;

        ::GetQDGlobalsGray(&grayPattern);

        ::ForeColor(blackColor);
        ::BackColor(whiteColor);

        ::PenMode(patXor);
        ::PenPat(&grayPattern);
        ::PaintRgn(rgn);
        FlushCurPortBuffer();

        ::Delay(5, &endTicks);
        ::PaintRgn(rgn);
        FlushCurPortBuffer();
        ::PenNormal();
    }
    else
    {
        ::InvertRgn(rgn);
        FlushCurPortBuffer();
        ::Delay(5, &endTicks);
        ::InvertRgn(rgn);
        FlushCurPortBuffer();
    }

    if (oldClip != NULL)
        ::SetClip(oldClip);
}

#endif

struct TRectArray
{
  TRectArray(Rect* aRectList, PRUint32 aCapacity)
  : mRectList(aRectList)
  , mNumRects(0)
  , mCapacity(aCapacity) { }

  Rect*    mRectList;
  PRUint32 mNumRects;
  PRUint32 mCapacity;
};

#pragma mark -






nsWindow::nsWindow() : nsBaseWidget() , nsDeleteObserved(this), nsIKBStateControl()
{
	WIDGET_SET_CLASSNAME("nsWindow");

  mParent = nsnull;
  mIsTopWidgetWindow = PR_FALSE;
  mBounds.SetRect(0,0,0,0);

  mResizingChildren = PR_FALSE;
  mVisible = PR_FALSE;
  mEnabled = PR_TRUE;
  SetPreferredSize(0,0);

  mFontMetrics = nsnull;
  mMenuBar = nsnull;
  mTempRenderingContext = nsnull;

  mWindowRegion = nsnull;
  mVisRegion = nsnull;
  mWindowPtr = nsnull;
  mDrawing = PR_FALSE;
  mInUpdate = PR_FALSE;
  mDestructorCalled = PR_FALSE;

  SetBackgroundColor(NS_RGB(255, 255, 255));
  SetForegroundColor(NS_RGB(0, 0, 0));

  mPluginPort = nsnull;

  AcceptFocusOnClick(PR_TRUE);
  
  if (!sAddRectToArrayProc)
    sAddRectToArrayProc = ::NewRegionToRectsUPP(AddRectToArrayProc);
}







nsWindow::~nsWindow()
{
	
	for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
		nsWindow* childWindow = NS_STATIC_CAST(nsWindow*, kid);
		childWindow->mParent = nsnull;
	}

	mDestructorCalled = PR_TRUE;

	

	if (mWindowRegion)
	{
		::DisposeRgn(mWindowRegion);
		mWindowRegion = nsnull;	
	}

	if (mVisRegion)
	{
		::DisposeRgn(mVisRegion);
		mVisRegion = nsnull;	
	}
			
	NS_IF_RELEASE(mTempRenderingContext);
	
	NS_IF_RELEASE(mFontMetrics);
	NS_IF_RELEASE(mMenuBar);
	NS_IF_RELEASE(mMenuListener);
	
	if (mPluginPort) {
		delete mPluginPort;
	}
}

NS_IMPL_ISUPPORTS_INHERITED2(nsWindow, nsBaseWidget, nsIKBStateControl, nsIPluginWidget)







nsresult nsWindow::StandardCreate(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData,
                      nsNativeWidget aNativeParent)	
{
	mParent = aParent;
	mBounds = aRect;
	CalcWindowRegions();

	BaseCreate(aParent, aRect, aHandleEventFunction, 
							aContext, aAppShell, aToolkit, aInitData);

	if (mParent)
	{
		SetBackgroundColor(mParent->GetBackgroundColor());
		SetForegroundColor(mParent->GetForegroundColor());
	}

	if (mWindowPtr == nsnull) {
		if (aParent)
			mWindowPtr = (WindowPtr)aParent->GetNativeData(NS_NATIVE_DISPLAY);




	}
	return NS_OK;
}






NS_IMETHODIMP nsWindow::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{	 
	return(StandardCreate(aParent, aRect, aHandleEventFunction,
													aContext, aAppShell, aToolkit, aInitData,
														nsnull));
}






NS_IMETHODIMP nsWindow::Create(nsNativeWidget aNativeParent,		
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
	
	
	nsIWidget* aParent = (nsIWidget*)aNativeParent;
	
	return(Create(aParent, aRect, aHandleEventFunction,
									aContext, aAppShell, aToolkit, aInitData));
}






NS_IMETHODIMP nsWindow::Destroy()
{
	if (mOnDestroyCalled)
		return NS_OK;
	mOnDestroyCalled = PR_TRUE;

	nsBaseWidget::OnDestroy();
	nsBaseWidget::Destroy();
	mParent = 0;

  
  
  if ( this == gRollupWidget ) {
    if ( gRollupListener )
      gRollupListener->Rollup();
    CaptureRollupEvents(nsnull, PR_FALSE, PR_TRUE);
  }

	NS_IF_RELEASE(mMenuBar);
	SetMenuBar(nsnull);

	ReportDestroyEvent();	

	return NS_OK;
}

#pragma mark -






nsIWidget* nsWindow::GetParent(void)
{
  if (mIsTopWidgetWindow) return nsnull;
  return  mParent;
}






void* nsWindow::GetNativeData(PRUint32 aDataType)
{
	nsPoint		point;
	void*		retVal = nsnull;

  switch (aDataType) 
	{
	case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    	retVal = (void*)this;
    	break;

    case NS_NATIVE_GRAPHIC:
    
    
    
    
      retVal = (void*)::GetWindowPort(mWindowPtr);
      break;
      
    case NS_NATIVE_DISPLAY:
      retVal = (void*)mWindowPtr;
    	break;

    case NS_NATIVE_REGION:
		retVal = (void*)mVisRegion;
    	break;

    case NS_NATIVE_COLORMAP:
    	
    	break;

    case NS_NATIVE_OFFSETX:
    	point.MoveTo(mBounds.x, mBounds.y);
    	LocalToWindowCoordinate(point);
    	retVal = (void*)point.x;
     	break;

    case NS_NATIVE_OFFSETY:
    	point.MoveTo(mBounds.x, mBounds.y);
    	LocalToWindowCoordinate(point);
    	retVal = (void*)point.y;
    	break;
    
    case NS_NATIVE_PLUGIN_PORT:
    	
    	if (mPluginPort == nsnull)
    		mPluginPort = new nsPluginPort;
    		
		point.MoveTo(mBounds.x, mBounds.y);
		LocalToWindowCoordinate(point);

		
		
		mPluginPort->port = ::GetWindowPort(mWindowPtr);
		mPluginPort->portx = -point.x;
		mPluginPort->porty = -point.y;
		
    	retVal = (void*)mPluginPort;
	}

  return retVal;
}

#pragma mark -





NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
  bState = mVisible;
  return NS_OK;
}






NS_IMETHODIMP nsWindow::Show(PRBool bState)
{
  mVisible = bState;
  return NS_OK;
}

    
NS_IMETHODIMP nsWindow::ModalEventFilter(PRBool aRealEvent, void *aEvent,
                                         PRBool *aForWindow)
{
  *aForWindow = PR_TRUE;
  return NS_OK;
}






NS_IMETHODIMP nsWindow::Enable(PRBool aState)
{
	mEnabled = aState;
	return NS_OK;
}

    
NS_IMETHODIMP nsWindow::IsEnabled(PRBool *aState)
{
	NS_ENSURE_ARG_POINTER(aState);
	*aState = mEnabled;
	return NS_OK;
}

static Boolean we_are_front_process()
{
	ProcessSerialNumber	thisPSN;
	ProcessSerialNumber	frontPSN;
	(void)::GetCurrentProcess(&thisPSN);
	if (::GetFrontProcess(&frontPSN) == noErr)
	{
		if ((frontPSN.highLongOfPSN == thisPSN.highLongOfPSN) &&
			(frontPSN.lowLongOfPSN == thisPSN.lowLongOfPSN))
			return true;
	}
	return false;
}






NS_IMETHODIMP nsWindow::SetFocus(PRBool aRaise)
{
  nsCOMPtr<nsIWidget> top;
  nsToolkit::GetTopWidget(mWindowPtr, getter_AddRefs(top));

  if (top) {
    nsCOMPtr<nsPIWidgetMac> topMac = do_QueryInterface(top);

    if (topMac) {
      nsMacEventDispatchHandler* eventDispatchHandler = nsnull;
      topMac->GetEventDispatchHandler(&eventDispatchHandler);

      if (eventDispatchHandler)
        eventDispatchHandler->SetFocus(this);
    }
  }
	
	
	if (gNotificationInstalled && we_are_front_process())
	{
		(void)::NMRemove(&gNMRec);
		gNotificationInstalled = false;
	}
	
	return NS_OK;
}






nsIFontMetrics* nsWindow::GetFont(void)
{
	return mFontMetrics;
}

    





NS_IMETHODIMP nsWindow::SetFont(const nsFont &aFont)
{
	NS_IF_RELEASE(mFontMetrics);
	if (mContext)
		mContext->GetMetricsFor(aFont, mFontMetrics);
 	return NS_OK;
}







NS_IMETHODIMP nsWindow::SetColorMap(nsColorMap *aColorMap)
{
	
	
	
	
	return NS_OK;
}










NS_IMETHODIMP nsWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
  if (mMenuBar)
    mMenuBar->SetParent(nsnull);
  NS_IF_RELEASE(mMenuBar);
  NS_IF_ADDREF(aMenuBar);
  mMenuBar = aMenuBar;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::ShowMenuBar(PRBool aShow)
{
  
  return NS_ERROR_FAILURE;
}






nsIMenuBar* nsWindow::GetMenuBar()
{
  return mMenuBar;
}


PRBool OnPantherOrLater() 
{
    static PRBool gInitVer1030 = PR_FALSE;
    static PRBool gOnPantherOrLater = PR_FALSE;
    if(!gInitVer1030)
    {
        gOnPantherOrLater =
            (nsToolkit::OSXVersion() >= MAC_OS_X_VERSION_10_3_HEX);
        gInitVer1030 = PR_TRUE;
    }
    return gOnPantherOrLater;
}






NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  nsBaseWidget::SetCursor(aCursor);
	
  if ( gCursorSpinner == nsnull )
  {
      gCursorSpinner = new CursorSpinner();
  }
  
  
  short cursor = -1;
  switch (aCursor)
  {
    case eCursor_standard:            cursor = kThemeArrowCursor; break;
    case eCursor_wait:                cursor = kThemeWatchCursor; break;
    case eCursor_select:              cursor = kThemeIBeamCursor; break;
    case eCursor_hyperlink:           cursor = kThemePointingHandCursor; break;
    case eCursor_crosshair:           cursor = kThemeCrossCursor; break;
    case eCursor_move:                cursor = kThemeOpenHandCursor; break;
    case eCursor_help:                cursor = 128; break;
    case eCursor_copy:                cursor = kThemeCopyArrowCursor; break;
    case eCursor_alias:               cursor = kThemeAliasArrowCursor; break;
    case eCursor_context_menu:        cursor = kThemeContextualMenuArrowCursor; break;
    case eCursor_cell:                cursor = kThemePlusCursor; break;
    case eCursor_grab:                cursor = kThemeOpenHandCursor; break;
    case eCursor_grabbing:            cursor = kThemeClosedHandCursor; break;
    case eCursor_spinning:            cursor = kSpinCursorFirstFrame; break; 
    case eCursor_zoom_in:             cursor = 129; break;
    case eCursor_zoom_out:            cursor = 130; break;
    case eCursor_not_allowed:
    case eCursor_no_drop:             cursor = kThemeNotAllowedCursor; break;  
    case eCursor_col_resize:          cursor = 132; break; 
    case eCursor_row_resize:          cursor = 133; break;
    case eCursor_vertical_text:       cursor = 134; break;   
    case eCursor_all_scroll:          cursor = kThemeOpenHandCursor; break;
    case eCursor_n_resize:            cursor = OnPantherOrLater() ? PANTHER_RESIZE_UP_CURSOR : JAGUAR_RESIZE_UP_CURSOR; break;
    case eCursor_s_resize:            cursor = OnPantherOrLater() ? PANTHER_RESIZE_DOWN_CURSOR : JAGUAR_RESIZE_DOWN_CURSOR; break;
    case eCursor_w_resize:            cursor = kThemeResizeLeftCursor; break; 
    case eCursor_e_resize:            cursor = kThemeResizeRightCursor; break;
    case eCursor_nw_resize:           cursor = 137; break;
    case eCursor_se_resize:           cursor = 138; break;
    case eCursor_ne_resize:           cursor = 139; break;
    case eCursor_sw_resize:           cursor = 140; break;
    case eCursor_ew_resize:           cursor = kThemeResizeLeftRightCursor; break;
    case eCursor_ns_resize:           cursor = OnPantherOrLater() ? PANTHER_RESIZE_UPDOWN_CURSOR : JAGUAR_RESIZE_UPDOWN_CURSOR; break;
    case eCursor_nesw_resize:         cursor = 142; break;
    case eCursor_nwse_resize:         cursor = 143; break;        
    default:                          
      cursor = kThemeArrowCursor; 
      break;
  }


  if (aCursor == eCursor_spinning)
  {
    gCursorSpinner->StartSpinCursor();
  }
  else
  {
    gCursorSpinner->StopSpinCursor();
    nsWindow::SetCursorResource(cursor);
  }

  return NS_OK;
  
} 

void nsWindow::SetCursorResource(short aCursorResourceNum)
{
    if (aCursorResourceNum >= 0)
    {
        if (aCursorResourceNum >= 128)
        {
            nsMacResources::OpenLocalResourceFile();
            CursHandle cursHandle = ::GetCursor(aCursorResourceNum);
            NS_ASSERTION (cursHandle, "Can't load cursor, is the resource file installed correctly?");
            if (cursHandle)
            {
                ::SetCursor(*cursHandle);
            }
            nsMacResources::CloseLocalResourceFile();            
        }
        else
        {
            ::SetThemeCursor(aCursorResourceNum);
        }
    }    
}

CursorSpinner::CursorSpinner() :
    mSpinCursorFrame(0), mTimerUPP(nsnull), mTimerRef(nsnull)
{
   mTimerUPP = NewEventLoopTimerUPP(SpinCursor);
}

CursorSpinner::~CursorSpinner()
{
    if (mTimerRef) ::RemoveEventLoopTimer(mTimerRef);
    if (mTimerUPP) ::DisposeEventLoopTimerUPP(mTimerUPP);
}

short CursorSpinner::GetNextCursorFrame()
{
    int result = kSpinCursorFirstFrame + mSpinCursorFrame;
    mSpinCursorFrame = (mSpinCursorFrame + 1) % 4;
    return (short) result;
}

void CursorSpinner::StartSpinCursor()
{
    OSStatus result = noErr;
    if (mTimerRef == nsnull)
    {
        result = ::InstallEventLoopTimer(::GetMainEventLoop(), 0, 0.25 * kEventDurationSecond,
                                         mTimerUPP, this, &mTimerRef);
        if (result != noErr)
        {
            mTimerRef = nsnull;
            nsWindow::SetCursorResource(kSpinCursorFirstFrame);
        }
    }
}

void CursorSpinner::StopSpinCursor()
{
    if (mTimerRef)
    {
        ::RemoveEventLoopTimer(mTimerRef);
        mTimerRef = nsnull;
    }
}

pascal void CursorSpinner::SpinCursor(EventLoopTimerRef inTimer, void *inUserData)
{
    CursorSpinner* cs = reinterpret_cast<CursorSpinner*>(inUserData);
    nsWindow::SetCursorResource(cs->GetNextCursorFrame());
}

#pragma mark -





NS_IMETHODIMP nsWindow::GetBounds(nsRect &aRect)
{
  aRect = mBounds;
  return NS_OK;
}


NS_METHOD nsWindow::SetBounds(const nsRect &aRect)
{
  nsresult rv = Inherited::SetBounds(aRect);
  if ( NS_SUCCEEDED(rv) )
    CalcWindowRegions();

  return rv;
}


NS_IMETHODIMP nsWindow::ConstrainPosition(PRBool aAllowSlop,
                                          PRInt32 *aX, PRInt32 *aY)
{
	return NS_OK;
}






NS_IMETHODIMP nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
	if ((mBounds.x != aX) || (mBounds.y != aY))
	{
		
		if ((mParent != nsnull) && (!mIsTopWidgetWindow))
			Invalidate(PR_FALSE);
	  	
		
		mBounds.x = aX;
		mBounds.y = aY;

		
		CalcWindowRegions();

		
		ReportMoveEvent();
	}
	return NS_OK;
}






NS_IMETHODIMP nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  if ((mBounds.width != aWidth) || (mBounds.height != aHeight))
  {
    
    mBounds.width  = aWidth;
    mBounds.height = aHeight;

	
	CalcWindowRegions();
	
    
    if (aRepaint)
      Invalidate(PR_FALSE);

    
    ReportSizeEvent();
  }
  else {
    
    
    
    CalcWindowRegions();
    if (aRepaint)
      Invalidate(PR_FALSE);
  }

  return NS_OK;
}






NS_IMETHODIMP nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
	Move(aX, aY);
	Resize(aWidth, aHeight, aRepaint);
	return NS_OK;
}


NS_METHOD nsWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  aWidth  = mPreferredWidth;
  aHeight = mPreferredHeight;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}





NS_IMETHODIMP nsWindow::BeginResizingChildren(void)
{
	mResizingChildren = PR_TRUE;
	mSaveVisible = mVisible;
	mVisible = PR_FALSE;

	return NS_OK;
}





NS_IMETHODIMP nsWindow::EndResizingChildren(void)
{
	mResizingChildren = PR_FALSE;
	mVisible = mSaveVisible;

	CalcWindowRegions();
	return NS_OK;
}







NS_IMETHODIMP nsWindow::Validate()
{
	if (!mWindowPtr || !mVisible || !ContainerHierarchyIsVisible())
		return NS_OK;

	nsRect wRect = mBounds;
	LocalToWindowCoordinate(wRect);
	Rect macRect;
	nsRectToMacRect(wRect, macRect);

	StPortSetter    portSetter(mWindowPtr);
	StOriginSetter  originSetter(mWindowPtr);

	::ValidWindowRect(mWindowPtr, &macRect);

	return NS_OK;
}






NS_IMETHODIMP nsWindow::Invalidate(PRBool aIsSynchronous)
{
	nsRect area = mBounds;
	area.x = area.y = 0;
	Invalidate(area, aIsSynchronous);
	return NS_OK;
}






NS_IMETHODIMP nsWindow::Invalidate(const nsRect &aRect, PRBool aIsSynchronous)
{
	if (!mWindowPtr)
		return NS_OK;

  if (!mVisible || !ContainerHierarchyIsVisible())
    return NS_OK;

	nsRect wRect = aRect;
	wRect.MoveBy(mBounds.x, mBounds.y);				
	LocalToWindowCoordinate(wRect);
	Rect macRect;
	nsRectToMacRect(wRect, macRect);

	StPortSetter portSetter(mWindowPtr);
	StOriginSetter  originSetter(mWindowPtr);

#ifdef INVALIDATE_DEBUGGING
	if (caps_lock())
		::blinkRect(&macRect, PR_FALSE);
#endif

	::InvalWindowRect(mWindowPtr, &macRect);
	if ( aIsSynchronous )
	  Update();

	return NS_OK;
}







NS_IMETHODIMP nsWindow::InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous)
{
	if (!mWindowPtr)
		return NS_OK;

  if (!mVisible || !ContainerHierarchyIsVisible())
    return NS_OK;
    
	
	void* nativeRgn;
	aRegion->GetNativeRegion(nativeRgn);
	StRegionFromPool windowRgn;
	::CopyRgn(RgnHandle(nativeRgn), windowRgn);

	
	PRInt32	offX, offY;
	CalcOffset(offX, offY);
	::OffsetRgn(windowRgn, mBounds.x + offX, mBounds.y + offY);
	
	StPortSetter    portSetter(mWindowPtr);
    StOriginSetter  originSetter(mWindowPtr);

#ifdef INVALIDATE_DEBUGGING
	if (caps_lock())
		::blinkRgn(windowRgn, PR_FALSE);
#endif

	::InvalWindowRgn(mWindowPtr, windowRgn);
	if ( aIsSynchronous )
	  Update();

	return NS_OK;
}

inline PRUint16 COLOR8TOCOLOR16(PRUint8 color8)
{
	
	return (color8 << 8) | color8;	
}





void nsWindow::StartDraw(nsIRenderingContext* aRenderingContext)
{
	if (mDrawing || mOnDestroyCalled)
		return;
	mDrawing = PR_TRUE;

	CalcWindowRegions();	

	if (aRenderingContext == nsnull)
	{
		
		mTempRenderingContext = GetRenderingContext();
		mTempRenderingContextMadeHere = PR_TRUE;
	}
	else
	{
		
		NS_IF_ADDREF(aRenderingContext);
		mTempRenderingContext = aRenderingContext;
		mTempRenderingContextMadeHere = PR_FALSE;

		
		mTempRenderingContext->Init(mContext, this);
	}

	
	
	if (mFontMetrics)
	{
		mTempRenderingContext->SetFont(mFontMetrics);
	}

	
	nscolor color = GetBackgroundColor();
	RGBColor macColor;
	macColor.red   = COLOR8TOCOLOR16(NS_GET_R(color));
	macColor.green = COLOR8TOCOLOR16(NS_GET_G(color));
	macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(color));
	::RGBBackColor(&macColor);

	color = GetForegroundColor();
	macColor.red   = COLOR8TOCOLOR16(NS_GET_R(color));
	macColor.green = COLOR8TOCOLOR16(NS_GET_G(color));
	macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(color));
	::RGBForeColor(&macColor);

	mTempRenderingContext->SetColor(color);				
	mTempRenderingContext->PushState();           
}






void nsWindow::EndDraw()
{
	if (! mDrawing || mOnDestroyCalled)
		return;
	mDrawing = PR_FALSE;

	mTempRenderingContext->PopState();

	NS_RELEASE(mTempRenderingContext);
	
	
}






void
nsWindow::Flash(nsPaintEvent	&aEvent)
{
#ifdef NS_DEBUG
	Rect flashRect;
	if (debug_WantPaintFlashing() && aEvent.rect ) {
		::SetRect ( &flashRect, aEvent.rect->x, aEvent.rect->y, aEvent.rect->x + aEvent.rect->width,
	          aEvent.rect->y + aEvent.rect->height );
		StPortSetter portSetter(mWindowPtr);
		unsigned long endTicks;
		::InvertRect ( &flashRect );
		::Delay(10, &endTicks);
		::InvertRect ( &flashRect );
	}
#endif
}







PRBool
nsWindow::OnPaint(nsPaintEvent &event)
{
	return PR_TRUE;
}












NS_IMETHODIMP	nsWindow::Update()
{
  if (! mVisible || !mWindowPtr || !ContainerHierarchyIsVisible())
    return NS_OK;

  static PRBool  reentrant = PR_FALSE;

  if (reentrant)
    HandleUpdateEvent(nil);
  else
  {
    reentrant = PR_TRUE;
    
    StRegionFromPool redrawnRegion;
    if (!redrawnRegion)
      return NS_ERROR_OUT_OF_MEMORY;

    
    StRegionFromPool saveUpdateRgn;
    if (!saveUpdateRgn)
      return NS_ERROR_OUT_OF_MEMORY;
    ::GetWindowRegion(mWindowPtr, kWindowUpdateRgn, saveUpdateRgn);

    
    
    
    
    
    
    StRegionFromPool windowContentRgn;
    if (!windowContentRgn)
      return NS_ERROR_OUT_OF_MEMORY;

    ::GetWindowRegion(mWindowPtr, kWindowContentRgn, windowContentRgn);

    ::SectRgn(saveUpdateRgn, windowContentRgn, saveUpdateRgn);

    
    StPortSetter portSetter(mWindowPtr);
    StOriginSetter originSetter(mWindowPtr);

    
    
    ::BeginUpdate(mWindowPtr);
    mInUpdate = PR_TRUE;

    HandleUpdateEvent(redrawnRegion);

    
    ::EndUpdate(mWindowPtr);
    mInUpdate = PR_FALSE;
    
    
    
    
    

    Point origin = {0, 0};
    ::GlobalToLocal(&origin);
    
    ::OffsetRgn(saveUpdateRgn, origin.h, origin.v);
    
    
    
    ::DiffRgn(saveUpdateRgn, redrawnRegion, saveUpdateRgn);

    
    ::InvalWindowRgn(mWindowPtr, saveUpdateRgn);

    reentrant = PR_FALSE;
  }

  return NS_OK;
}


#pragma mark -










OSStatus
nsWindow::AddRectToArrayProc(UInt16 message, RgnHandle rgn,
                             const Rect* inDirtyRect, void* inArray)
{
  if (message == kQDRegionToRectsMsgParse) {
    NS_ASSERTION(inArray, "You better pass an array!");
    TRectArray* rectArray = NS_REINTERPRET_CAST(TRectArray*, inArray);

    if (rectArray->mNumRects == rectArray->mCapacity) {
      
      
      return memFullErr;
    }

    rectArray->mRectList[rectArray->mNumRects++] = *inDirtyRect;

    if (rectArray->mNumRects == rectArray->mCapacity) {
      
      
      
      return memFullErr;
    }
  }

  return noErr;
}








void 
nsWindow::PaintUpdateRect(Rect *inDirtyRect, void* inData)
{
  nsWindow* self = NS_REINTERPRET_CAST(nsWindow*, inData);
  Rect dirtyRect = *inDirtyRect;
   
	nsCOMPtr<nsIRenderingContext> renderingContext ( dont_AddRef(self->GetRenderingContext()) );
	if (renderingContext)
	{
        nsRect bounds = self->mBounds;
        self->LocalToWindowCoordinate(bounds);

        
        ::OffsetRect(&dirtyRect, -bounds.x, -bounds.y);
        nsRect rect ( dirtyRect.left, dirtyRect.top, dirtyRect.right - dirtyRect.left,
                        dirtyRect.bottom - dirtyRect.top );

        
        self->UpdateWidget(rect, renderingContext);
  }

} 









nsresult nsWindow::HandleUpdateEvent(RgnHandle regionToValidate)
{
  if (! mVisible || !ContainerHierarchyIsVisible())
    return NS_OK;

  
  StPortSetter    portSetter(mWindowPtr);
  
  StOriginSetter  originSetter(mWindowPtr);
  
  
  StRegionFromPool damagedRgn;
  if (!damagedRgn)
    return NS_ERROR_OUT_OF_MEMORY;
  ::GetPortVisibleRegion(::GetWindowPort(mWindowPtr), damagedRgn);







  
  
  
  StRegionFromPool updateRgn;
  if (!updateRgn)
    return NS_ERROR_OUT_OF_MEMORY;
  ::CopyRgn(mWindowRegion, updateRgn);

  nsRect bounds = mBounds;
  LocalToWindowCoordinate(bounds);
  ::OffsetRgn(updateRgn, bounds.x, bounds.y);

  
  ::SectRgn(damagedRgn, updateRgn, updateRgn);

#ifdef PAINT_DEBUGGING
  if (caps_lock())
    blinkRgn(updateRgn, PR_TRUE);
#endif

  if (!::EmptyRgn(updateRgn)) {
    
    
    

    
    const PRUint32 kMaxUpdateRects = 15;
    
    
    
    const PRUint32 kRectsBeforeBoundingBox = 10;

    
    
    const PRUint32 kRectCapacity = kMaxUpdateRects + 1;

    Rect rectList[kRectCapacity];
    TRectArray rectWrapper(rectList, kRectCapacity);
    ::QDRegionToRects(updateRgn, kQDParseRegionFromTopLeft,
                      sAddRectToArrayProc, &rectWrapper);
    PRUint32 numRects = rectWrapper.mNumRects;

    PRBool painted = PR_FALSE;

    if (numRects <= kMaxUpdateRects) {
      if (numRects > 1) {
        
        
        
        CombineRects(rectWrapper);

        
        numRects = rectWrapper.mNumRects;
      }

      
      
      if (numRects < kRectsBeforeBoundingBox) {
        painted = PR_TRUE;
        for (PRUint32 i = 0 ; i < numRects ; i++)
          PaintUpdateRect(&rectList[i], this);
      }
    }

    if (!painted) {
      
      Rect boundingBox;
      ::GetRegionBounds(updateRgn, &boundingBox);
      PaintUpdateRect(&boundingBox, this);
    }

    
    if (regionToValidate)
      ::CopyRgn(updateRgn, regionToValidate);
  }

  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");

  return NS_OK;
}











void
nsWindow::SortRectsLeftToRight ( TRectArray & inRectArray )
{
  PRInt32 numRects = inRectArray.mNumRects;
  
  for ( int i = 0; i < numRects - 1; ++i ) {
    for ( int j = i+1; j < numRects; ++j ) {
      if ( inRectArray.mRectList[j].left < inRectArray.mRectList[i].left ) {
        Rect temp = inRectArray.mRectList[i];
        inRectArray.mRectList[i] = inRectArray.mRectList[j];
        inRectArray.mRectList[j] = temp;
      }
    }
  }

} 













void
nsWindow::CombineRects ( TRectArray & rectArray )
{
  const float kCombineThresholdRatio = 0.50;      
  
  
  SortRectsLeftToRight ( rectArray );
  
  
  
  
  
  
  
  
  
  
  PRUint32 i = 0;
  while (i < rectArray.mNumRects - 1) {
    Rect* curr = &rectArray.mRectList[i];
    Rect* next = &rectArray.mRectList[i+1];
  
    
    int currArea = (curr->right - curr->left) * (curr->bottom - curr->top);
    int nextArea = (next->right - next->left) * (next->bottom - next->top);

    
    Rect boundingBox;
    ::UnionRect ( curr, next, &boundingBox );
    int boundingRectArea = (boundingBox.right - boundingBox.left) * 
                              (boundingBox.bottom - boundingBox.top);
    
    
    
    if ( (currArea + nextArea) / (float)boundingRectArea > kCombineThresholdRatio ) {
      
      
      
      
      
      *curr = boundingBox;
      for (PRUint32 j = i + 1 ; j < rectArray.mNumRects - 1 ; ++j)
        rectArray.mRectList[j] = rectArray.mRectList[j+1];
      --rectArray.mNumRects;
      
    } 
    else
      ++i;
  } 
  
} 


#pragma mark -






void nsWindow::UpdateWidget(nsRect& aRect, nsIRenderingContext* aContext)
{
	if (! mVisible || !ContainerHierarchyIsVisible())
		return;

	
	nsPaintEvent paintEvent(PR_TRUE, NS_PAINT, this);
	paintEvent.renderingContext = aContext;         
	paintEvent.rect             = &aRect;

	
	StartDraw(aContext);

	if ( OnPaint(paintEvent) ) {
		nsEventStatus	eventStatus;
		DispatchWindowEvent(paintEvent,eventStatus);
		if(eventStatus != nsEventStatus_eIgnore)
			Flash(paintEvent);
	}

	EndDraw();

	
	
	
#ifdef FRONT_TO_BACK
#	define FIRST_CHILD() (mLastChild)
#	define NEXT_CHILD(child) ((child)->GetPrevSibling())
#else
#	define FIRST_CHILD() (mFirstChild)
#	define NEXT_CHILD(child) ((child)->GetNextSibling())
#endif

	
	for (nsIWidget* kid = FIRST_CHILD(); kid; kid = NEXT_CHILD(kid)) {
		nsWindow* childWindow = NS_STATIC_CAST(nsWindow*, kid);

		nsRect childBounds;
		childWindow->GetBounds(childBounds);

		
		nsRect intersection;
		if (intersection.IntersectRect(aRect, childBounds))
		{
			intersection.MoveBy(-childBounds.x, -childBounds.y);
			childWindow->UpdateWidget(intersection, aContext);
		}
	}

#undef FIRST_CHILD
#undef NEXT_CHILD

	NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");
}


















void
nsWindow::ScrollBits ( Rect & inRectToScroll, PRInt32 inLeftDelta, PRInt32 inTopDelta )
{
  ::ScrollWindowRect ( mWindowPtr, &inRectToScroll, inLeftDelta, inTopDelta, 
                        kScrollWindowInvalidate, NULL );
}







NS_IMETHODIMP nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  if (mVisible && ContainerHierarchyIsVisible())
  {
    
    
    
    if (!IsRegionRectangular(mWindowRegion))
    {
      Invalidate(PR_FALSE);
      goto scrollChildren;
    }

    
    
    nsRect scrollRect;  
    if (aClipRect)
      scrollRect = *aClipRect;
    else
    {
      scrollRect = mBounds;
      scrollRect.x = scrollRect.y = 0;
    }

    
    
    if (aDx >= scrollRect.width || aDy >= scrollRect.height)
    {
      Invalidate(scrollRect, PR_FALSE);
    }
    else
    {
      Rect macRect;
      nsRectToMacRect(scrollRect, macRect);

      StartDraw();

      
      
      
      ::SetClip(mWindowRegion);

      
      ScrollBits(macRect,aDx,aDy);

      EndDraw();
    }
  }

scrollChildren:
  
  
  for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
    nsWindow* childWindow = NS_STATIC_CAST(nsWindow*, kid);

    nsRect bounds;
    childWindow->GetBounds(bounds);
    bounds.x += aDx;
    bounds.y += aDy;
    childWindow->SetBounds(bounds);
  }

  
  CalcWindowRegions();

  return NS_OK;
}






NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
  aStatus = nsEventStatus_eIgnore;
	if (mEnabled && !mDestructorCalled)
	{
		nsIWidget* aWidget = event->widget;
		NS_IF_ADDREF(aWidget);
	  
	  if (nsnull != mMenuListener){
	    if(NS_MENU_EVENT == event->eventStructType)
	  	  aStatus = mMenuListener->MenuSelected( static_cast<nsMenuEvent&>(*event) );
	  }
	  if (mEventCallback)
	    aStatus = (*mEventCallback)(event);

		
	  if ((aStatus != nsEventStatus_eConsumeNoDefault) && (mEventListener != nsnull))
	    aStatus = mEventListener->ProcessEvent(*event);

		NS_IF_RELEASE(aWidget);
	}
  return NS_OK;
}


PRBool nsWindow::DispatchWindowEvent(nsGUIEvent &event)
{
  nsEventStatus status;
  DispatchEvent(&event, status);
  return ConvertStatus(status);
}


PRBool nsWindow::DispatchWindowEvent(nsGUIEvent &event,nsEventStatus &aStatus)
{
  DispatchEvent(&event, aStatus);
  return ConvertStatus(aStatus);
}






PRBool nsWindow::DispatchMouseEvent(nsMouseEvent &aEvent)
{

  PRBool result = PR_FALSE;
  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  
  if (mEventCallback && (mEnabled || aEvent.message == NS_MOUSE_EXIT)) {
    result = (DispatchWindowEvent(aEvent));
    return result;
  }

  if (nsnull != mMouseListener) {
    switch (aEvent.message) {
      case NS_MOUSE_MOVE: {
        result = ConvertStatus(mMouseListener->MouseMoved(aEvent));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(aEvent.refPoint.x, aEvent.refPoint.y)) 
        	{
          
          	
            
            
          	
        	} 
        else 
        	{
          
        	}

      } break;

      case NS_MOUSE_BUTTON_DOWN:
        result = ConvertStatus(mMouseListener->MousePressed(aEvent));
        break;

      case NS_MOUSE_BUTTON_UP:
        result = ConvertStatus(mMouseListener->MouseReleased(aEvent));
        result = ConvertStatus(mMouseListener->MouseClicked(aEvent));
        break;
    } 
  } 
  return result;
}

#pragma mark -





PRBool nsWindow::ReportDestroyEvent()
{
	
	nsGUIEvent moveEvent(PR_TRUE, NS_DESTROY, this);
	moveEvent.message			= NS_DESTROY;
	moveEvent.time				= PR_IntervalNow();

	
	return (DispatchWindowEvent(moveEvent));
}





PRBool nsWindow::ReportMoveEvent()
{
	
	nsGUIEvent moveEvent(PR_TRUE, NS_MOVE, this);
	moveEvent.refPoint.x	= mBounds.x;
	moveEvent.refPoint.y	= mBounds.y;
	moveEvent.time				= PR_IntervalNow();

	
	return (DispatchWindowEvent(moveEvent));
}





PRBool nsWindow::ReportSizeEvent()
{
	
	nsSizeEvent sizeEvent(PR_TRUE, NS_SIZE, this);
	sizeEvent.time				= PR_IntervalNow();

	
	sizeEvent.windowSize	= &mBounds;
	sizeEvent.mWinWidth		= mBounds.width;
	sizeEvent.mWinHeight	= mBounds.height;
  
	
	return(DispatchWindowEvent(sizeEvent));
}



#pragma mark -





void nsWindow::CalcWindowRegions()
{
	
	
	if (mWindowRegion == nsnull)
	{
		mWindowRegion = ::NewRgn();
		if (mWindowRegion == nsnull)
			return;
	}
 	::SetRectRgn(mWindowRegion, 0, 0, mBounds.width, mBounds.height);

	
	nsWindow* parent = (nsWindow*)mParent;
	nsPoint origin(-mBounds.x, -mBounds.y);

	
	if (!mIsTopWidgetWindow)
	{
		while (parent && (!parent->mIsTopWidgetWindow))
	{
    if (parent->mWindowRegion)
    {
      
      
      
      
      StRegionFromPool shiftedParentWindowRgn;
      if ( !shiftedParentWindowRgn )
        return;
      ::CopyRgn(parent->mWindowRegion, shiftedParentWindowRgn); 
      ::OffsetRgn(shiftedParentWindowRgn, origin.x, origin.y);
      ::SectRgn(mWindowRegion, shiftedParentWindowRgn, mWindowRegion);
    }
		origin.x -= parent->mBounds.x;
		origin.y -= parent->mBounds.y;
		parent = (nsWindow*)parent->mParent;
		}
	}

	
	
	if (mVisRegion == nsnull)
	{
		mVisRegion = ::NewRgn();
		if (mVisRegion == nsnull)
			return;
	}
	::CopyRgn(mWindowRegion, mVisRegion);

	
	if (mFirstChild)
	{
		StRegionFromPool childRgn;
		if (childRgn != nsnull) {
			nsIWidget* child = mFirstChild;
			do
			{
				nsWindow* childWindow = NS_STATIC_CAST(nsWindow*, child);
					
				PRBool visible;
				childWindow->IsVisible(visible);
				if (visible) {
					nsRect childRect;
					childWindow->GetBounds(childRect);

					Rect macRect;
					::SetRect(&macRect, childRect.x, childRect.y, childRect.XMost(), childRect.YMost());
					::RectRgn(childRgn, &macRect);
					::DiffRgn(mVisRegion, childRgn, mVisRegion);
				}
				
				child = child->GetNextSibling();
			} while (child);
		}
	}
}








PRBool nsWindow::RgnIntersects(RgnHandle aTheRegion, RgnHandle aIntersectRgn)
{
	::SectRgn(aTheRegion, this->mWindowRegion, aIntersectRgn);
	return (::EmptyRgn(aIntersectRgn) != false);
}










 
void
nsWindow::CalcOffset(PRInt32 &aX, PRInt32 &aY)
{
  aX = aY = 0;

  nsIWidget* theParent = GetParent();
  while (theParent)
  {
    nsRect theRect;
    theParent->GetBounds(theRect);
    aX += theRect.x;
    aY += theRect.y;

    theParent = theParent->GetParent();
  }
}


PRBool
nsWindow::ContainerHierarchyIsVisible()
{
  nsIWidget* theParent = GetParent();
  
  while (theParent)
  {
    PRBool  visible;
    theParent->IsVisible(visible);
    if (!visible)
      return PR_FALSE;
    
    theParent = theParent->GetParent();
  }
  
  return PR_TRUE;
}






PRBool nsWindow::PointInWidget(Point aThePoint)
{
	
	nsPoint widgetOrigin(0, 0);
	LocalToWindowCoordinate(widgetOrigin);

	
	nsRect widgetRect;
	GetBounds(widgetRect);

	
	widgetRect.MoveBy(widgetOrigin.x, widgetOrigin.y);

	
	return(widgetRect.Contains(aThePoint.h, aThePoint.v));
}








nsWindow*  nsWindow::FindWidgetHit(Point aThePoint)
{
	if (!mVisible || !ContainerHierarchyIsVisible() || !PointInWidget(aThePoint))
		return nsnull;

	nsWindow* widgetHit = this;

	
	for (nsIWidget* kid = mLastChild; kid; kid = kid->GetPrevSibling()) {
		nsWindow* childWindow = NS_STATIC_CAST(nsWindow*, kid);
		
		nsWindow* deeperHit = childWindow->FindWidgetHit(aThePoint);
		if (deeperHit)
		{
			widgetHit = deeperHit;
			break;
		}
	}

	return widgetHit;
}

#pragma mark -










NS_IMETHODIMP nsWindow::WidgetToScreen(const nsRect& aLocalRect, nsRect& aGlobalRect)
{	
	aGlobalRect = aLocalRect;
	nsIWidget* theParent = this->GetParent();
	if ( theParent ) {
		
		
		
		theParent->WidgetToScreen(aLocalRect, aGlobalRect);

		
		nsRect myBounds;
		GetBounds(myBounds);
		aGlobalRect.MoveBy(myBounds.x, myBounds.y);
	}
	else {
		
		
		
		
		StPortSetter	portSetter(mWindowPtr);
		StOriginSetter	originSetter(mWindowPtr);
		
		
		Point origin = {0, 0};
		::LocalToGlobal ( &origin );
		aGlobalRect.MoveBy ( origin.h, origin.v );
	}
	
	return NS_OK;
}









NS_IMETHODIMP nsWindow::ScreenToWidget(const nsRect& aGlobalRect, nsRect& aLocalRect)
{
	aLocalRect = aGlobalRect;
	nsIWidget* theParent = GetParent();
	if ( theParent ) {
		
		
		
		theParent->WidgetToScreen(aGlobalRect, aLocalRect);
	  
		
		nsRect myBounds;
		GetBounds(myBounds);
		aLocalRect.MoveBy(myBounds.x, myBounds.y);
	}
	else {
		
		
		
		
		StPortSetter	portSetter(mWindowPtr);
		StOriginSetter	originSetter(mWindowPtr);
		
		
		Point origin = {0, 0};
		::GlobalToLocal ( &origin );
		aLocalRect.MoveBy ( origin.h, origin.v );
	}
	
	return NS_OK;
} 









void nsWindow::nsRectToMacRect(const nsRect& aRect, Rect& aMacRect)
{
		aMacRect.left = aRect.x;
		aMacRect.top = aRect.y;
		aMacRect.right = aRect.x + aRect.width;
		aMacRect.bottom = aRect.y + aRect.height;
}









void
nsWindow::ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY)
{
	PRInt32	offX, offY;
	CalcOffset(offX,offY);

	aX += offX;
	aY += offY;
}

NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener, 
                                            PRBool aDoCapture, 
                                            PRBool aConsumeRollupEvent)
{
  if (aDoCapture) {
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    gRollupListener = aListener;
    NS_ADDREF(aListener);
    gRollupWidget = this;
    NS_ADDREF(this);
  } else {
    NS_IF_RELEASE(gRollupListener);
    
    NS_IF_RELEASE(gRollupWidget);
  }

  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetTitle(const nsAString& title)
{
  NS_ERROR("Would some Mac person please implement me? Thanks.");
  return NS_OK;
}

NS_IMETHODIMP nsWindow::GetAttention(PRInt32 aCycleCount)
{
        
	
	
	
	
	
  
  OSErr err;
    
	if (we_are_front_process())
		return NS_OK;
  
	if (gNotificationInstalled)
	{
		(void)::NMRemove(&gNMRec);
		gNotificationInstalled = false;
	}
	
	err = GetIconSuite( &gNMRec.nmIcon, 128, svAllSmallData );
	if ( err != noErr )
		gNMRec.nmIcon = NULL;
		
	
	gNMRec.qType    = nmType;
	gNMRec.nmMark   = 1;      
	gNMRec.nmSound  = NULL;   
	gNMRec.nmStr    = NULL;   
	gNMRec.nmResp   = NULL;   
	gNMRec.nmRefCon = 0;
	if (::NMInstall(&gNMRec) == noErr)
		gNotificationInstalled = true;

	return NS_OK;
}

#pragma mark -


NS_IMETHODIMP nsWindow::GetPluginClipRect(nsRect& outClipRect, nsPoint& outOrigin, PRBool& outWidgetVisible)
{
  PRBool isVisible = mVisible;

  nsRect widgetClipRect = mBounds;
  
  nscoord absX = widgetClipRect.x;
  nscoord absY = widgetClipRect.y;

  nscoord ancestorX = -widgetClipRect.x;
  nscoord ancestorY = -widgetClipRect.y;

  
  widgetClipRect.x = 0;
  widgetClipRect.y = 0;

  
  nsIWidget* widget = GetParent();
  while (widget)
  {
    if (isVisible)
      widget->IsVisible(isVisible);

    nsRect widgetRect;
    widget->GetClientBounds(widgetRect);
    nscoord wx = widgetRect.x;
    nscoord wy = widgetRect.y;

    widgetRect.x = ancestorX;
    widgetRect.y = ancestorY;

    widgetClipRect.IntersectRect(widgetClipRect, widgetRect);
    absX += wx;
    absY += wy;
    widget = widget->GetParent();
    if (!widget)
    {
      
      
      absX -= wx;
      absY -= wy;
    }
    ancestorX -= wx;
    ancestorY -= wy;
  }

  widgetClipRect.x += absX;
  widgetClipRect.y += absY;

  outClipRect = widgetClipRect;
  outOrigin.x = absX;
  outOrigin.y = absY;
  
  outWidgetVisible = isVisible;
  return NS_OK;
}


NS_IMETHODIMP nsWindow::StartDrawPlugin(void)
{
  
  GrafPtr savePort;
  ::GetPort(&savePort);  
  ::SetPortWindowPort(mWindowPtr);
  
  RGBColor macColor;
  macColor.red   = COLOR8TOCOLOR16(NS_GET_R(mBackground));  
  macColor.green = COLOR8TOCOLOR16(NS_GET_G(mBackground));
  macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(mBackground));
  ::RGBBackColor(&macColor);

  ::SetPort(savePort);  

  return NS_OK;
}

NS_IMETHODIMP nsWindow::EndDrawPlugin(void)
{
  GrafPtr savePort;
  ::GetPort(&savePort);  
  ::SetPortWindowPort(mWindowPtr);

  RGBColor rgbWhite = { 0xFFFF, 0xFFFF, 0xFFFF };
  ::RGBBackColor(&rgbWhite);

  ::SetPort(savePort);  
  return NS_OK;
}


#pragma mark -


NS_IMETHODIMP nsWindow::ResetInputState()
{
	
	
  nsIWidget* parent = GetParent();
  NS_ASSERTION(parent, "cannot get parent");
  if (parent)
  {
    nsCOMPtr<nsIKBStateControl> kb = do_QueryInterface(parent);
    NS_ASSERTION(kb, "cannot get parent");
  	if (kb) {
  		return kb->ResetInputState();
  	}
  }
	return NS_ERROR_ABORT;
}

NS_IMETHODIMP nsWindow::SetIMEOpenState(PRBool aState) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindow::GetIMEOpenState(PRBool* aState) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindow::SetIMEEnabled(PRUint32 aState) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindow::GetIMEEnabled(PRUint32* aState) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindow::CancelIMEComposition() {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindow::GetToggledKeyState(PRUint32 aKeyCode,
                                           PRBool* aLEDState) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

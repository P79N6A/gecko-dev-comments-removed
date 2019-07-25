


































































#include "nsWindow.h"
#include "os2FrameWindow.h"
#include "gfxContext.h"
#include "gfxOS2Surface.h"
#include "imgIContainer.h"
#include "npapi.h"
#include "nsDragService.h"
#include "nsGfxCIID.h"
#include "nsHashKeys.h"
#include "nsIMenuRollup.h"
#include "nsIRollupListener.h"
#include "nsIScreenManager.h"
#include "nsOS2Uni.h"
#include "nsTHashtable.h"
#include "nsToolkit.h"
#include "nsWidgetAtoms.h"
#include "wdgtos2rc.h"

#include "mozilla/Preferences.h"

using namespace mozilla;








#define ACTION_PAINT    1
#define ACTION_DRAW     2
#define ACTION_SCROLL   3
#define ACTION_SHOW     4
#define ACTION_PTRPOS   5


#define DND_None                (nsIDragSessionOS2::DND_NONE)
#define DND_NativeDrag          (nsIDragSessionOS2::DND_NATIVEDRAG)
#define DND_MozDrag             (nsIDragSessionOS2::DND_MOZDRAG)
#define DND_InDrop              (nsIDragSessionOS2::DND_INDROP)
#define DND_DragStatus          (nsIDragSessionOS2::DND_DRAGSTATUS)
#define DND_DispatchEnterEvent  (nsIDragSessionOS2::DND_DISPATCHENTEREVENT)
#define DND_DispatchEvent       (nsIDragSessionOS2::DND_DISPATCHEVENT)
#define DND_GetDragoverResult   (nsIDragSessionOS2::DND_GETDRAGOVERRESULT)
#define DND_ExitSession         (nsIDragSessionOS2::DND_EXITSESSION)




#define WM_APPCOMMAND                   0x0319

#define APPCOMMAND_BROWSER_BACKWARD     1
#define APPCOMMAND_BROWSER_FORWARD      2
#define APPCOMMAND_BROWSER_REFRESH      3
#define APPCOMMAND_BROWSER_STOP         4





#define PMSCAN_PADMULT      0x37
#define PMSCAN_PAD7         0x47
#define PMSCAN_PAD8         0x48
#define PMSCAN_PAD9         0x49
#define PMSCAN_PADMINUS     0x4A
#define PMSCAN_PAD4         0x4B
#define PMSCAN_PAD5         0x4C
#define PMSCAN_PAD6         0x4D
#define PMSCAN_PADPLUS      0x4E
#define PMSCAN_PAD1         0x4F
#define PMSCAN_PAD2         0x50
#define PMSCAN_PAD3         0x51
#define PMSCAN_PAD0         0x52
#define PMSCAN_PADPERIOD    0x53
#define PMSCAN_PADDIV       0x5c

#define isNumPadScanCode(scanCode) !((scanCode < PMSCAN_PAD7) ||      \
                                     (scanCode > PMSCAN_PADPERIOD) || \
                                     (scanCode == PMSCAN_PADMULT) ||  \
                                     (scanCode == PMSCAN_PADDIV) ||   \
                                     (scanCode == PMSCAN_PADMINUS) || \
                                     (scanCode == PMSCAN_PADPLUS))

#define isNumlockOn     (WinGetKeyState(HWND_DESKTOP, VK_NUMLOCK) & 0x0001)
#define isKeyDown(vk)   ((WinGetKeyState(HWND_DESKTOP,vk) & 0x8000) == 0x8000)





#define XFROMMP(m)      (SHORT(LOUSHORT(m)))
#define YFROMMP(m)      (SHORT(HIUSHORT(m)))


#define PM2NS_PARENT NS2PM_PARENT
#define PM2NS NS2PM


#define NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION \
                        "MozillaPluginWindowPropertyAssociation"


#define kClipWndClass   "nsClipWnd"




#ifdef DEBUG_FOCUS
  #define DEBUGFOCUS(what) fprintf(stderr, "[%8x]  %8lx  (%02d)  "#what"\n", \
                                   (int)this, mWnd, mWindowIdentifier)
#else
  #define DEBUGFOCUS(what)
#endif






nsIRollupListener*  gRollupListener           = 0;
nsIMenuRollup*      gMenuRollup               = 0;
nsIWidget*          gRollupWidget             = 0;
PRBool              gRollupConsumeRollupEvent = PR_FALSE;


PRUint32            gOS2Flags = 0;


static HPOINTER     sPtrArray[IDC_COUNT];


static POINTS       sLastButton1Down = {0,0};


static PRUint32     sDragStatus = 0;

#ifdef DEBUG_FOCUS
  int currentWindowIdentifier = 0;
#endif



static PRUint32     WMChar2KeyCode(MPARAM mp1, MPARAM mp2);





nsWindow::nsWindow() : nsBaseWidget()
{
  mWnd                = 0;
  mParent             = 0;
  mFrame              = 0;
  mWindowType         = eWindowType_toplevel;
  mBorderStyle        = eBorderStyle_default;
  mWindowState        = nsWindowState_ePrecreate;
  mOnDestroyCalled    = PR_FALSE;
  mIsDestroying       = PR_FALSE;
  mInSetFocus         = PR_FALSE;
  mNoPaint            = PR_FALSE;
  mDragHps            = 0;
  mDragStatus         = 0;
  mClipWnd            = 0;
  mCssCursorHPtr      = 0;
  mThebesSurface      = 0;

  if (!gOS2Flags) {
    InitGlobals();
  }
}



nsWindow::~nsWindow()
{
  
  
  

  
  
  

  mIsDestroying = PR_TRUE;

  if (mCssCursorHPtr) {
    WinDestroyPointer(mCssCursorHPtr);
    mCssCursorHPtr = 0;
  }

  
  
  if (!(mWindowState & nsWindowState_eDead)) {
    mWindowState |= nsWindowState_eDoingDelete;
    mWindowState &= ~(nsWindowState_eLive | nsWindowState_ePrecreate |
                      nsWindowState_eInCreate);
    Destroy();
  }

  
  
  if (mClipWnd) {
    WinDestroyWindow(mClipWnd);
    mClipWnd = 0;
  }
 
  
  if (mFrame) {
    delete mFrame;
    mFrame = 0;
  }
}





void nsWindow::InitGlobals()
{
  gOS2Flags = kIsInitialized;

  
  WinRegisterClass(0, kWindowClassName, fnwpNSWindow, 0, 8);

  
  WinRegisterClass(0, kClipWndClass, WinDefWindowProc, 0, 4);

  
  HMODULE hModResources = 0;
  DosQueryModFromEIP(&hModResources, 0, 0, 0, 0, (ULONG)&gOS2Flags);
  for (int i = 0; i < IDC_COUNT; i++) {
    sPtrArray[i] = WinLoadPointer(HWND_DESKTOP, hModResources, IDC_BASE+i);
  }

  
  char buffer[16];
  COUNTRYCODE cc = { 0 };
  DosQueryDBCSEnv(sizeof(buffer), &cc, buffer);
  if (buffer[0] || buffer[1]) {
    gOS2Flags |= kIsDBCS;
  }

  
  
  
  
  
  if (Preferences::GetBool("os2.trackpoint", PR_FALSE)) {
    gOS2Flags |= kIsTrackPoint;
  }
}





void nsWindow::ReleaseGlobals()
{
  for (int i = 0; i < IDC_COUNT; i++) {
    WinDestroyPointer(sPtrArray[i]);
  }
}




NS_METHOD nsWindow::Create(nsIWidget* aParent,
                           nsNativeWidget aNativeParent,
                           const nsIntRect& aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsDeviceContext* aContext,
                           nsIAppShell* aAppShell,
                           nsIToolkit* aToolkit,
                           nsWidgetInitData* aInitData)
{
  mWindowState = nsWindowState_eInCreate;

  
  
  
  HWND      hParent;
  nsWindow* pParent;
  if (aParent) {
    hParent = (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW);
    pParent = (nsWindow*)aParent;
  } else {
    if (aNativeParent && (HWND)aNativeParent != HWND_DESKTOP) {
      hParent = (HWND)aNativeParent;
      pParent = GetNSWindowPtr(hParent);
    } else {
      hParent = HWND_DESKTOP;
      pParent = 0;
    }
  }

  BaseCreate(aParent, aRect, aHandleEventFunction,
             aContext, aAppShell, aToolkit, aInitData);


#ifdef DEBUG_FOCUS
  mWindowIdentifier = currentWindowIdentifier;
  currentWindowIdentifier++;
#endif

  
  if (aInitData) {
    
    
    if (mWindowType == eWindowType_toplevel ||
        mWindowType == eWindowType_invisible) {
      mNoPaint = PR_TRUE;
    }
    
    else if (mWindowType == eWindowType_popup) {
      pParent = 0;
    }
  }

  
  
  
  if (mWindowType == eWindowType_toplevel ||
      mWindowType == eWindowType_dialog   ||
      mWindowType == eWindowType_invisible) {
    mFrame = new os2FrameWindow(this);
    NS_ENSURE_TRUE(mFrame, NS_ERROR_FAILURE);
    mWnd = mFrame->CreateFrameWindow(pParent, hParent, aRect,
                                     mWindowType, mBorderStyle);
    NS_ENSURE_TRUE(mWnd, NS_ERROR_FAILURE);
  } else {
    nsresult rv = CreateWindow(pParent, hParent, aRect, aInitData);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  SetNSWindowPtr(mWnd, this);

  
  nsGUIEvent event(PR_TRUE, NS_CREATE, this);
  InitEvent(event);
  DispatchWindowEvent(&event);

  mWindowState = nsWindowState_eLive;
  return NS_OK;
}




nsresult nsWindow::CreateWindow(nsWindow* aParent,
                                HWND aParentWnd,
                                const nsIntRect& aRect,
                                nsWidgetInitData* aInitData)
{
  
  HWND hOwner = 0;
  if (mWindowType == eWindowType_popup && aParentWnd != HWND_DESKTOP) {
    hOwner = aParentWnd;
    aParentWnd = HWND_DESKTOP;
  }

  
  
  PRUint32 style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  if (aInitData && !aInitData->clipSiblings) {
    style &= ~WS_CLIPSIBLINGS;
  }

  
  mWnd = WinCreateWindow(aParentWnd,
                         kWindowClassName,
                         0,
                         style,
                         0, 0, 0, 0,
                         hOwner,
                         HWND_TOP,
                         0,
                         0, 0);
  NS_ENSURE_TRUE(mWnd, NS_ERROR_FAILURE);

  
  
  if ((gOS2Flags & kIsTrackPoint) && mWindowType == eWindowType_child) {
    WinCreateWindow(mWnd, WC_SCROLLBAR, 0, SBS_VERT,
                    0, 0, 0, 0, mWnd, HWND_TOP,
                    FID_VERTSCROLL, 0, 0);
  }

  
  mBounds = aRect;
  nsIntRect parRect;
  if (aParent) {
    aParent->GetBounds(parRect);
  } else {
    parRect.height = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
  }
  WinSetWindowPos(mWnd, 0,
                  aRect.x, parRect.height - aRect.y - aRect.height,
                  aRect.width, aRect.height, SWP_SIZE | SWP_MOVE);

  
  
  mParent = aParent;
  if (mParent) {
    mParent->AddChild(this);
  }

  DEBUGFOCUS(Create nsWindow);
  return NS_OK;
}




NS_METHOD nsWindow::Destroy()
{
  
  
  if ((mWindowState & nsWindowState_eLive) && mParent) {
    nsBaseWidget::Destroy();
  }

  
  
  if (this == gRollupWidget) {
    if (gRollupListener) {
      gRollupListener->Rollup(PR_UINT32_MAX, nsnull);
    }
    CaptureRollupEvents(nsnull, nsnull, PR_FALSE, PR_TRUE);
  }

  HWND hMain = GetMainWindow();
  if (hMain) {
    DEBUGFOCUS(Destroy);
    if (hMain == WinQueryFocus(HWND_DESKTOP)) {
      WinSetFocus(HWND_DESKTOP, WinQueryWindow(hMain, QW_PARENT));
    }
    WinDestroyWindow(hMain);
  }
  return NS_OK;
}








inline HWND nsWindow::GetMainWindow()
{
  return mFrame ? mFrame->GetFrameWnd() : mWnd;
}





inline nsWindow* nsWindow::GetNSWindowPtr(HWND aWnd)
{
  return (nsWindow*)WinQueryWindowPtr(aWnd, QWL_NSWINDOWPTR);
}




inline PRBool nsWindow::SetNSWindowPtr(HWND aWnd, nsWindow* aPtr)
{
  return WinSetWindowPtr(aWnd, QWL_NSWINDOWPTR, aPtr);
}



nsIWidget* nsWindow::GetParent()
{
  
  
  if (mFrame || mIsDestroying || mOnDestroyCalled ||
      !mParent || mParent->mIsDestroying) {
    return 0;
  }

  return mParent;
}



NS_METHOD nsWindow::Enable(PRBool aState)
{
  HWND hMain = GetMainWindow();
  if (hMain) {
    WinEnableWindow(hMain, aState);
  }
  return NS_OK;
}



NS_METHOD nsWindow::IsEnabled(PRBool* aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  HWND hMain = GetMainWindow();
  *aState = !hMain || WinIsWindowEnabled(hMain);
  return NS_OK;
}



NS_METHOD nsWindow::Show(PRBool aState)
{
  if (mFrame) {
    return mFrame->Show(aState);
  }
  if (mWnd) {
    if (aState) {
      
      
      if (CheckDragStatus(ACTION_SHOW, 0)) {
        PRBool isVisible;
        IsVisible(isVisible);
        if (!isVisible) {
          PlaceBehind(eZPlacementTop, 0, PR_FALSE);
        }
        WinShowWindow(mWnd, PR_TRUE);
      }
    } else {
      WinShowWindow(mWnd, PR_FALSE);
    }
  }

  return NS_OK;
}



NS_METHOD nsWindow::IsVisible(PRBool& aState)
{
  aState = WinIsWindowVisible(GetMainWindow()) ? PR_TRUE : PR_FALSE;
  return NS_OK;
}



NS_METHOD nsWindow::SetFocus(PRBool aRaise)
{
  
  if (mWnd) {
    if (!mInSetFocus) {
      DEBUGFOCUS(SetFocus);
      mInSetFocus = PR_TRUE;
      WinSetFocus(HWND_DESKTOP, mWnd);
      mInSetFocus = PR_FALSE;
    }
  }
  return NS_OK;
}



NS_METHOD nsWindow::Invalidate(const nsIntRect& aRect, PRBool aIsSynchronous)
{
  if (mWnd) {
    RECTL rcl = {aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height};
    NS2PM(rcl);
    WinInvalidateRect(mWnd, &rcl, PR_FALSE);
#if 0
    if (aIsSynchronous) {
      Update();
    }
#endif
  }
  return NS_OK;
}




NS_IMETHODIMP nsWindow::Update()
{
  if (mWnd) {
    WinUpdateWindow(mWnd);
  }
  return NS_OK;
}




gfxASurface* nsWindow::GetThebesSurface()
{
  if (mWnd && !mThebesSurface) {
    mThebesSurface = new gfxOS2Surface(mWnd);
  }
  return mThebesSurface;
}






gfxASurface* nsWindow::ConfirmThebesSurface()
{
  if (!mThebesSurface && !mNoPaint && mWnd) {
    mThebesSurface = new gfxOS2Surface(mWnd);
  }
  return mThebesSurface;
}



float nsWindow::GetDPI()
{
  static PRInt32 sDPI = 0;

  
  
  if (!sDPI) {
    HDC dc = DevOpenDC(0, OD_MEMORY,"*",0L, 0, 0);
    if (dc > 0) {
      LONG lDPI;
      if (DevQueryCaps(dc, CAPS_VERTICAL_FONT_RES, 1, &lDPI))
        sDPI = lDPI;
      DevCloseDC(dc);
    }
    if (sDPI <= 0) {
      sDPI = 96;
    }
  }
  return sDPI;  
}




void* nsWindow::GetNativeData(PRUint32 aDataType)
{
  switch(aDataType) {
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_PLUGIN_PORT:
      return (void*)mWnd;

    
    
    case NS_NATIVE_GRAPHIC: {
      HPS hps = 0;
      CheckDragStatus(ACTION_DRAW, &hps);
      if (!hps) {
        hps = WinGetPS(mWnd);
      }
      return (void*)hps;
    }
  }

  return 0;
}



void nsWindow::FreeNativeData(void* data, PRUint32 aDataType)
{
  
  if (aDataType == NS_NATIVE_GRAPHIC &&
      data &&
      !ReleaseIfDragHPS((HPS)data)) {
    WinReleasePS((HPS)data);
  }
}



NS_METHOD nsWindow::CaptureMouse(PRBool aCapture)
{
  if (aCapture) {
    WinSetCapture(HWND_DESKTOP, mWnd);
  } else {
    WinSetCapture(HWND_DESKTOP, 0);
  }
  return NS_OK;
}



PRBool nsWindow::HasPendingInputEvent()
{
  return (WinQueryQueueStatus(HWND_DESKTOP) & (QS_KEY | QS_MOUSE)) != 0;
}








NS_METHOD nsWindow::GetBounds(nsIntRect& aRect)
{
  if (mFrame) {
    return mFrame->GetBounds(aRect);
  }
  aRect = mBounds;
  return NS_OK;
}





NS_METHOD nsWindow::GetClientBounds(nsIntRect& aRect)
{
  aRect.x = 0;
  aRect.y = 0;
  aRect.width = mBounds.width;
  aRect.height = mBounds.height;
  return NS_OK;
}



nsIntPoint nsWindow::WidgetToScreenOffset()
{
  POINTL point = { 0, 0 };
  NS2PM(point);

  WinMapWindowPoints(mWnd, HWND_DESKTOP, &point, 1);
  return nsIntPoint(point.x,
                    WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - point.y - 1);
}





void nsWindow::NS2PM(POINTL& ptl)
{
  ptl.y = mBounds.height - ptl.y - 1;
}


void nsWindow::NS2PM(RECTL& rcl)
{
  LONG height = rcl.yTop - rcl.yBottom;
  rcl.yTop = mBounds.height - rcl.yBottom;
  rcl.yBottom = rcl.yTop - height;
}


void nsWindow::NS2PM_PARENT(POINTL& ptl)
{
  if (mParent) {
    mParent->NS2PM(ptl);
  } else {
    HWND hParent = WinQueryWindow(mWnd, QW_PARENT);
    SWP  swp;
    WinQueryWindowPos(hParent, &swp);
    ptl.y = swp.cy - ptl.y - 1;
  }
}



NS_METHOD nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
  if (mFrame) {
    return mFrame->Move(aX, aY);
  }
  Resize(aX, aY, mBounds.width, mBounds.height, PR_FALSE);
  return NS_OK;
}



NS_METHOD nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  if (mFrame) {
    return mFrame->Resize(aWidth, aHeight, aRepaint);
  }
  Resize(mBounds.x, mBounds.y, aWidth, aHeight, aRepaint);
  return NS_OK;
}



NS_METHOD nsWindow::Resize(PRInt32 aX, PRInt32 aY,
                           PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  if (mFrame) {
    return mFrame->Resize(aX, aY, aWidth, aHeight, aRepaint);
  }

  
  
  

  if (!mWnd ||
      mWindowType == eWindowType_child ||
      mWindowType == eWindowType_plugin) {
    mBounds.x      = aX;
    mBounds.y      = aY;
    mBounds.width  = aWidth;
    mBounds.height = aHeight;
  }

  
  
  if (mWnd) {
    POINTL ptl = { aX, aY };
    NS2PM_PARENT(ptl);
    ptl.y -= aHeight - 1;

    
    if (mWindowType == eWindowType_popup) {
      ptl.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - aHeight - 1 - aY;
    }
    else if (mParent) {
      WinMapWindowPoints(mParent->mWnd, WinQueryWindow(mWnd, QW_PARENT),
                         &ptl, 1);
    }

    if (!WinSetWindowPos(mWnd, 0, ptl.x, ptl.y, aWidth, aHeight,
                         SWP_MOVE | SWP_SIZE) && aRepaint) {
      WinInvalidateRect(mWnd, 0, FALSE);
    }
  }

  return NS_OK;
}



NS_METHOD nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                nsIWidget* aWidget, PRBool aActivate)
{
  HWND hBehind = HWND_TOP;

  if (aPlacement == eZPlacementBottom) {
    hBehind = HWND_BOTTOM;
  } else
  if (aPlacement == eZPlacementBelow && aWidget) {
    hBehind = (static_cast<nsWindow*>(aWidget))->GetMainWindow();
  }

  PRUint32 flags = SWP_ZORDER;
  if (aActivate) {
    flags |= SWP_ACTIVATE;
  }

  WinSetWindowPos(GetMainWindow(), hBehind, 0, 0, 0, 0, flags);
  return NS_OK;
}




NS_METHOD nsWindow::SetZIndex(PRInt32 aZIndex)
{
  
  
  
  return NS_OK;
}








void nsWindow::ActivatePlugin(HWND aWnd)
{
  
  static PRBool inPluginActivate = FALSE;
  if (inPluginActivate) {
    return;
  }

  
  
  if (!WinQueryProperty(mWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION)) {
    return;
  }

  
  inPluginActivate = TRUE;
  DEBUGFOCUS(NS_PLUGIN_ACTIVATE);
  DispatchActivationEvent(NS_PLUGIN_ACTIVATE);

  
  
  
  
  
  
  HWND hFocus = 0;
  if (WinIsChild(aWnd, mWnd)) {
    hFocus = aWnd;
  } else {
    hFocus = WinQueryWindow(mWnd, QW_TOP);
    if (hFocus) {
      PID pidFocus, pidThis;
      TID tid;
      WinQueryWindowProcess(hFocus, &pidFocus, &tid);
      WinQueryWindowProcess(mWnd, &pidThis, &tid);
      if (pidFocus != pidThis) {
        hFocus = 0;
      }
    }
  }
  if (hFocus) {
    WinSetFocus(HWND_DESKTOP, hFocus);
  }

  inPluginActivate = FALSE;
  return;
}





nsresult nsWindow::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
  for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
    const Configuration& configuration = aConfigurations[i];
    nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
    NS_ASSERTION(w->GetParent() == this,
                 "Configured widget is not a child");
    w->SetPluginClipRegion(configuration);
  }
  return NS_OK;
}











void nsWindow::SetPluginClipRegion(const Configuration& aConfiguration)
{
  NS_ASSERTION((mParent && mParent->mWnd), "Child window has no parent");

  
  if (!StoreWindowClipRegion(aConfiguration.mClipRegion) &&
      mBounds.IsEqualInterior(aConfiguration.mBounds)) {
    return;
  }

  
  
  mBounds.MoveTo(aConfiguration.mBounds.TopLeft());

  
  HWND hClip = GetPluginClipWindow(mParent->mWnd);
  NS_ASSERTION(hClip, "No clipping window for plugin");
  if (!hClip) {
    return;
  }

  
  const nsTArray<nsIntRect>& rects = aConfiguration.mClipRegion;
  nsIntRect r;
  for (PRUint32 i = 0; i < rects.Length(); ++i) {
    r.UnionRect(r, rects[i]);
  }

  
  SWP    swp;
  POINTL ptl;
  WinQueryWindowPos(hClip, &swp);
  ptl.x = aConfiguration.mBounds.x + r.x;
  ptl.y = mParent->mBounds.height
          - (aConfiguration.mBounds.y + r.y + r.height);

  ULONG  clipFlags = 0;
  if (swp.x != ptl.x || swp.y != ptl.y) {
    clipFlags |= SWP_MOVE;
  }
  if (swp.cx != r.width || swp.cy != r.height) {
    clipFlags |= SWP_SIZE;
  }
  if (clipFlags) {
    WinSetWindowPos(hClip, 0, ptl.x, ptl.y, r.width, r.height, clipFlags);
  }

  
  
  
  WinQueryWindowPos(mWnd, &swp);
  ptl.x = -r.x;
  ptl.y = r.height + r.y - aConfiguration.mBounds.height;

  ULONG  wndFlags = 0;
  if (swp.x != ptl.x || swp.y != ptl.y) {
    wndFlags |= SWP_MOVE;
  }
  if (mBounds.Size() != aConfiguration.mBounds.Size()) {
    wndFlags |= SWP_SIZE;
  }
  if (wndFlags) {
    WinSetWindowPos(mWnd, 0, ptl.x, ptl.y,
                    aConfiguration.mBounds.width,
                    aConfiguration.mBounds.height, wndFlags);
  }

  
  
  if (wndFlags & SWP_SIZE) {
    HWND hChild = WinQueryWindow(mWnd, QW_TOP);
    if (hChild) {
      WinSetWindowPos(hChild, 0, 0, 0,
                      aConfiguration.mBounds.width,
                      aConfiguration.mBounds.height,
                      SWP_MOVE | SWP_SIZE);
    }
  }

  
  
  if (clipFlags & SWP_SIZE) {
    WinInvalidateRect(mWnd, 0, TRUE);
    WinUpdateWindow(mWnd);
  }
}






HWND nsWindow::GetPluginClipWindow(HWND aParentWnd)
{
  if (mClipWnd) {
    return mClipWnd;
  }

  
  mClipWnd = WinCreateWindow(aParentWnd, kClipWndClass, "",
                             WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             0, 0, 0, 0, 0, mWnd, 0, 0, 0);
  if (mClipWnd) {
    if (!WinSetParent(mWnd, mClipWnd, FALSE)) {
      WinDestroyWindow(mClipWnd);
      mClipWnd = 0;
    }
  }

  return mClipWnd;
}









void nsWindow::ActivateTopLevelWidget()
{
  if (mFrame) {
    mFrame->ActivateTopLevelWidget();
  } else {
    nsWindow* top = static_cast<nsWindow*>(GetTopLevelWidget());
    if (top && top->mFrame) {
      top->mFrame->ActivateTopLevelWidget();
    }
  }
  return;
}






NS_IMETHODIMP nsWindow::SetSizeMode(PRInt32 aMode)
{
  NS_ENSURE_TRUE(mFrame, NS_ERROR_UNEXPECTED);
  return mFrame->SetSizeMode(aMode);
}

NS_IMETHODIMP nsWindow::HideWindowChrome(PRBool aShouldHide)
{
  NS_ENSURE_TRUE(mFrame, NS_ERROR_UNEXPECTED);
  return mFrame->HideWindowChrome(aShouldHide);
}

NS_METHOD nsWindow::SetTitle(const nsAString& aTitle)
{
  NS_ENSURE_TRUE(mFrame, NS_ERROR_UNEXPECTED);
  return mFrame->SetTitle(aTitle);
}

NS_METHOD nsWindow::SetIcon(const nsAString& aIconSpec)
{
  NS_ENSURE_TRUE(mFrame, NS_ERROR_UNEXPECTED);
  return mFrame->SetIcon(aIconSpec);
}

NS_METHOD nsWindow::ConstrainPosition(PRBool aAllowSlop,
                                      PRInt32* aX, PRInt32* aY)
{
  NS_ENSURE_TRUE(mFrame, NS_ERROR_UNEXPECTED);
  return mFrame->ConstrainPosition(aAllowSlop, aX, aY);
}







NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  HPOINTER newPointer = 0;

  switch (aCursor) {
    case eCursor_select:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_TEXT, FALSE);
      break;

    case eCursor_wait:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);
      break;

    case eCursor_hyperlink:
      newPointer = sPtrArray[IDC_SELECTANCHOR-IDC_BASE];
      break;

    case eCursor_standard:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE);
      break;

    case eCursor_n_resize:
    case eCursor_s_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENS, FALSE);
      break;

    case eCursor_w_resize:
    case eCursor_e_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZEWE, FALSE);
      break;

    case eCursor_nw_resize:
    case eCursor_se_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENWSE, FALSE);
      break;

    case eCursor_ne_resize:
    case eCursor_sw_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENESW, FALSE);
      break;

    case eCursor_crosshair:
      newPointer = sPtrArray[IDC_CROSS-IDC_BASE];
      break;

    case eCursor_move:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_MOVE, FALSE);
      break;

    case eCursor_help:
      newPointer = sPtrArray[IDC_HELP-IDC_BASE];
      break;

    case eCursor_copy: 
      newPointer = sPtrArray[IDC_COPY-IDC_BASE];
      break;

    case eCursor_alias:
      newPointer = sPtrArray[IDC_ALIAS-IDC_BASE];
      break;

    case eCursor_cell:
      newPointer = sPtrArray[IDC_CELL-IDC_BASE];
      break;

    case eCursor_grab:
      newPointer = sPtrArray[IDC_GRAB-IDC_BASE];
      break;

    case eCursor_grabbing:
      newPointer = sPtrArray[IDC_GRABBING-IDC_BASE];
      break;

    case eCursor_spinning:
      newPointer = sPtrArray[IDC_ARROWWAIT-IDC_BASE];
      break;

    case eCursor_context_menu:
      
      break;

    case eCursor_zoom_in:
      newPointer = sPtrArray[IDC_ZOOMIN-IDC_BASE];
      break;

    case eCursor_zoom_out:
      newPointer = sPtrArray[IDC_ZOOMOUT-IDC_BASE];
      break;

    case eCursor_not_allowed:
    case eCursor_no_drop:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_ILLEGAL, FALSE);
      break;

    case eCursor_col_resize:
      newPointer = sPtrArray[IDC_COLRESIZE-IDC_BASE];
      break;

    case eCursor_row_resize:
      newPointer = sPtrArray[IDC_ROWRESIZE-IDC_BASE];
      break;

    case eCursor_vertical_text:
      newPointer = sPtrArray[IDC_VERTICALTEXT-IDC_BASE];
      break;

    case eCursor_all_scroll:
      
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_MOVE, FALSE);
      break;

    case eCursor_nesw_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENESW, FALSE);
      break;

    case eCursor_nwse_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENWSE, FALSE);
      break;

    case eCursor_ns_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENS, FALSE);
      break;

    case eCursor_ew_resize:
      newPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZEWE, FALSE);
      break;

    case eCursor_none:
      newPointer = sPtrArray[IDC_NONE-IDC_BASE];
      break;

    default:
      NS_ERROR("Invalid cursor type");
      break;
  }

  if (newPointer) {
    WinSetPointer(HWND_DESKTOP, newPointer);
  }

  return NS_OK;
}







NS_IMETHODIMP nsWindow::SetCursor(imgIContainer* aCursor,
                                  PRUint32 aHotspotX, PRUint32 aHotspotY)
{

  
  
  
  if (mCssCursorImg == aCursor && mCssCursorHPtr) {
    WinSetPointer(HWND_DESKTOP, mCssCursorHPtr);
    return NS_OK;
  }

  nsRefPtr<gfxImageSurface> frame;
  aCursor->CopyFrame(imgIContainer::FRAME_CURRENT,
                     imgIContainer::FLAG_SYNC_DECODE,
                     getter_AddRefs(frame));
  NS_ENSURE_TRUE(frame, NS_ERROR_NOT_AVAILABLE);

  
  
  PRInt32 width = frame->Width();
  PRInt32 height = frame->Height();
  NS_ENSURE_TRUE(width <= 128 && height <= 128, NS_ERROR_FAILURE);

  PRUint8* data = frame->Data();

  
  HBITMAP hBmp = CreateBitmapRGB(data, width, height);
  NS_ENSURE_TRUE(hBmp, NS_ERROR_FAILURE);

  
  HBITMAP hAlpha = CreateTransparencyMask(frame->Format(), data, width, height);
  if (!hAlpha) {
    GpiDeleteBitmap(hBmp);
    return NS_ERROR_FAILURE;
  }

  POINTERINFO info = {0};
  info.fPointer = TRUE;
  info.xHotspot = aHotspotX;
  info.yHotspot = height - aHotspotY - 1;
  info.hbmPointer = hAlpha;
  info.hbmColor = hBmp;

  
  HPOINTER cursor = WinCreatePointerIndirect(HWND_DESKTOP, &info);
  GpiDeleteBitmap(hBmp);
  GpiDeleteBitmap(hAlpha);
  NS_ENSURE_TRUE(cursor, NS_ERROR_FAILURE);

  
  WinSetPointer(HWND_DESKTOP, cursor);

  
  
  if (mCssCursorHPtr) {
    WinDestroyPointer(mCssCursorHPtr);
  }

  
  mCssCursorHPtr = cursor;
  mCssCursorImg = aCursor;

  return NS_OK;
}





#define ALIGNEDBPR(cx,bits) ( ( ( ((cx)*(bits)) + 31) / 32) * 4)

HBITMAP nsWindow::DataToBitmap(PRUint8* aImageData, PRUint32 aWidth,
                               PRUint32 aHeight, PRUint32 aDepth)
{
  
  HPS hps = (HPS)GetNativeData(NS_NATIVE_GRAPHIC);
  if (!hps) {
    return 0;
  }

  
  
  struct {
    BITMAPINFOHEADER2 head;
    RGB2 black;
    RGB2 white;
  } bi;

  memset(&bi, 0, sizeof(bi));
  bi.white.bBlue = (BYTE)255;
  bi.white.bGreen = (BYTE)255;
  bi.white.bRed = (BYTE)255;

  
  bi.head.cbFix = sizeof(bi.head);
  bi.head.cx = aWidth;
  bi.head.cy = aHeight;
  bi.head.cPlanes = 1;
  bi.head.cBitCount = aDepth;
  bi.head.ulCompression = BCA_UNCOMP;
  bi.head.cbImage = ALIGNEDBPR(aWidth, aDepth) * aHeight;
  bi.head.cclrUsed = (aDepth == 1 ? 2 : 0);

  
  HBITMAP hBmp = GpiCreateBitmap(hps, &bi.head, CBM_INIT,
                 reinterpret_cast<BYTE*>(aImageData),
                 (BITMAPINFO2*)&bi);

  
  FreeNativeData((void*)hps, NS_NATIVE_GRAPHIC);
  return hBmp;
}




HBITMAP nsWindow::CreateBitmapRGB(PRUint8* aImageData,
                                  PRUint32 aWidth,
                                  PRUint32 aHeight)
{
  
  const PRUint32 bpr = ALIGNEDBPR(aWidth, 24);
  PRUint8* bmp = (PRUint8*)malloc(bpr * aHeight);
  if (!bmp) {
    return 0;
  }

  PRUint32* pSrc = (PRUint32*)aImageData;
  for (PRUint32 row = aHeight; row > 0; --row) {
    PRUint8* pDst = bmp + bpr * (row - 1);

    for (PRUint32 col = aWidth; col > 0; --col) {
      
      
      PRUint32 color = *pSrc++;
      *pDst++ = color;       
      *pDst++ = color >> 8;  
      *pDst++ = color >> 16; 
    }
  }

  
  HBITMAP hAlpha = DataToBitmap(bmp, aWidth, aHeight, 24);

  
  free(bmp);
  return hAlpha;
}




HBITMAP nsWindow::CreateTransparencyMask(gfxASurface::gfxImageFormat format,
                                         PRUint8* aImageData,
                                         PRUint32 aWidth,
                                         PRUint32 aHeight)
{
  
  PRUint32 abpr = ALIGNEDBPR(aWidth, 1);
  PRUint32 cbData = abpr * aHeight;

  
  PRUint8* mono = (PRUint8*)calloc(cbData, 2);
  if (!mono) {
    return 0;
  }

  
  
  if (format == gfxASurface::ImageFormatARGB32) {

    
    PRInt32* pSrc = (PRInt32*)aImageData;
    for (PRUint32 row = aHeight; row > 0; --row) {
      
      PRUint8* pDst = mono + cbData + abpr * (row - 1);
      PRUint8 mask = 0x80;
      for (PRUint32 col = aWidth; col > 0; --col) {
        
        
        
        if (*pSrc++ >= 0) {
          *pDst |= mask;
        }

        mask >>= 1;
        if (!mask) {
          pDst++;
          mask = 0x80;
        }
      }
    }
  }

  
  HBITMAP hAlpha = DataToBitmap(mono, aWidth, aHeight * 2, 1);

  
  free(mono);
  return hAlpha;
}





NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener* aListener,
                                            nsIMenuRollup* aMenuRollup,
                                            PRBool aDoCapture,
                                            PRBool aConsumeRollupEvent)
{
  
  
  
  if (aDoCapture) {
    NS_ASSERTION(!gRollupWidget, "rollup widget reassigned before release");
    gRollupConsumeRollupEvent = aConsumeRollupEvent;
    NS_IF_RELEASE(gRollupWidget);
    gRollupListener = aListener;
    NS_IF_RELEASE(gMenuRollup);
    gMenuRollup = aMenuRollup;
    NS_IF_ADDREF(aMenuRollup);
    gRollupWidget = this;
    NS_ADDREF(this);
 } else {
    gRollupListener = nsnull;
    NS_IF_RELEASE(gMenuRollup);
    NS_IF_RELEASE(gRollupWidget);
  }

  return NS_OK;
}




PRBool nsWindow::EventIsInsideWindow(nsWindow* aWindow)
{
  RECTL  rcl;
  POINTL ptl;

  if (WinQueryMsgPos(0, &ptl)) {
    WinMapWindowPoints(HWND_DESKTOP, aWindow->mWnd, &ptl, 1);
    WinQueryWindowRect(aWindow->mWnd, &rcl);

    
    if (ptl.x < rcl.xLeft || ptl.x > rcl.xRight ||
        ptl.y > rcl.yTop  || ptl.y < rcl.yBottom) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}





PRBool nsWindow::RollupOnButtonDown(ULONG aMsg)
{
  
  if (EventIsInsideWindow((nsWindow*)gRollupWidget)) {
    return PR_FALSE;
  }

  
  
  PRUint32 popupsToRollup = PR_UINT32_MAX;

  if (gMenuRollup) {
    nsAutoTArray<nsIWidget*, 5> widgetChain;
    PRUint32 sameTypeCount = gMenuRollup->GetSubmenuWidgetChain(&widgetChain);
    for (PRUint32 i = 0; i < widgetChain.Length(); ++i) {
      nsIWidget* widget = widgetChain[i];
      if (EventIsInsideWindow((nsWindow*)widget)) {
        if (i < sameTypeCount) {
          return PR_FALSE;
        }
        popupsToRollup = sameTypeCount;
        break;
      }
    } 
  } 

  
  gRollupListener->Rollup(popupsToRollup,
                          aMsg == WM_BUTTON1DOWN ? &mLastRollup : nsnull);

  
  return gRollupConsumeRollupEvent;
}




void nsWindow::RollupOnFocusLost(HWND aFocus)
{
  HWND hRollup = ((nsWindow*)gRollupWidget)->mWnd;

  
  if (hRollup == aFocus) {
    return;
  }

  
  if (gMenuRollup) {
    nsAutoTArray<nsIWidget*, 5> widgetChain;
    gMenuRollup->GetSubmenuWidgetChain(&widgetChain);
    for (PRUint32 i = 0; i < widgetChain.Length(); ++i) {
      if (((nsWindow*)widgetChain[i])->mWnd == aFocus) {
        return;
      }
    }
  }

  
  gRollupListener->Rollup(PR_UINT32_MAX, nsnull);
  return;
}








MRESULT EXPENTRY fnwpNSWindow(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  nsAutoRollup autoRollup;

  
  
  nsWindow* wnd = nsWindow::GetNSWindowPtr(hwnd);
  if (!wnd) {
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
  }

  
  
  
  nsCOMPtr<nsISupports> kungFuDeathGrip;
  if (!wnd->mIsDestroying) {
    kungFuDeathGrip = do_QueryInterface((nsBaseWidget*)wnd);
  }

  
  if (gRollupListener && gRollupWidget) {
    switch (msg) {
      case WM_BUTTON1DOWN:
      case WM_BUTTON2DOWN:
      case WM_BUTTON3DOWN:
        if (nsWindow::RollupOnButtonDown(msg)) {
          return (MRESULT)PR_TRUE;
        }
        break;

      case WM_SETFOCUS:
        if (!mp2) {
          nsWindow::RollupOnFocusLost((HWND)mp1);
        }
        break;
    }
  }

  return wnd->ProcessMessage(msg, mp1, mp2);
}




MRESULT nsWindow::ProcessMessage(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  PRBool  isDone = PR_FALSE;
  MRESULT mresult = 0;

  switch (msg) {

    
    
    case WM_CLOSE:
    case WM_QUIT: {
      mWindowState |= nsWindowState_eClosing;
      nsGUIEvent event(PR_TRUE, NS_XUL_CLOSE, this);
      InitEvent(event);
      DispatchWindowEvent(&event);
      
      isDone = PR_TRUE;
      break;
    }

    case WM_DESTROY:
      OnDestroy();
      isDone = PR_TRUE;
      break;

    case WM_PAINT:
      isDone = OnPaint();
      break;

    case WM_TRANSLATEACCEL:
      isDone = OnTranslateAccelerator((PQMSG)mp1);
      break;

    case WM_CHAR:
      isDone = DispatchKeyEvent(mp1, mp2);
      break;

    
    
    

    case WM_BUTTON1DOWN:
      WinSetCapture(HWND_DESKTOP, mWnd);
      isDone = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, mp1, mp2);
      
      
      if (mWindowType == eWindowType_popup) {
        isDone = PR_TRUE;
      }
      
      sLastButton1Down.x = XFROMMP(mp1);
      sLastButton1Down.y = YFROMMP(mp1);
      break;

    case WM_BUTTON1UP:
      WinSetCapture(HWND_DESKTOP, 0);
      isDone = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, mp1, mp2);
      break;

    case WM_BUTTON1DBLCLK:
      isDone = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, mp1, mp2);
      break;

    case WM_BUTTON2DOWN:
      WinSetCapture(HWND_DESKTOP, mWnd);
      isDone = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, mp1, mp2, PR_FALSE,
                                  nsMouseEvent::eRightButton);
      break;

    case WM_BUTTON2UP:
      WinSetCapture(HWND_DESKTOP, 0);
      isDone = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, mp1, mp2, PR_FALSE,
                                  nsMouseEvent::eRightButton);
      break;

    case WM_BUTTON2DBLCLK:
      isDone = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, mp1, mp2,
                                  PR_FALSE, nsMouseEvent::eRightButton);
      break;

    case WM_BUTTON3DOWN:
      WinSetCapture(HWND_DESKTOP, mWnd);
      isDone = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, mp1, mp2, PR_FALSE,
                                  nsMouseEvent::eMiddleButton);
      break;

    case WM_BUTTON3UP:
      WinSetCapture(HWND_DESKTOP, 0);
      isDone = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, mp1, mp2, PR_FALSE,
                                  nsMouseEvent::eMiddleButton);
      break;

    case WM_BUTTON3DBLCLK:
      isDone = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, mp1, mp2, PR_FALSE,
                                  nsMouseEvent::eMiddleButton);
      break;

    case WM_CONTEXTMENU:
      if (SHORT2FROMMP(mp2)) {
        HWND hFocus = WinQueryFocus(HWND_DESKTOP);
        if (hFocus != mWnd) {
          WinSendMsg(hFocus, msg, mp1, mp2);
        } else {
          isDone = DispatchMouseEvent(NS_CONTEXTMENU, mp1, mp2, PR_TRUE,
                                      nsMouseEvent::eLeftButton);
        }
      } else {
        isDone = DispatchMouseEvent(NS_CONTEXTMENU, mp1, mp2, PR_FALSE,
                                    nsMouseEvent::eRightButton);
      }
      break;

    
    case WM_CHORD:
      isDone = OnMouseChord(mp1, mp2);
      break;

    case WM_MOUSEMOVE: {
      static POINTL ptlLastPos = { -1, -1 };

      
      
      if (ptlLastPos.x != (SHORT)SHORT1FROMMP(mp1) ||
          ptlLastPos.y != (SHORT)SHORT2FROMMP(mp1)) {
        ptlLastPos.x = (SHORT)SHORT1FROMMP(mp1);
        ptlLastPos.y = (SHORT)SHORT2FROMMP(mp1);
        DispatchMouseEvent(NS_MOUSE_MOVE, mp1, mp2);
      }

      
      isDone = PR_TRUE;
      break;
    }

    case WM_MOUSEENTER:
      isDone = DispatchMouseEvent(NS_MOUSE_ENTER, mp1, mp2);
      break;

    case WM_MOUSELEAVE:
      isDone = DispatchMouseEvent(NS_MOUSE_EXIT, mp1, mp2);
      break;

    case WM_APPCOMMAND: {
      PRUint32 appCommand = SHORT2FROMMP(mp2) & 0xfff;

      switch (appCommand) {
        case APPCOMMAND_BROWSER_BACKWARD:
        case APPCOMMAND_BROWSER_FORWARD:
        case APPCOMMAND_BROWSER_REFRESH:
        case APPCOMMAND_BROWSER_STOP:
          DispatchCommandEvent(appCommand);
          
          mresult = (MRESULT)1;
          isDone = PR_TRUE;
          break;
      }
      break;
    }

    case WM_HSCROLL:
    case WM_VSCROLL:
      isDone = DispatchScrollEvent(msg, mp1, mp2);
      break;

    
    
    

    
    
    
    case WM_FOCUSCHANGED:
      DEBUGFOCUS(WM_FOCUSCHANGED);
      if (SHORT1FROMMP(mp2)) {
        ActivateTopLevelWidget();
        ActivatePlugin(HWNDFROMMP(mp1));
      }
      break;

    case WM_WINDOWPOSCHANGED:
      isDone = OnReposition((PSWP) mp1);
      break;

      
    case DM_DRAGOVER:
    case DM_DRAGLEAVE:
    case DM_DROP:
    case DM_RENDERCOMPLETE:
    case DM_DROPHELP:
      OnDragDropMsg(msg, mp1, mp2, mresult);
      isDone = PR_TRUE;
      break;
  }

  
  
  if (!isDone) {
    mresult = WinDefWindowProc(mWnd, msg, mp1, mp2);
  }

  return mresult;
}







void nsWindow::OnDestroy()
{
  mOnDestroyCalled = PR_TRUE;

  SetNSWindowPtr(mWnd, 0);
  mWnd = 0;

  
  nsBaseWidget::OnDestroy();

  
  
  
  
  
  
  
  if (!(nsWindowState_eDoingDelete & mWindowState)) {
    AddRef();
    nsGUIEvent event(PR_TRUE, NS_DESTROY, this);
    InitEvent(event);
    DispatchWindowEvent(&event);
    Release();
  }

  
  mWindowState |= nsWindowState_eDead;
  mWindowState &= ~(nsWindowState_eLive|nsWindowState_ePrecreate|
                    nsWindowState_eInCreate);
}



PRBool nsWindow::OnReposition(PSWP pSwp)
{
  PRBool result = PR_FALSE;

  if (pSwp->fl & SWP_MOVE && !(pSwp->fl & SWP_MINIMIZE)) {
    HWND hParent = mParent ? mParent->mWnd : WinQueryWindow(mWnd, QW_PARENT);

    
    POINTL ptl = { pSwp->x, pSwp->y + pSwp->cy - 1 };
    
    WinMapWindowPoints(WinQueryWindow(mWnd, QW_PARENT), hParent, &ptl, 1);
    PM2NS_PARENT(ptl);
    mBounds.x = ptl.x;
    mBounds.y = ptl.y;
    WinMapWindowPoints(hParent, HWND_DESKTOP, &ptl, 1);

    result = DispatchMoveEvent(ptl.x, ptl.y);
  }

  if (pSwp->fl & SWP_SIZE && !(pSwp->fl & SWP_MINIMIZE)) {
    mBounds.width  = pSwp->cx;
    mBounds.height = pSwp->cy;

    
    if (ConfirmThebesSurface()) {
        mThebesSurface->Resize(gfxIntSize(mBounds.width, mBounds.height));
    }

    result = DispatchResizeEvent(mBounds.width, mBounds.height);
  }

  return result;
}



PRBool nsWindow::OnPaint()
{
  HPS    hPS;
  HPS    hpsDrag;
  HRGN   hrgn;
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

#ifdef DEBUG_PAINT
  HRGN debugPaintFlashRegion = 0;
  HPS  debugPaintFlashPS = 0;

  if (debug_WantPaintFlashing()) {
    debugPaintFlashPS = WinGetPS(mWnd);
    debugPaintFlashRegion = GpiCreateRegion(debugPaintFlashPS, 0, 0);
    WinQueryUpdateRegion(mWnd, debugPaintFlashRegion);
  }
#endif


do {

  
  
  
  CheckDragStatus(ACTION_PAINT, &hpsDrag);
  hPS = hpsDrag ? hpsDrag : WinGetPS(mWnd);

  
  
  RECTL  rcl = { 0 };
  if (!hPS) {
    WinQueryWindowRect(mWnd, &rcl);
    WinValidateRect(mWnd, &rcl, FALSE);
    break;
  }

  
  hrgn = GpiCreateRegion(hPS, 0, 0);
  WinQueryUpdateRegion(mWnd, hrgn);
  WinBeginPaint(mWnd, hPS, &rcl);

  
  if (WinIsRectEmpty(0, &rcl)) {
    break;
  }

  
  
  
  if (!ConfirmThebesSurface()) {
    WinDrawBorder(hPS, &rcl, 0, 0, 0, 0, DB_INTERIOR | DB_AREAATTRS);
    break;
  }

  
  
  if (!mEventCallback) {
    mThebesSurface->Refresh(&rcl, hPS);
    break;
  }

  
  nsPaintEvent event(PR_TRUE, NS_PAINT, this);
  InitEvent(event);
  nsRefPtr<gfxContext> thebesContext = new gfxContext(mThebesSurface);

  
  
  HRGN hrgnPaint;
  hrgnPaint = GpiCreateRegion(hPS, 1, &rcl);
  if (hrgnPaint) {
    GpiCombineRegion(hPS, hrgn, hrgn, hrgnPaint, CRGN_AND);
    GpiDestroyRegion(hPS, hrgnPaint);
  }

  
  
  
  #define MAX_CLIPRECTS 8
  RGNRECT rgnrect = { 1, MAX_CLIPRECTS, 0, RECTDIR_LFRT_TOPBOT };
  RECTL   arect[MAX_CLIPRECTS];
  RECTL*  pr = arect;

  if (!GpiQueryRegionRects(hPS, hrgn, 0, &rgnrect, 0) ||
      rgnrect.crcReturned > MAX_CLIPRECTS) {
    rgnrect.crcReturned = 1;
    arect[0] = rcl;
  } else {
    GpiQueryRegionRects(hPS, hrgn, 0, &rgnrect, arect);
  }

  
  thebesContext->NewPath();
  for (PRUint32 i = 0; i < rgnrect.crcReturned; i++, pr++) {
    event.region.Or(event.region, 
                    nsIntRect(pr->xLeft,
                              mBounds.height - pr->yTop,
                              pr->xRight - pr->xLeft,
                              pr->yTop - pr->yBottom));

    thebesContext->Rectangle(gfxRect(pr->xLeft,
                                     mBounds.height - pr->yTop,
                                     pr->xRight - pr->xLeft,
                                     pr->yTop - pr->yBottom));
  }
  thebesContext->Clip();

#ifdef DEBUG_PAINT
  debug_DumpPaintEvent(stdout, this, &event, nsCAutoString("noname"),
                       (PRInt32)mWnd);
#endif

  
  
  AutoLayerManagerSetup
      setupLayerManager(this, thebesContext, BasicLayerManager::BUFFER_NONE);
  if (!DispatchWindowEvent(&event, eventStatus)) {
    break;
  }

  
  thebesContext->PopGroupToSource();
  thebesContext->SetOperator(gfxContext::OPERATOR_SOURCE);
  thebesContext->Paint();
  pr = arect;
  for (PRUint32 i = 0; i < rgnrect.crcReturned; i++, pr++) {
    mThebesSurface->Refresh(pr, hPS);
  }

} while (0);

  
  if (hPS) {
    WinEndPaint(hPS);
    if (hrgn) {
      GpiDestroyRegion(hPS, hrgn);
    }
    if (!hpsDrag || !ReleaseIfDragHPS(hpsDrag)) {
      WinReleasePS(hPS);
    }
  }

#ifdef DEBUG_PAINT
  if (debug_WantPaintFlashing()) {
    
    
    
    if (eventStatus != nsEventStatus_eIgnore) {
      LONG CurMix = GpiQueryMix(debugPaintFlashPS);
      GpiSetMix(debugPaintFlashPS, FM_INVERT);

      GpiPaintRegion(debugPaintFlashPS, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
      GpiPaintRegion(debugPaintFlashPS, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));

      GpiSetMix(debugPaintFlashPS, CurMix);
    }
    GpiDestroyRegion(debugPaintFlashPS, debugPaintFlashRegion);
    WinReleasePS(debugPaintFlashPS);
  }
#endif

  return PR_TRUE;
}




PRBool nsWindow::OnMouseChord(MPARAM mp1, MPARAM mp2)
{
  if (!isKeyDown(VK_BUTTON1) || !isKeyDown(VK_BUTTON2)) {
    return PR_FALSE;
  }

  
  
  PRBool isCopy = PR_FALSE;
  if (abs(XFROMMP(mp1) - sLastButton1Down.x) >
        (WinQuerySysValue(HWND_DESKTOP, SV_CXMOTIONSTART) / 2) ||
      abs(YFROMMP(mp1) - sLastButton1Down.y) >
        (WinQuerySysValue(HWND_DESKTOP, SV_CYMOTIONSTART) / 2)) {
    isCopy = PR_TRUE;
  }

  nsKeyEvent event(PR_TRUE, NS_KEY_PRESS, this);
  nsIntPoint point(0,0);
  InitEvent(event, &point);

  event.keyCode     = NS_VK_INSERT;
  if (isCopy) {
    event.isShift   = PR_FALSE;
    event.isControl = PR_TRUE;
  } else {
    event.isShift   = PR_TRUE;
    event.isControl = PR_FALSE;
  }
  event.isAlt       = PR_FALSE;
  event.isMeta      = PR_FALSE;
  event.eventStructType = NS_KEY_EVENT;
  event.charCode    = 0;

  
  if (SHORT1FROMMP(mp1) & (KC_VIRTUALKEY | KC_KEYUP | KC_LONEKEY)) {
    USHORT usVKey = SHORT2FROMMP(mp2);
    if (usVKey == VK_SHIFT) {
      event.isShift = PR_TRUE;
    }
    if (usVKey == VK_CTRL) {
      event.isControl = PR_TRUE;
    }
    if (usVKey == VK_ALTGRAF || usVKey == VK_ALT) {
      event.isAlt = PR_TRUE;
    }
  }

  return DispatchWindowEvent(&event);
}
























PRBool nsWindow::OnDragDropMsg(ULONG msg, MPARAM mp1, MPARAM mp2, MRESULT& mr)
{
  nsresult rv;
  PRUint32 eventType = 0;
  PRUint32 dragFlags = 0;

  mr = 0;
  nsCOMPtr<nsIDragService> dragService =
                    do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (dragService) {
    nsCOMPtr<nsIDragSessionOS2> dragSession(
                        do_QueryInterface(dragService, &rv));
    if (dragSession) {

      
      switch (msg) {

        case DM_DRAGOVER:
          dragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);
          rv = dragSession->DragOverMsg((PDRAGINFO)mp1, mr, &dragFlags);
          eventType = NS_DRAGDROP_OVER;
          break;

        case DM_DRAGLEAVE:
          rv = dragSession->DragLeaveMsg((PDRAGINFO)mp1, &dragFlags);
          eventType = NS_DRAGDROP_EXIT;
          break;

        case DM_DROP:
          rv = dragSession->DropMsg((PDRAGINFO)mp1, mWnd, &dragFlags);
          eventType = NS_DRAGDROP_DROP;
          break;

        case DM_DROPHELP:
          rv = dragSession->DropHelpMsg((PDRAGINFO)mp1, &dragFlags);
          eventType = NS_DRAGDROP_EXIT;
          break;

        case DM_RENDERCOMPLETE:
          rv = dragSession->RenderCompleteMsg((PDRAGTRANSFER)mp1,
                                              SHORT1FROMMP(mp2), &dragFlags);
          eventType = NS_DRAGDROP_DROP;
          break;

        default:
          rv = NS_ERROR_FAILURE;
      }

      
      if (NS_SUCCEEDED(rv)) {
        mDragStatus = sDragStatus = (dragFlags & DND_DragStatus);

        if (dragFlags & DND_DispatchEnterEvent) {
          DispatchDragDropEvent(NS_DRAGDROP_ENTER);
        }
        if (dragFlags & DND_DispatchEvent) {
          DispatchDragDropEvent(eventType);
        }
        if (dragFlags & DND_GetDragoverResult) {
          dragSession->GetDragoverResult(mr);
        }
        if (dragFlags & DND_ExitSession) {
          dragSession->ExitSession(&dragFlags);
        }
      }
    }
  }
  
  sDragStatus = mDragStatus = (dragFlags & DND_DragStatus);

  return PR_TRUE;
}







PRBool nsWindow::CheckDragStatus(PRUint32 aAction, HPS* aHps)
{
  PRBool rtn    = PR_TRUE;
  PRBool getHps = PR_FALSE;

  switch (aAction) {

    
    case ACTION_PAINT:
    case ACTION_SCROLL:
      if (sDragStatus & DND_MozDrag) {
        getHps = PR_TRUE;
      }
      break;

    
    case ACTION_DRAW:
      if ((sDragStatus & DND_MozDrag) ||
          (mDragStatus & DND_NativeDrag)) {
        getHps = PR_TRUE;
      }
      break;

    
    case ACTION_SHOW:
      if ((sDragStatus & (DND_NativeDrag | DND_InDrop)) == DND_NativeDrag) {
        rtn = PR_FALSE;
      }
      break;

    
    case ACTION_PTRPOS:
      if (!sDragStatus) {
        rtn = PR_FALSE;
      }
      break;

    default:
      rtn = PR_FALSE;
  }

  
  
  
  
  
  if (aHps) {
    if (getHps && !mDragHps) {
      mDragHps = DrgGetPS(mWnd);
      *aHps = mDragHps;
    } else {
      *aHps = 0;
    }
  }

  return rtn;
}





PRBool nsWindow::ReleaseIfDragHPS(HPS aHps)
{
  if (mDragHps && aHps == mDragHps) {
    DrgReleasePS(mDragHps);
    mDragHps = 0;
    return PR_TRUE;
  }

  return PR_FALSE;
}







NS_IMETHODIMP nsWindow::GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState)
{
  PRUint32  vkey;

  NS_ENSURE_ARG_POINTER(aLEDState);

  switch (aKeyCode) {
    case NS_VK_CAPS_LOCK:
      vkey = VK_CAPSLOCK;
      break;
    case NS_VK_NUM_LOCK:
      vkey = VK_NUMLOCK;
      break;
    case NS_VK_SCROLL_LOCK:
      vkey = VK_SCRLLOCK;
      break;
    default:
      *aLEDState = PR_FALSE;
      return NS_OK;
  }

  *aLEDState = (WinGetKeyState(HWND_DESKTOP, vkey) & 1) != 0;
  return NS_OK;
}




PRBool nsWindow::OnTranslateAccelerator(PQMSG pQmsg)
{
  if (pQmsg->msg != WM_CHAR) {
    return PR_FALSE;
  }

  LONG mp1 = (LONG)pQmsg->mp1;
  LONG mp2 = (LONG)pQmsg->mp2;
  LONG sca = SHORT1FROMMP(mp1) & (KC_SHIFT | KC_CTRL | KC_ALT);

  if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY) {

    
    if (SHORT2FROMMP(mp2) == VK_F1 || SHORT2FROMMP(mp2) == VK_F10) {
      return (!sca ? PR_TRUE : PR_FALSE);
    }

    
    if (SHORT2FROMMP(mp2) == VK_ENTER) {
      return (sca == KC_SHIFT ? PR_TRUE : PR_FALSE);
    }

    
    if (SHORT2FROMMP(mp2) == VK_NEWLINE) {
      return (sca == KC_ALT ? PR_TRUE : PR_FALSE);
    }

    
    if ((SHORT2FROMMP(mp2) == VK_ALT || SHORT2FROMMP(mp2) == VK_ALTGRAF) &&
        (SHORT1FROMMP(mp1) & (KC_KEYUP | KC_LONEKEY))
                          == (KC_KEYUP | KC_LONEKEY)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}









PRBool nsWindow::DispatchKeyEvent(MPARAM mp1, MPARAM mp2)
{
  nsKeyEvent pressEvent(PR_TRUE, 0, nsnull);
  USHORT fsFlags = SHORT1FROMMP(mp1);
  USHORT usVKey = SHORT2FROMMP(mp2);
  USHORT usChar = SHORT1FROMMP(mp2);
  UCHAR uchScan = CHAR4FROMMP(mp1);

  
  
  if (fsFlags & KC_VIRTUALKEY && !(fsFlags & KC_KEYUP) &&
      (usVKey == VK_SHIFT || usVKey == VK_CTRL || usVKey == VK_ALTGRAF)) {
    return PR_FALSE;
  }

  
  
  if ((fsFlags & KC_VIRTUALKEY) && (usVKey == VK_ALT) && !usChar &&
      (!(fsFlags & KC_LONEKEY)) && (fsFlags & KC_KEYUP)) {
    return PR_FALSE;
  }

   
  if (fsFlags & KC_DEADKEY) {
    return PR_TRUE;
  }

  
  
  nsIntPoint point(0,0);
  nsKeyEvent event(PR_TRUE, (fsFlags & KC_KEYUP) ? NS_KEY_UP : NS_KEY_DOWN,
                   this);
  InitEvent(event, &point);
  event.keyCode   = WMChar2KeyCode(mp1, mp2);
  event.isShift   = (fsFlags & KC_SHIFT) ? PR_TRUE : PR_FALSE;
  event.isControl = (fsFlags & KC_CTRL) ? PR_TRUE : PR_FALSE;
  event.isAlt     = (fsFlags & KC_ALT) ? PR_TRUE : PR_FALSE;
  event.isMeta    = PR_FALSE;
  event.charCode  = 0;

  
  
  
  
  if (((event.keyCode == NS_VK_UP) || (event.keyCode == NS_VK_DOWN)) &&
      !(fsFlags & KC_KEYUP) &&
      (!CHAR3FROMMP(mp1) || fsFlags & KC_CTRL || fsFlags & KC_ALT)) {
    if (!(WinGetPhysKeyState(HWND_DESKTOP, uchScan) & 0x8000)) {
      MPARAM mp2;
      if (event.keyCode == NS_VK_UP) {
        mp2 = MPFROM2SHORT(0, SB_LINEUP);
      } else {
        mp2 = MPFROM2SHORT(0, SB_LINEDOWN);
      }
      WinSendMsg(mWnd, WM_VSCROLL, 0, mp2);
      return FALSE;
    }
  }

  pressEvent = event;
  PRBool rc = DispatchWindowEvent(&event);

  
  if (fsFlags & KC_KEYUP) {
    return rc;
  }

  
  
  
  if (fsFlags & KC_INVALIDCOMP) {
    
    
    return rc;
  }

  
  
  pressEvent.message = NS_KEY_PRESS;
  if (rc) {
    pressEvent.flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }

  if (usChar) {
    USHORT inbuf[2];
    inbuf[0] = usChar;
    inbuf[1] = '\0';

    nsAutoChar16Buffer outbuf;
    PRInt32 bufLength;
    MultiByteToWideChar(0, (const char*)inbuf, 2, outbuf, bufLength);

    pressEvent.charCode = outbuf[0];

    if (pressEvent.isControl && !(fsFlags & (KC_VIRTUALKEY | KC_DEADKEY))) {
      if (!pressEvent.isShift && (pressEvent.charCode >= 'A' && pressEvent.charCode <= 'Z')) {
        pressEvent.charCode = tolower(pressEvent.charCode);
      }
      if (pressEvent.isShift && (pressEvent.charCode >= 'a' && pressEvent.charCode <= 'z')) {
        pressEvent.charCode = toupper(pressEvent.charCode);
      }
      pressEvent.keyCode = 0;
    } else if (!pressEvent.isControl && !pressEvent.isAlt && pressEvent.charCode != 0) {
      if (!(fsFlags & KC_VIRTUALKEY) || 
          ((fsFlags & KC_CHAR) && !pressEvent.keyCode)) {
        pressEvent.keyCode = 0;
      } else if (usVKey == VK_SPACE) {
        
      } else if ((fsFlags & KC_VIRTUALKEY) &&
                 isNumPadScanCode(uchScan) && pressEvent.keyCode != 0 && isNumlockOn) {
        
        pressEvent.keyCode = 0;
      } else { 
        pressEvent.charCode = 0;
      }
    }
    rc = DispatchWindowEvent(&pressEvent);
  }

  return rc;
}




static
PRUint32 WMChar2KeyCode(MPARAM mp1, MPARAM mp2)
{
  PRUint32 rc = SHORT1FROMMP(mp2);  
  PRUint32 rcmask = rc & 0x00FF;    
  USHORT sc = CHAR4FROMMP(mp1);     
  USHORT flags = SHORT1FROMMP(mp1); 

  
  
  

  
  if (!(flags & (KC_VIRTUALKEY | KC_DEADKEY)) ||
      (rcmask >= '0' && rcmask <= '9' &&             
       (isNumPadScanCode(sc) ? isNumlockOn : 1))) { 
    if (flags & KC_KEYUP) { 
                            
      rc = rcmask;
    } else { 
      if (!(flags & KC_CHAR)) {
        if ((flags & KC_ALT) || (flags & KC_CTRL)) {
          rc = rcmask;
        } else {
          rc = 0;
        }
      }
    }

    if (rc < 0xFF) {
      if (rc >= 'a' && rc <= 'z') { 
                                    
        rc = rc - 'a' + NS_VK_A;
      } else if (rc >= 'A' && rc <= 'Z') { 
        rc = rc - 'A' + NS_VK_A;
      } else if (rc >= '0' && rc <= '9') {
        
        rc = rc - '0' + NS_VK_0;
      } else {
        
        
        switch (sc) {
          case 0x02: rc = NS_VK_1;             break;
          case 0x03: rc = NS_VK_2;             break;
          case 0x04: rc = NS_VK_3;             break;
          case 0x05: rc = NS_VK_4;             break;
          case 0x06: rc = NS_VK_5;             break;
          case 0x07: rc = NS_VK_6;             break;
          case 0x08: rc = NS_VK_7;             break;
          case 0x09: rc = NS_VK_8;             break;
          case 0x0A: rc = NS_VK_9;             break;
          case 0x0B: rc = NS_VK_0;             break;
          case 0x0D: rc = NS_VK_EQUALS;        break;
          case 0x1A: rc = NS_VK_OPEN_BRACKET;  break;
          case 0x1B: rc = NS_VK_CLOSE_BRACKET; break;
          case 0x27: rc = NS_VK_SEMICOLON;     break;
          case 0x28: rc = NS_VK_QUOTE;         break;
          case 0x29: rc = NS_VK_BACK_QUOTE;    break;
          case 0x2B: rc = NS_VK_BACK_SLASH;    break;
          case 0x33: rc = NS_VK_COMMA;         break;
          case 0x34: rc = NS_VK_PERIOD;        break;
          case 0x35: rc = NS_VK_SLASH;         break;
          case 0x37: rc = NS_VK_MULTIPLY;      break;
          case 0x4A: rc = NS_VK_SUBTRACT;      break;
          case 0x4C: rc = NS_VK_CLEAR;         break; 
          case 0x4E: rc = NS_VK_ADD;           break;
          case 0x5C: rc = NS_VK_DIVIDE;        break;
          default: break;
        } 
      } 
    } 
  } else if (flags & KC_VIRTUALKEY) {
    USHORT vk = SHORT2FROMMP(mp2);
    if (flags & KC_KEYUP) { 
                            
      rc = rcmask;
    }
    if (isNumPadScanCode(sc) &&
        (((flags & KC_ALT) && (sc != PMSCAN_PADPERIOD)) ||
          ((flags & (KC_CHAR | KC_SHIFT)) == KC_CHAR)  ||
          ((flags & KC_KEYUP) && rc != 0))) {
      CHAR numpadMap[] = {NS_VK_NUMPAD7, NS_VK_NUMPAD8, NS_VK_NUMPAD9, 0,
                          NS_VK_NUMPAD4, NS_VK_NUMPAD5, NS_VK_NUMPAD6, 0,
                          NS_VK_NUMPAD1, NS_VK_NUMPAD2, NS_VK_NUMPAD3,
                          NS_VK_NUMPAD0, NS_VK_DECIMAL};
      
      
      if (numpadMap[sc - PMSCAN_PAD7] != 0) { 
        if (flags & KC_ALT) { 
          rc = 0;
        } else {
          rc = numpadMap[sc - PMSCAN_PAD7];
        }
      } else {                                
        rc = 0; 
      }
    } else if (!(flags & KC_CHAR) || isNumPadScanCode(sc) ||
               (vk == VK_BACKSPACE) || (vk == VK_TAB) || (vk == VK_BACKTAB) ||
               (vk == VK_ENTER) || (vk == VK_NEWLINE) || (vk == VK_SPACE)) {
      if (vk >= VK_F1 && vk <= VK_F24) {
        rc = NS_VK_F1 + (vk - VK_F1);
      }
      else switch (vk) {
        case VK_NUMLOCK:   rc = NS_VK_NUM_LOCK; break;
        case VK_SCRLLOCK:  rc = NS_VK_SCROLL_LOCK; break;
        case VK_ESC:       rc = NS_VK_ESCAPE; break; 
        case VK_BACKSPACE: rc = NS_VK_BACK; break;
        case VK_TAB:       rc = NS_VK_TAB; break;
        case VK_BACKTAB:   rc = NS_VK_TAB; break; 
        case VK_CLEAR:     rc = NS_VK_CLEAR; break;
        case VK_NEWLINE:   rc = NS_VK_RETURN; break;
        case VK_ENTER:     rc = NS_VK_RETURN; break;
        case VK_SHIFT:     rc = NS_VK_SHIFT; break;
        case VK_CTRL:      rc = NS_VK_CONTROL; break;
        case VK_ALT:       rc = NS_VK_ALT; break;
        case VK_PAUSE:     rc = NS_VK_PAUSE; break;
        case VK_CAPSLOCK:  rc = NS_VK_CAPS_LOCK; break;
        case VK_SPACE:     rc = NS_VK_SPACE; break;
        case VK_PAGEUP:    rc = NS_VK_PAGE_UP; break;
        case VK_PAGEDOWN:  rc = NS_VK_PAGE_DOWN; break;
        case VK_END:       rc = NS_VK_END; break;
        case VK_HOME:      rc = NS_VK_HOME; break;
        case VK_LEFT:      rc = NS_VK_LEFT; break;
        case VK_UP:        rc = NS_VK_UP; break;
        case VK_RIGHT:     rc = NS_VK_RIGHT; break;
        case VK_DOWN:      rc = NS_VK_DOWN; break;
        case VK_PRINTSCRN: rc = NS_VK_PRINTSCREEN; break;
        case VK_INSERT:    rc = NS_VK_INSERT; break;
        case VK_DELETE:    rc = NS_VK_DELETE; break;
      } 
    }
  } 

  return rc;
}







void nsWindow::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
  
  if (!aPoint) {
    
    
    POINTL ptl;
    if (CheckDragStatus(ACTION_PTRPOS, 0)) {
      WinQueryPointerPos(HWND_DESKTOP, &ptl);
    } else {
      WinQueryMsgPos(0, &ptl);
    }

    WinMapWindowPoints(HWND_DESKTOP, mWnd, &ptl, 1);
    PM2NS(ptl);
    event.refPoint.x = ptl.x;
    event.refPoint.y = ptl.y;
  } else {
    
    event.refPoint.x = aPoint->x;
    event.refPoint.y = aPoint->y;
  }

  event.time = WinQueryMsgTime(0);
  return;
}




NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
  aStatus = nsEventStatus_eIgnore;

  if (!mEventCallback) {
    return NS_OK;
  }

  
  
  if ((mWindowState & nsWindowState_eLive) ||
      (mWindowState == nsWindowState_eInCreate && event->message == NS_CREATE)) {
    aStatus = (*mEventCallback)(event);
  }
  return NS_OK;
}



NS_IMETHODIMP nsWindow::ReparentNativeWidget(nsIWidget* aNewParent)
{
  NS_PRECONDITION(aNewParent, "");
  return NS_ERROR_NOT_IMPLEMENTED;
}



PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
{
  nsEventStatus status;
  DispatchEvent(event, status);
  return (status == nsEventStatus_eConsumeNoDefault);
}

PRBool nsWindow::DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus) {
  DispatchEvent(event, aStatus);
  return (aStatus == nsEventStatus_eConsumeNoDefault);
}



PRBool nsWindow::DispatchCommandEvent(PRUint32 aEventCommand)
{
  nsCOMPtr<nsIAtom> command;

  switch (aEventCommand) {
    case APPCOMMAND_BROWSER_BACKWARD:
      command = nsWidgetAtoms::Back;
      break;
    case APPCOMMAND_BROWSER_FORWARD:
      command = nsWidgetAtoms::Forward;
      break;
    case APPCOMMAND_BROWSER_REFRESH:
      command = nsWidgetAtoms::Reload;
      break;
    case APPCOMMAND_BROWSER_STOP:
      command = nsWidgetAtoms::Stop;
      break;
    default:
      return PR_FALSE;
  }

  nsCommandEvent event(PR_TRUE, nsWidgetAtoms::onAppCommand, command, this);
  InitEvent(event);
  return DispatchWindowEvent(&event);
}



PRBool nsWindow::DispatchDragDropEvent(PRUint32 aMsg)
{
  nsDragEvent event(PR_TRUE, aMsg, this);
  InitEvent(event);

  event.isShift   = isKeyDown(VK_SHIFT);
  event.isControl = isKeyDown(VK_CTRL);
  event.isAlt     = isKeyDown(VK_ALT) || isKeyDown(VK_ALTGRAF);
  event.isMeta    = PR_FALSE;

  return DispatchWindowEvent(&event);
}



PRBool nsWindow::DispatchMoveEvent(PRInt32 aX, PRInt32 aY)
{
  
  nsGUIEvent event(PR_TRUE, NS_MOVE, this);
  nsIntPoint point(aX, aY);
  InitEvent(event, &point);
  return DispatchWindowEvent(&event);
}



PRBool nsWindow::DispatchResizeEvent(PRInt32 aX, PRInt32 aY)
{
  nsSizeEvent event(PR_TRUE, NS_SIZE, this);
  nsIntRect   rect(0, 0, aX, aY);

  InitEvent(event);
  event.windowSize = &rect;             
  event.mWinWidth = mBounds.width;
  event.mWinHeight = mBounds.height;

  return DispatchWindowEvent(&event);
}




PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType, MPARAM mp1, MPARAM mp2,
                                    PRBool aIsContextMenuKey, PRInt16 aButton)
{
  NS_ENSURE_TRUE(aEventType, PR_FALSE);

  nsMouseEvent event(PR_TRUE, aEventType, this, nsMouseEvent::eReal,
                     aIsContextMenuKey
                     ? nsMouseEvent::eContextMenuKey
                     : nsMouseEvent::eNormal);
  event.button = aButton;

  if (aEventType == NS_MOUSE_ENTER || aEventType == NS_MOUSE_EXIT) {
    
    
    if (HWNDFROMMP(mp1) != mWnd) {
      return FALSE;
    }

    
    
    
    
    if (aEventType == NS_MOUSE_EXIT) {
      HWND  hTop = 0;
      HWND  hCur = mWnd;
      HWND  hDesk = WinQueryDesktopWindow(0, 0);
      while (hCur && hCur != hDesk) {
        hTop = hCur;
        hCur = WinQueryWindow(hCur, QW_PARENT);
      }

      
      hTop = WinWindowFromID(hTop, FID_CLIENT);
      if (!hTop || !WinIsChild(HWNDFROMMP(mp2), hTop)) {
        event.exit = nsMouseEvent::eTopLevel;
      }
    }

    InitEvent(event, nsnull);
    event.isShift   = isKeyDown(VK_SHIFT);
    event.isControl = isKeyDown(VK_CTRL);
    event.isAlt     = isKeyDown(VK_ALT) || isKeyDown(VK_ALTGRAF);
  } else {
    POINTL ptl;
    if (aEventType == NS_CONTEXTMENU && aIsContextMenuKey) {
      WinQueryPointerPos(HWND_DESKTOP, &ptl);
      WinMapWindowPoints(HWND_DESKTOP, mWnd, &ptl, 1);
    } else {
      ptl.x = (SHORT)SHORT1FROMMP(mp1);
      ptl.y = (SHORT)SHORT2FROMMP(mp1);
    }
    PM2NS(ptl);
    nsIntPoint pt(ptl.x, ptl.y);
    InitEvent(event, &pt);

    USHORT usFlags  = SHORT2FROMMP(mp2);
    event.isShift   = (usFlags & KC_SHIFT) ? PR_TRUE : PR_FALSE;
    event.isControl = (usFlags & KC_CTRL) ? PR_TRUE : PR_FALSE;
    event.isAlt     = (usFlags & KC_ALT) ? PR_TRUE : PR_FALSE;
  }
  event.isMeta = PR_FALSE;

  
  if (aEventType == NS_MOUSE_DOUBLECLICK &&
      (aButton == nsMouseEvent::eLeftButton ||
       aButton == nsMouseEvent::eRightButton)) {
    event.message = NS_MOUSE_BUTTON_DOWN;
    event.button = (aButton == nsMouseEvent::eLeftButton) ?
                   nsMouseEvent::eLeftButton : nsMouseEvent::eRightButton;
    event.clickCount = 2;
  } else {
    event.clickCount = 1;
  }

  NPEvent pluginEvent;
  switch (aEventType) {

    case NS_MOUSE_BUTTON_DOWN:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_BUTTON1DOWN;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_BUTTON3DOWN;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_BUTTON2DOWN;
          break;
        default:
          break;
      }
      break;

    case NS_MOUSE_BUTTON_UP:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_BUTTON1UP;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_BUTTON3UP;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_BUTTON2UP;
          break;
        default:
          break;
      }
      break;

    case NS_MOUSE_DOUBLECLICK:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_BUTTON1DBLCLK;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_BUTTON3DBLCLK;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_BUTTON2DBLCLK;
          break;
        default:
          break;
      }
      break;

    case NS_MOUSE_MOVE:
      pluginEvent.event = WM_MOUSEMOVE;
      break;
  }

  pluginEvent.wParam = 0;
  pluginEvent.lParam = MAKELONG(event.refPoint.x, event.refPoint.y);

  event.pluginEvent = (void*)&pluginEvent;

  return DispatchWindowEvent(&event);
}




PRBool nsWindow::DispatchActivationEvent(PRUint32 aEventType)
{
  nsGUIEvent event(PR_TRUE, aEventType, this);

  
  
  nsIntPoint point(0, 0);
  InitEvent(event, &point);

  NPEvent pluginEvent;
  switch (aEventType) {
    case NS_ACTIVATE:
      pluginEvent.event = WM_SETFOCUS;
      break;
    case NS_DEACTIVATE:
      pluginEvent.event = WM_FOCUSCHANGED;
      break;
    case NS_PLUGIN_ACTIVATE:
      pluginEvent.event = WM_FOCUSCHANGED;
      break;
  }
  event.pluginEvent = (void*)&pluginEvent;

  return DispatchWindowEvent(&event);
}



PRBool nsWindow::DispatchScrollEvent(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  nsMouseScrollEvent scrollEvent(PR_TRUE, NS_MOUSE_SCROLL, this);
  InitEvent(scrollEvent);

  scrollEvent.isShift     = isKeyDown(VK_SHIFT);
  scrollEvent.isControl   = isKeyDown(VK_CTRL);
  scrollEvent.isAlt       = isKeyDown(VK_ALT) || isKeyDown(VK_ALTGRAF);
  scrollEvent.isMeta      = PR_FALSE;
  scrollEvent.scrollFlags = (msg == WM_HSCROLL) ?
                            nsMouseScrollEvent::kIsHorizontal :
                            nsMouseScrollEvent::kIsVertical;

  
  
  switch (SHORT2FROMMP(mp2)) {
    case SB_LINEUP:
    
      scrollEvent.delta = -1;
      break;

    case SB_LINEDOWN:
    
      scrollEvent.delta = 1;
      break;

    case SB_PAGEUP:
    
      scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
      scrollEvent.delta = -1;
      break;

    case SB_PAGEDOWN:
    
      scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
      scrollEvent.delta = 1;
      break;

    default:
      scrollEvent.delta = 0;
      break;
  }
  DispatchWindowEvent(&scrollEvent);

  return PR_FALSE;
}




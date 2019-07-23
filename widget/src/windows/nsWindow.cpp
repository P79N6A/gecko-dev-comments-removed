



















































#include "nsWindow.h"
#include "nsIAppShell.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsFont.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIScreenManager.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsNativeCharsetUtils.h"
#include "nsWidgetAtoms.h"
#include <windows.h>
#include <process.h>
#include "nsUnicharUtils.h"
#include "prlog.h"
#include "nsISupportsPrimitives.h"
#include "gfxImageSurface.h"
#include "nsIDOMNSUIEvent.h"

#include "cairo.h"
extern "C" {
#include "pixman.h"
}

#ifdef DEBUG_vladimir
#include "nsFunctionTimer.h"
#endif




#include "cairo-features.h"

#if defined(MOZ_SPLASHSCREEN)
#include "nsSplashScreen.h"
#endif

#ifdef WINCE

#include "aygshell.h"
#include "imm.h"

#ifdef WINCE_WINDOWS_MOBILE
#define WINCE_HAVE_SOFTKB
#include "tpcshell.h"
#else
#undef WINCE_HAVE_SOFTKB
#include "winuserm.h"
#endif

#else 

#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"
#include "nsKeyboardLayout.h"
#include "nsNativeDragTarget.h"

#include <pbt.h>
#ifndef PBT_APMRESUMEAUTOMATIC
#define PBT_APMRESUMEAUTOMATIC 0x0012
#endif


#include <mmsystem.h>
#include <zmouse.h>

#endif 


#include <unknwn.h>


#include "nsGfxCIID.h"
#include "resource.h"
#include <commctrl.h>
#include "prtime.h"
#include "gfxContext.h"
#include "gfxWindowsSurface.h"
#include "nsIImage.h"

#ifdef ACCESSIBILITY
#include "OLEIDL.H"
#include <winuser.h>
#ifndef WINABLEAPI
#include <winable.h>
#endif
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessNode.h"
#ifndef WM_GETOBJECT
#define WM_GETOBJECT 0x03d
#endif
#endif

#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIRegion.h"


#include "nsplugindefs.h"


#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsWidgetsCID.h"

#include "nsITimer.h"

#include "nsITheme.h"


#include "nsILocalFile.h"
#include "nsCRT.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"

#include "prprf.h"
#include "prmem.h"

#ifdef NS_ENABLE_TSF
#include "nsTextStore.h"
#endif 
#include "nsIMM32Handler.h"

#include "gfxImageSurface.h"

#ifdef CAIRO_HAS_DDRAW_SURFACE
#include "gfxDDrawSurface.h"


static LPDIRECTDRAW glpDD = NULL;
static LPDIRECTDRAWSURFACE glpDDPrimary = NULL;
static LPDIRECTDRAWCLIPPER glpDDClipper = NULL;
static nsAutoPtr<gfxDDrawSurface> gpDDSurf;


static LPDIRECTDRAWSURFACE glpDDSecondary = NULL;
static DDSURFACEDESC gDDSDSecondary;


static void DDError(const char *msg, HRESULT hr)
{
  
  fprintf(stderr, "direct draw error %s: 0x%08x\n", msg, hr);
}

#endif



#define MAX_RECTS_IN_REGION 100

static nsAutoPtr<PRUint8> gSharedSurfaceData;
static gfxIntSize gSharedSurfaceSize;




#ifdef WINCE

#ifdef WINCE_HAVE_SOFTKB
static PRBool gSoftKeyMenuBar = PR_FALSE;
static PRBool gSoftKeyboardState = PR_FALSE;

static void NotifySoftKbObservers() {
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  if (observerService) {
    SIPINFO sipInfo;
    wchar_t rectBuf[256];
    memset(&sipInfo, 0, sizeof(SIPINFO));
    sipInfo.cbSize = sizeof(SIPINFO);
    if (SipGetInfo(&sipInfo)) {
      _snwprintf(rectBuf, 256, L"{\"left\": %d, \"top\": %d,"
                 L" \"right\": %d, \"bottom\": %d}", 
                 sipInfo.rcVisibleDesktop.left, 
                 sipInfo.rcVisibleDesktop.top, 
                 sipInfo.rcVisibleDesktop.right, 
                 sipInfo.rcVisibleDesktop.bottom);
      observerService->NotifyObservers(nsnull, "softkb-change", rectBuf);
    }
  }
}

static void ToggleSoftKB(PRBool show)
{
  HWND hWndSIP = FindWindowW(L"SipWndClass", NULL );
  if (hWndSIP)
    ::ShowWindow(hWndSIP, show ? SW_SHOW: SW_HIDE);

  hWndSIP = FindWindowW(L"MS_SIPBUTTON", NULL ); 
  if (hWndSIP)
    ShowWindow(hWndSIP, show ? SW_SHOW: SW_HIDE);

  SipShowIM(show ? SIPF_ON : SIPF_OFF);
  NotifySoftKbObservers();
}

static void CreateSoftKeyMenuBar(HWND wnd)
{
  if (!wnd)
    return;
  
  static HWND gSoftKeyMenuBar = nsnull;
  
  if (gSoftKeyMenuBar != nsnull)
    return;
  
  SHMENUBARINFO mbi;
  ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
  mbi.cbSize = sizeof(SHMENUBARINFO);
  mbi.hwndParent = wnd;
  
  
  
  
  
  mbi.nToolBarId = IDC_DUMMY_CE_MENUBAR;
  mbi.hInstRes   = GetModuleHandle(NULL);
  
  if (!SHCreateMenuBar(&mbi))
    return;
  
  SetWindowPos(mbi.hwndMB, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE);
  
  SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
              MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                         SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
  
  SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TSOFT1, 
              MAKELPARAM (SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                          SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
  
  SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TSOFT2, 
              MAKELPARAM (SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                          SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
  
  gSoftKeyMenuBar = mbi.hwndMB;
}
#endif  


#define IDI_APPLICATION MAKEINTRESOURCE(32512)

#define RDW_NOINTERNALPAINT 0

#define SetWindowLongPtrA SetWindowLongW
#define SetWindowLongPtrW SetWindowLongW
#define GetWindowLongPtrW GetWindowLongW
#define GWLP_WNDPROC   GWL_WNDPROC
#define GetPropW       GetProp
#define SetPropW       SetProp
#define RemovePropW    RemoveProp

#define MapVirtualKeyEx(a,b,c) MapVirtualKey(a,b)

inline void FlashWindow(HWND window, BOOL ignore){}
inline int  GetMessageTime() {return 0;}
inline BOOL IsIconic(HWND inWnd){return false;}

typedef struct ECWWindows
{
  LPARAM      params;
  WNDENUMPROC func;
  HWND        parent;
} ECWWindows;

static BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  ECWWindows *myParams = (ECWWindows*) lParam;
  
  if (IsChild(myParams->parent, hwnd))
  {
    return myParams->func(hwnd, myParams->params);
  }
  return TRUE;
}

inline BOOL EnumChildWindows(HWND inParent, WNDENUMPROC inFunc, LPARAM inParam)
{
  ECWWindows myParams;
  myParams.params = inParam;
  myParams.func   = inFunc;
  myParams.parent = inParent;
  
  return EnumWindows(MyEnumWindowsProc, (LPARAM) &myParams);
}

inline BOOL EnumThreadWindows(DWORD inThreadID, WNDENUMPROC inFunc, LPARAM inParam)
{
  return FALSE;
}

#endif  



#ifdef PR_LOGGING
PRLogModuleInfo* sWindowsLog = nsnull;
#endif

static const PRUnichar kMozHeapDumpMessageString[] = L"MOZ_HeapDump";

#define kWindowPositionSlop 20

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#ifndef SPI_GETWHEELSCROLLCHARS
#define SPI_GETWHEELSCROLLCHARS 0x006C
#endif

#ifndef MAPVK_VSC_TO_VK
#define MAPVK_VK_TO_VSC  0
#define MAPVK_VSC_TO_VK  1
#define MAPVK_VK_TO_CHAR 2
#endif

static PRBool IsCursorTranslucencySupported() {
#ifdef WINCE
  return PR_FALSE;
#else
  static PRBool didCheck = PR_FALSE;
  static PRBool isSupported = PR_FALSE;
  if (!didCheck) {
    didCheck = PR_TRUE;
    
    isSupported = GetWindowsVersion() >= 0x501;
  }

  return isSupported;
#endif
}

PRInt32 GetWindowsVersion()
{
#ifdef WINCE
  return 0x500;
#else
  static PRInt32 version = 0;
  static PRBool didCheck = PR_FALSE;

  if (!didCheck)
  {
    didCheck = PR_TRUE;
    OSVERSIONINFOEX osInfo;
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    ::GetVersionEx((OSVERSIONINFO*)&osInfo);
    version = (osInfo.dwMajorVersion & 0xff) << 8 | (osInfo.dwMinorVersion & 0xff);
  }
  return version;
#endif
}


static NS_DEFINE_CID(kCClipboardCID,       NS_CLIPBOARD_CID);
static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);

static const char *sScreenManagerContractID = "@mozilla.org/gfx/screenmanager;1";




PRUint32   nsWindow::sInstanceCount            = 0;

PRBool     nsWindow::gSwitchKeyboardLayout     = PR_FALSE;


PRBool gDisableNativeTheme = PR_FALSE;

typedef enum {
  
  RENDER_GDI = 0,

  
  RENDER_IMAGE_STRETCH32,

  
  RENDER_IMAGE_STRETCH24,

  
  RENDER_DDRAW,

  
  RENDER_IMAGE_DDRAW16,

  
  RENDER_MODE_MAX
} WinRenderMode;




#if defined(WINCE_WINDOWS_MOBILE)
#define DEFAULT_RENDER_MODE   RENDER_IMAGE_DDRAW16
#elif defined(WINCE)
#define DEFAULT_RENDER_MODE   RENDER_DDRAW
#else
#define DEFAULT_RENDER_MODE   RENDER_GDI
#endif

static WinRenderMode gRenderMode = DEFAULT_RENDER_MODE;

#ifndef WINCE
static KeyboardLayout gKbdLayout;
#endif

TriStateBool nsWindow::sCanQuit = TRI_UNKNOWN;

BOOL nsWindow::sIsRegistered       = FALSE;
BOOL nsWindow::sIsPopupClassRegistered = FALSE;
BOOL nsWindow::sIsOleInitialized = FALSE;
UINT nsWindow::uWM_HEAP_DUMP       = 0; 

HCURSOR        nsWindow::gHCursor            = NULL;
imgIContainer* nsWindow::gCursorImgContainer = nsnull;


#ifdef ACCESSIBILITY
BOOL nsWindow::gIsAccessibilityOn = FALSE;
#endif
nsWindow* nsWindow::gCurrentWindow = nsnull;





static nsIRollupListener * gRollupListener           = nsnull;
static nsIWidget         * gRollupWidget             = nsnull;
static PRBool              gRollupConsumeRollupEvent = PR_FALSE;






static HHOOK        gMsgFilterHook = NULL;
static HHOOK        gCallProcHook  = NULL;
static HHOOK        gCallMouseHook = NULL;
static PRPackedBool gProcessHook   = PR_FALSE;
static UINT         gRollupMsgId   = 0;
static HWND         gRollupMsgWnd  = NULL;
static UINT         gHookTimerId   = 0;







static POINT gLastMousePoint;
static POINT gLastMouseMovePoint;
static LONG  gLastMouseDownTime = 0L;
static LONG  gLastClickCount    = 0L;
static BYTE  gLastMouseButton = 0;





#ifndef WINCE
static
#endif
PRUint32 gLastInputEventTime = 0;

static int gTrimOnMinimize = 2; 

#if 0
static PRBool is_vk_down(int vk)
{
   SHORT st = GetKeyState(vk);
#ifdef DEBUG
   printf("is_vk_down vk=%x st=%x\n",vk, st);
#endif
   return (st < 0);
}
#define IS_VK_DOWN is_vk_down
#else
#define IS_VK_DOWN(a) (GetKeyState(a) < 0)
#endif

nsModifierKeyState::nsModifierKeyState()
{
  mIsShiftDown   = IS_VK_DOWN(NS_VK_SHIFT);
  mIsControlDown = IS_VK_DOWN(NS_VK_CONTROL);
  mIsAltDown     = IS_VK_DOWN(NS_VK_ALT);
}






#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND  0x0319
#endif

#ifndef APPCOMMAND_BROWSER_BACKWARD
#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define APPCOMMAND_BROWSER_REFRESH        3
#define APPCOMMAND_BROWSER_STOP           4
#define APPCOMMAND_BROWSER_SEARCH         5
#define APPCOMMAND_BROWSER_FAVORITES      6
#define APPCOMMAND_BROWSER_HOME           7





















#define FAPPCOMMAND_MASK  0xF000

#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))





#endif  

#define VERIFY_WINDOW_STYLE(s) \
  NS_ASSERTION(((s) & (WS_CHILD | WS_POPUP)) != (WS_CHILD | WS_POPUP), \
               "WS_POPUP and WS_CHILD are mutually exclusive")

HWND nsWindow::GetTopLevelHWND(HWND aWnd, PRBool aStopOnDialogOrPopup)
{
  HWND curWnd = aWnd;
  HWND topWnd = NULL;

  while (curWnd) {
    topWnd = curWnd;

    if (aStopOnDialogOrPopup) {
      DWORD_PTR style = ::GetWindowLongPtrW(curWnd, GWL_STYLE);

      VERIFY_WINDOW_STYLE(style);

      if (!(style & WS_CHILD)) 
        break;
    }

    curWnd = ::GetParent(curWnd);       
  }

  return topWnd;
}












BOOL CALLBACK nsWindow::BroadcastMsgToChildren(HWND aWnd, LPARAM aMsg)
{
  WNDPROC winProc = (WNDPROC)::GetWindowLongPtrW(aWnd, GWLP_WNDPROC);
  if (winProc == &nsWindow::WindowProc) {
    
    ::CallWindowProcW(winProc, aWnd, aMsg, 0, 0);
  }
  return TRUE;
}






BOOL CALLBACK nsWindow::BroadcastMsg(HWND aTopWindow, LPARAM aMsg)
{
  
  
  EnumChildWindows(aTopWindow, nsWindow::BroadcastMsgToChildren, aMsg);
  return TRUE;
}




void nsWindow::GlobalMsgWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_SYSCOLORCHANGE:
      
      
      
     ::EnumThreadWindows(GetCurrentThreadId(), nsWindow::BroadcastMsg, msg);
    break;
  }
}








nsWindow::nsWindow() : nsBaseWidget()
{
#ifdef PR_LOGGING
  if (!sWindowsLog)
    sWindowsLog = PR_NewLogModule("nsWindowsWidgets");
#endif

  mWnd                = 0;
  mPaintDC            = 0;
  mPrevWndProc        = NULL;
  mBackground         = ::GetSysColor(COLOR_BTNFACE);
  mBrush              = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
  mForeground         = ::GetSysColor(COLOR_WINDOWTEXT);
  mIsDestroying       = PR_FALSE;
  mDeferredPositioner = NULL;
  mLastPoint.x        = 0;
  mLastPoint.y        = 0;
  mIsVisible          = PR_FALSE;
  mHas3DBorder        = PR_FALSE;
#ifdef MOZ_XUL
  mTransparencyMode   = eTransparencyOpaque;
  mTransparentSurface = nsnull;
  mMemoryDC           = NULL;
#endif
  mWindowType         = eWindowType_child;
  mBorderStyle        = eBorderStyle_default;
  mUnicodeWidget      = PR_TRUE;
  mIsInMouseCapture   = PR_FALSE;
  mIsInMouseWheelProcessing = PR_FALSE;
  mLastSize.width     = 0;
  mLastSize.height    = 0;
  mOldStyle           = 0;
  mOldExStyle         = 0;
  mPainting           = 0;
  mOldIMC             = NULL;
  mIMEEnabled         = nsIWidget::IME_STATUS_ENABLED;
  mIsPluginWindow     = PR_FALSE;
  mPopupType          = ePopupTypeAny;

  mLeadByte = '\0';
  mBlurEventSuppressionLevel = 0;

  static BOOL gbInitGlobalValue = FALSE;
  if (! gbInitGlobalValue) {
    gbInitGlobalValue = TRUE;
#ifndef WINCE
    gKbdLayout.LoadLayout(::GetKeyboardLayout(0));
#endif
    nsIMM32Handler::Initialize();
    
    nsWindow::uWM_HEAP_DUMP = ::RegisterWindowMessageW(kMozHeapDumpMessageString);
  }

  mNativeDragTarget = nsnull;
  mIsTopWidgetWindow = PR_FALSE;
  mLastKeyboardLayout = 0;

#ifdef NS_ENABLE_TSF
  if (!sInstanceCount)
    nsTextStore::Initialize();
#endif 

#ifndef WINCE
  if (!sInstanceCount && SUCCEEDED(::OleInitialize(NULL))) {
    sIsOleInitialized = TRUE;
  }
  NS_ASSERTION(sIsOleInitialized, "***** OLE is not initialized!\n");
#endif

  
  gLastInputEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  sInstanceCount++;
}






nsWindow::~nsWindow()
{
  mIsDestroying = PR_TRUE;
  if (gCurrentWindow == this) {
    gCurrentWindow = nsnull;
  }

  MouseTrailer* mtrailer = nsToolkit::gMouseTrailer;
  if (mtrailer) {
    if (mtrailer->GetMouseTrailerWindow() == mWnd)
      mtrailer->DestroyTimer();

    if (mtrailer->GetCaptureWindow() == mWnd)
      mtrailer->SetCaptureWindow(nsnull);
  }

  
  
  if (NULL != mWnd) {
    Destroy();
  }

  if (mCursor == -1) {
    
    SetCursor(eCursor_standard);
  }

  sInstanceCount--;

#ifdef NS_ENABLE_TSF
  if (!sInstanceCount)
    nsTextStore::Terminate();
#endif 

#ifndef WINCE
  
  
  
  if (sInstanceCount == 0) {
    NS_IF_RELEASE(gCursorImgContainer);

    if (sIsOleInitialized) {
      ::OleFlushClipboard();
      ::OleUninitialize();
      sIsOleInitialized = FALSE;
    }
    nsIMM32Handler::Terminate();
  }

  NS_IF_RELEASE(mNativeDragTarget);
#endif

}

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)

NS_METHOD nsWindow::CaptureMouse(PRBool aCapture)
{
  if (!nsToolkit::gMouseTrailer) {
    NS_ERROR("nsWindow::CaptureMouse called after nsToolkit destroyed");
    return NS_OK;
  }

  if (aCapture) {
    nsToolkit::gMouseTrailer->SetCaptureWindow(mWnd);
    ::SetCapture(mWnd);
  } else {
    nsToolkit::gMouseTrailer->SetCaptureWindow(NULL);
    ::ReleaseCapture();
  }
  mIsInMouseCapture = aCapture;
  return NS_OK;
}








PRInt32 nsWindow::GetHeight(PRInt32 aProposedHeight)
{
  PRInt32 extra = 0;

#if defined(WINCE) && !defined(WINCE_WINDOWS_MOBILE)
  DWORD style = WindowStyle();
  if ((style & WS_SYSMENU) && (style & WS_POPUP)) {
    extra = GetSystemMetrics(SM_CYCAPTION);
  }
#endif

  return aProposedHeight + extra;
}







NS_METHOD nsWindow::BeginResizingChildren(void)
{
  if (NULL == mDeferredPositioner)
    mDeferredPositioner = ::BeginDeferWindowPos(1);
  return NS_OK;
}

NS_METHOD nsWindow::EndResizingChildren(void)
{
  if (NULL != mDeferredPositioner) {
    ::EndDeferWindowPos(mDeferredPositioner);
    mDeferredPositioner = NULL;
  }
  return NS_OK;
}

nsIntPoint nsWindow::WidgetToScreenOffset()
{
  POINT point;
  point.x = 0;
  point.y = 0;
  ::ClientToScreen(mWnd, &point);
  return nsIntPoint(point.x, point.y);
}

LPARAM nsWindow::lParamToScreen(LPARAM lParam)
{
  POINT pt;
  pt.x = GET_X_LPARAM(lParam);
  pt.y = GET_Y_LPARAM(lParam);
  ::ClientToScreen(mWnd, &pt);
  return MAKELPARAM(pt.x, pt.y);
}

LPARAM nsWindow::lParamToClient(LPARAM lParam)
{
  POINT pt;
  pt.x = GET_X_LPARAM(lParam);
  pt.y = GET_Y_LPARAM(lParam);
  ::ScreenToClient(mWnd, &pt);
  return MAKELPARAM(pt.x, pt.y);
}






void nsWindow::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
  if (nsnull == aPoint) {     
    
    if (mWnd != NULL) {

      DWORD pos = ::GetMessagePos();
      POINT cpos;
      
      cpos.x = GET_X_LPARAM(pos);
      cpos.y = GET_Y_LPARAM(pos);

      ::ScreenToClient(mWnd, &cpos);
      event.refPoint.x = cpos.x;
      event.refPoint.y = cpos.y;
    } else {
      event.refPoint.x = 0;
      event.refPoint.y = 0;
    }
  }
  else {                      
    event.refPoint.x = aPoint->x;
    event.refPoint.y = aPoint->y;
  }

  event.time = ::GetMessageTime();

  mLastPoint = event.refPoint;
}



void nsWindow::SuppressBlurEvents(PRBool aSuppress)
{
  if (aSuppress)
    ++mBlurEventSuppressionLevel; 
  else {
    NS_ASSERTION(mBlurEventSuppressionLevel > 0, "unbalanced blur event suppression");
    if (mBlurEventSuppressionLevel > 0)
      --mBlurEventSuppressionLevel;
  }
}

PRBool nsWindow::BlurEventsSuppressed()
{
  
  if (mBlurEventSuppressionLevel > 0)
    return PR_TRUE;

  
  HWND parentWnd = ::GetParent(mWnd);
  if (parentWnd) {
    nsWindow *parent = GetNSWindowPtr(parentWnd);
    if (parent)
      return parent->BlurEventsSuppressed();
  }
  return PR_FALSE;
}







NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
#ifdef NS_DEBUG
  debug_DumpEvent(stdout,
                  event->widget,
                  event,
                  nsCAutoString("something"),
                  (PRInt32) mWnd);
#endif 

  aStatus = nsEventStatus_eIgnore;

  
  if (event->message == NS_DEACTIVATE && BlurEventsSuppressed())
    return NS_OK;

  if (nsnull != mEventCallback) {
    aStatus = (*mEventCallback)(event);
  }

  
  if ((aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener)) {
    aStatus = mEventListener->ProcessEvent(*event);
  }

  
  
  
  if (mOnDestroyCalled)
    aStatus = nsEventStatus_eConsumeNoDefault;
  return NS_OK;
}


PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
{
  nsEventStatus status;
  DispatchEvent(event, status);
  return ConvertStatus(status);
}

PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event, nsEventStatus &aStatus) {
  DispatchEvent(event, aStatus);
  return ConvertStatus(aStatus);
}







PRBool nsWindow::DispatchStandardEvent(PRUint32 aMsg)
{
  nsGUIEvent event(PR_TRUE, aMsg, this);
  InitEvent(event);

  PRBool result = DispatchWindowEvent(&event);
  return result;
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
    case APPCOMMAND_BROWSER_SEARCH:
      command = nsWidgetAtoms::Search;
      break;
    case APPCOMMAND_BROWSER_FAVORITES:
      command = nsWidgetAtoms::Bookmarks;
      break;
    case APPCOMMAND_BROWSER_HOME:
      command = nsWidgetAtoms::Home;
      break;
    default:
      return PR_FALSE;
  }
  nsCommandEvent event(PR_TRUE, nsWidgetAtoms::onAppCommand, command, this);

  InitEvent(event);
  DispatchWindowEvent(&event);

  return PR_TRUE;
}


NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener,
                                            PRBool aDoCapture,
                                            PRBool aConsumeRollupEvent)
{
  if (aDoCapture) {
    


    NS_ASSERTION(!gRollupWidget, "rollup widget reassigned before release");
    gRollupConsumeRollupEvent = aConsumeRollupEvent;
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    gRollupListener = aListener;
    NS_ADDREF(aListener);
    gRollupWidget = this;
    NS_ADDREF(this);

#ifndef WINCE
    if (!gMsgFilterHook && !gCallProcHook && !gCallMouseHook) {
      RegisterSpecialDropdownHooks();
    }
    gProcessHook = PR_TRUE;
#endif
    
  } else {
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    
#ifndef WINCE
    gProcessHook = PR_FALSE;
    UnregisterSpecialDropdownHooks();
#endif
  }

  return NS_OK;
}

PRBool
nsWindow::EventIsInsideWindow(UINT Msg, nsWindow* aWindow)
{
  RECT r;

#ifndef WINCE
  if (Msg == WM_ACTIVATEAPP)
    
    return PR_FALSE;
#else
  if (Msg == WM_ACTIVATE)
    
    
    
    return PR_TRUE;
#endif

  ::GetWindowRect(aWindow->mWnd, &r);
  DWORD pos = ::GetMessagePos();
  POINT mp;
  mp.x = GET_X_LPARAM(pos);
  mp.y = GET_Y_LPARAM(pos);

  
  return (PRBool) PtInRect(&r, mp);
}

static PRUnichar sPropName[40] = L"";
static PRUnichar* GetNSWindowPropName() {
  if (!*sPropName)
  {
    _snwprintf(sPropName, 39, L"MozillansIWidgetPtr%p", GetCurrentProcessId());
    sPropName[39] = '\0';
  }
  return sPropName;
}

nsWindow * nsWindow::GetNSWindowPtr(HWND aWnd) {
  return (nsWindow *) ::GetPropW(aWnd, GetNSWindowPropName());
}

BOOL nsWindow::SetNSWindowPtr(HWND aWnd, nsWindow * ptr) {
  if (ptr == NULL) {
    ::RemovePropW(aWnd, GetNSWindowPropName());
    return TRUE;
  } else {
    return ::SetPropW(aWnd, GetNSWindowPropName(), (HANDLE)ptr);
  }
}






LRESULT CALLBACK nsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  
  
  nsAutoRollup autoRollup;

  LRESULT popupHandlingResult;
  if ( DealWithPopups(hWnd, msg, wParam, lParam, &popupHandlingResult) )
    return popupHandlingResult;

  
  nsWindow *someWindow = GetNSWindowPtr(hWnd);

  
  
  if (nsnull == someWindow) {
    NS_ASSERTION(someWindow, "someWindow is null, cannot call any CallWindowProc");
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
  }

  
  
  
  nsCOMPtr<nsISupports> kungFuDeathGrip;
  if (!someWindow->mIsDestroying) 
    kungFuDeathGrip = do_QueryInterface((nsBaseWidget*)someWindow);

  
  
  if (msg == WM_NOTIFY) {
    LPNMHDR pnmh = (LPNMHDR) lParam;
    if (pnmh->code == TCN_SELCHANGE) {
      someWindow = GetNSWindowPtr(pnmh->hwndFrom);
    }
  }

  if (nsnull != someWindow) {
    LRESULT retValue;
    if (PR_TRUE == someWindow->ProcessMessage(msg, wParam, lParam, &retValue)) {
      return retValue;
    }
  }

  return ::CallWindowProcW(someWindow->GetPrevWindowProc(),
                           hWnd, msg, wParam, lParam);
}








nsresult
nsWindow::StandardWindowCreate(nsIWidget *aParent,
                               const nsIntRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell,
                               nsIToolkit *aToolkit,
                               nsWidgetInitData *aInitData,
                               nsNativeWidget aNativeParent)
{
  nsIWidget *baseParent = aInitData &&
                         (aInitData->mWindowType == eWindowType_dialog ||
                          aInitData->mWindowType == eWindowType_toplevel ||
                          aInitData->mWindowType == eWindowType_invisible) ?
                         nsnull : aParent;

  mIsTopWidgetWindow = (nsnull == baseParent);
  mBounds.width = aRect.width;
  mBounds.height = aRect.height;

  BaseCreate(baseParent, aRect, aHandleEventFunction, aContext,
             aAppShell, aToolkit, aInitData);

  
  

  nsToolkit* toolkit = (nsToolkit *)mToolkit;
  if (toolkit && !toolkit->IsGuiThread()) {
    DWORD_PTR args[7];
    args[0] = (DWORD_PTR)aParent;
    args[1] = (DWORD_PTR)&aRect;
    args[2] = (DWORD_PTR)aHandleEventFunction;
    args[3] = (DWORD_PTR)aContext;
    args[4] = (DWORD_PTR)aAppShell;
    args[5] = (DWORD_PTR)aToolkit;
    args[6] = (DWORD_PTR)aInitData;

    if (nsnull != aParent) {
      
      MethodInfo info(this, nsWindow::CREATE, 7, args);
      toolkit->CallMethod(&info);
      return NS_OK;
    }
    else {
      
      MethodInfo info(this, nsWindow::CREATE_NATIVE, 5, args);
      toolkit->CallMethod(&info);
      return NS_OK;
    }
  }

  HWND parent;
  if (nsnull != aParent) { 
    parent = ((aParent) ? (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW) : nsnull);
  } else { 
    parent = (HWND)aNativeParent;
  }

  if (nsnull != aInitData) {
    SetWindowType(aInitData->mWindowType);
    SetBorderStyle(aInitData->mBorderStyle);
    mPopupType = aInitData->mPopupHint;
  }

  mContentType = aInitData ? aInitData->mContentType : eContentTypeInherit;

  DWORD style = WindowStyle();
  DWORD extendedStyle = WindowExStyle();

  if (mWindowType == eWindowType_popup) {
    
    
    if (aParent)
      extendedStyle = WS_EX_TOOLWINDOW;
    else
      parent = NULL;
  } else if (nsnull != aInitData) {
    
    if (aInitData->clipChildren) {
      style |= WS_CLIPCHILDREN;
    } else {
      style &= ~WS_CLIPCHILDREN;
    }
    if (aInitData->clipSiblings) {
      style |= WS_CLIPSIBLINGS;
    }
  }

  mHas3DBorder = (extendedStyle & WS_EX_CLIENTEDGE) > 0;

  mWnd = ::CreateWindowExW(extendedStyle,
                           aInitData && aInitData->mDropShadow ?
                           WindowPopupClass() : WindowClass(),
                           L"",
                           style,
                           aRect.x,
                           aRect.y,
                           aRect.width,
                           GetHeight(aRect.height),
                           parent,
                           NULL,
                           nsToolkit::mDllInstance,
                           NULL);

  if (!mWnd)
    return NS_ERROR_FAILURE;

  








  

  DispatchStandardEvent(NS_CREATE);
  SubclassWindow(TRUE);

  if (gTrimOnMinimize == 2 && mWindowType == eWindowType_invisible) {
    









    gTrimOnMinimize = 0;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
      nsCOMPtr<nsIPrefBranch> prefBranch;
      prefs->GetBranch(0, getter_AddRefs(prefBranch));
      if (prefBranch) {

        PRBool temp;
        if (NS_SUCCEEDED(prefBranch->GetBoolPref("config.trim_on_minimize",
                                                 &temp))
            && temp)
          gTrimOnMinimize = 1;

        if (NS_SUCCEEDED(prefBranch->GetBoolPref("intl.keyboard.per_window_layout",
                                                 &temp)))
          gSwitchKeyboardLayout = temp;

        if (NS_SUCCEEDED(prefBranch->GetBoolPref("mozilla.widget.disable-native-theme",
                                                 &temp)))
          gDisableNativeTheme = temp;

        PRInt32 tempint;
        if (NS_SUCCEEDED(prefBranch->GetIntPref("mozilla.widget.render-mode",
                                                &tempint)))
        {
          if (tempint > 0 && tempint < RENDER_MODE_MAX) {
#ifndef CAIRO_HAS_DDRAW_SURFACE
            if (tempint == RENDER_DDRAW)
              tempint = RENDER_IMAGE_STRETCH24;
#endif
            gRenderMode = (WinRenderMode) tempint;
          }
        }
      }
    }
  }
#if defined(WINCE_HAVE_SOFTKB)
  if (mWindowType == eWindowType_dialog || mWindowType == eWindowType_toplevel )
     CreateSoftKeyMenuBar(mWnd);
#endif

  
  if (mWindowType != eWindowType_invisible &&
      mWindowType != eWindowType_plugin &&
      mWindowType != eWindowType_java &&
      mWindowType != eWindowType_toplevel) {
    
    
    
    
    mGesture.InitWinGestureSupport(mWnd);
  }

  return NS_OK;
}






NS_METHOD nsWindow::Create(nsIWidget *aParent,
                           const nsIntRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
  if (aInitData)
    mUnicodeWidget = aInitData->mUnicode;
  return(StandardWindowCreate(aParent, aRect, aHandleEventFunction,
                              aContext, aAppShell, aToolkit, aInitData,
                              nsnull));
}








NS_METHOD nsWindow::Create(nsNativeWidget aParent,
                           const nsIntRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
  if (aInitData)
    mUnicodeWidget = aInitData->mUnicode;
  return(StandardWindowCreate(nsnull, aRect, aHandleEventFunction,
                              aContext, aAppShell, aToolkit, aInitData,
                              aParent));
}







NS_METHOD nsWindow::Destroy()
{
  
  
  nsToolkit* toolkit = (nsToolkit *)mToolkit;
  if (toolkit != nsnull && !toolkit->IsGuiThread()) {
    MethodInfo info(this, nsWindow::DESTROY);
    toolkit->CallMethod(&info);
    return NS_ERROR_FAILURE;
  }

  
  if (!mIsDestroying) {
    nsBaseWidget::Destroy();
  }

  
  
  if ( this == gRollupWidget ) {
    if ( gRollupListener )
      gRollupListener->Rollup(nsnull, nsnull);
    CaptureRollupEvents(nsnull, PR_FALSE, PR_TRUE);
  }

  EnableDragDrop(PR_FALSE);

  
  if (mWnd) {
    
    mEventCallback = nsnull;

    
    if (mOldIMC) {
      mOldIMC = ::ImmAssociateContext(mWnd, mOldIMC);
      NS_ASSERTION(!mOldIMC, "Another IMC was associated");
    }

    HICON icon;
    icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM) 0);
    if (icon)
      ::DestroyIcon(icon);

    icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM) 0);
    if (icon)
      ::DestroyIcon(icon);

#ifdef MOZ_XUL
    if (eTransparencyTransparent == mTransparencyMode)
    {
      SetupTranslucentWindowMemoryBitmap(eTransparencyOpaque);

    }
#endif

    VERIFY(::DestroyWindow(mWnd));

    mWnd = NULL;
    
    
    
    
    
    if (PR_FALSE == mOnDestroyCalled)
      OnDestroy();
  }

  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetParent(nsIWidget *aNewParent)
{
  if (aNewParent) {
    nsCOMPtr<nsIWidget> kungFuDeathGrip(this);

    nsIWidget* parent = GetParent();
    if (parent) {
      parent->RemoveChild(this);
    }

    HWND newParent = (HWND)aNewParent->GetNativeData(NS_NATIVE_WINDOW);
    NS_ASSERTION(newParent, "Parent widget has a null native window handle");
    if (newParent && mWnd) {
      ::SetParent(mWnd, newParent);
    }

    aNewParent->AddChild(this);

    return NS_OK;
  }

  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);

  nsIWidget* parent = GetParent();

  if (parent) {
    parent->RemoveChild(this);
  }

  if (mWnd) {
    ::SetParent(mWnd, nsnull);
  }

  return NS_OK;
}







nsIWidget* nsWindow::GetParent(void)
{
  return GetParentWindow(PR_FALSE);
}

nsWindow* nsWindow::GetParentWindow(PRBool aIncludeOwner)
{
  if (mIsTopWidgetWindow) {
    
    
    
    return nsnull;
  }

  
  
  
  if (mIsDestroying || mOnDestroyCalled)
    return nsnull;


  
  
  
  nsWindow* widget = nsnull;
  if (mWnd) {
#ifdef WINCE
    HWND parent = ::GetParent(mWnd);
#else
    HWND parent = nsnull;
    if (aIncludeOwner)
      parent = ::GetParent(mWnd);
    else
      parent = ::GetAncestor(mWnd, GA_PARENT);
#endif
    if (parent) {
      widget = GetNSWindowPtr(parent);
      if (widget) {
        
        
        if (widget->mIsDestroying) {
          widget = nsnull;
        }
      }
    }
  }

  return widget;
}







PRBool gWindowsVisible;

static BOOL CALLBACK gEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  DWORD pid;
  ::GetWindowThreadProcessId(hwnd, &pid);
  if (pid == GetCurrentProcessId() && ::IsWindowVisible(hwnd))
  {
    gWindowsVisible = PR_TRUE;
    return FALSE;
  }
  return TRUE;
}

PRBool nsWindow::CanTakeFocus()
{
  gWindowsVisible = PR_FALSE;
  EnumWindows(gEnumWindowsProc, 0);
  if (!gWindowsVisible) {
    return PR_TRUE;
  } else {
    HWND fgWnd = ::GetForegroundWindow();
    if (!fgWnd) {
      return PR_TRUE;
    }
    DWORD pid;
    GetWindowThreadProcessId(fgWnd, &pid);
    if (pid == GetCurrentProcessId()) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

NS_METHOD nsWindow::Show(PRBool bState)
{
#if defined(MOZ_SPLASHSCREEN)
  
  
  nsSplashScreen *splash = nsSplashScreen::Get();
  if (splash && splash->IsOpen() && mWnd && bState &&
      (mWindowType == eWindowType_toplevel ||
       mWindowType == eWindowType_dialog ||
       mWindowType == eWindowType_popup))
  {
    splash->Close();
  }
#endif

  PRBool wasVisible = mIsVisible;
  
  
  mIsVisible = bState;

  if (mWnd) {
    if (bState) {
      if (!wasVisible && mWindowType == eWindowType_toplevel) {
        switch (mSizeMode) {
#ifdef WINCE
          case nsSizeMode_Maximized :
            ::SetForegroundWindow(mWnd);
            ::ShowWindow(mWnd, SW_SHOWMAXIMIZED);
            break;
          
#else
          case nsSizeMode_Maximized :
            ::ShowWindow(mWnd, SW_SHOWMAXIMIZED);
            break;
          case nsSizeMode_Minimized :
            ::ShowWindow(mWnd, SW_SHOWMINIMIZED);
            break;
#endif
          default:
            if (CanTakeFocus()) {
#ifdef WINCE
              ::SetForegroundWindow(mWnd);
#endif
              ::ShowWindow(mWnd, SW_SHOWNORMAL);
            } else {
              
              
              HWND wndAfter = ::GetForegroundWindow();
              if (!wndAfter)
                wndAfter = HWND_BOTTOM;
              else if (GetWindowLongPtrW(wndAfter, GWL_EXSTYLE) & WS_EX_TOPMOST)
                wndAfter = HWND_TOP;
              ::SetWindowPos(mWnd, wndAfter, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | 
                             SWP_NOMOVE | SWP_NOACTIVATE);
              GetAttention(2);
            }
            break;
        }
      } else {
        DWORD flags = SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW;
        if (wasVisible)
          flags |= SWP_NOZORDER;

        if (mWindowType == eWindowType_popup) {
#ifndef WINCE
          
          
          
          
          
          flags |= SWP_NOACTIVATE;
#endif
          HWND owner = ::GetWindow(mWnd, GW_OWNER);
          ::SetWindowPos(mWnd, owner ? 0 : HWND_TOPMOST, 0, 0, 0, 0, flags);
        } else {
#ifndef WINCE
          if (mWindowType == eWindowType_dialog && !CanTakeFocus())
            flags |= SWP_NOACTIVATE;
#endif
          ::SetWindowPos(mWnd, HWND_TOP, 0, 0, 0, 0, flags);
        }
      }
    } else {
      if (mWindowType != eWindowType_dialog) {
        ::ShowWindow(mWnd, SW_HIDE);
      } else {
        ::SetWindowPos(mWnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE |
                       SWP_NOZORDER | SWP_NOACTIVATE);
      }
    }
  }
  
#ifdef MOZ_XUL
  if (!wasVisible && bState)
    Invalidate(PR_FALSE);
#endif

  return NS_OK;
}






NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
  bState = mIsVisible;
  return NS_OK;
}






NS_METHOD nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                nsIWidget *aWidget, PRBool aActivate)
{
  HWND behind = HWND_TOP;
  if (aPlacement == eZPlacementBottom)
    behind = HWND_BOTTOM;
  else if (aPlacement == eZPlacementBelow && aWidget)
    behind = (HWND)aWidget->GetNativeData(NS_NATIVE_WINDOW);
  UINT flags = SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE;
  if (!aActivate)
    flags |= SWP_NOACTIVATE;

  if (!CanTakeFocus() && behind == HWND_TOP)
  {
    
    
    HWND wndAfter = ::GetForegroundWindow();
    if (!wndAfter)
      behind = HWND_BOTTOM;
    else if (!(GetWindowLongPtrW(wndAfter, GWL_EXSTYLE) & WS_EX_TOPMOST))
      behind = wndAfter;
    flags |= SWP_NOACTIVATE;
  }

  ::SetWindowPos(mWnd, behind, 0, 0, 0, 0, flags);
  return NS_OK;
}






NS_IMETHODIMP nsWindow::SetSizeMode(PRInt32 aMode) {

  nsresult rv;

  
  
  
  if (aMode == mSizeMode)
    return NS_OK;

#ifdef WINCE_WINDOWS_MOBILE
  
  
  if (mWindowType == eWindowType_dialog || mWindowType == eWindowType_toplevel) {
    aMode = nsSizeMode_Maximized;
  }
#endif

  
  rv = nsBaseWidget::SetSizeMode(aMode);
  if (NS_SUCCEEDED(rv) && mIsVisible) {
    int mode;

    switch (aMode) {
      case nsSizeMode_Maximized :
        mode = SW_MAXIMIZE;
        break;
#ifndef WINCE
      case nsSizeMode_Minimized :
        mode = gTrimOnMinimize ? SW_MINIMIZE : SW_SHOWMINIMIZED;
        if (!gTrimOnMinimize) {
          
          HWND hwndBelow = ::GetNextWindow(mWnd, GW_HWNDNEXT);
          while (hwndBelow && (!::IsWindowVisible(hwndBelow) ||
                               ::IsIconic(hwndBelow))) {
            hwndBelow = ::GetNextWindow(hwndBelow, GW_HWNDNEXT);
          }

          
          
          ::SetWindowPos(mWnd, HWND_BOTTOM, 0, 0, 0, 0,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
          if (hwndBelow)
            ::SetForegroundWindow(hwndBelow);

          
          
          ::PlaySoundW(L"Minimize", nsnull, SND_ALIAS | SND_NODEFAULT | SND_ASYNC);
        }
        break;
#endif
      default :
        mode = SW_RESTORE;
    }
    ::ShowWindow(mWnd, mode);
  }
  return rv;
}






NS_METHOD nsWindow::ConstrainPosition(PRBool aAllowSlop,
                                      PRInt32 *aX, PRInt32 *aY)
{
  if (!mIsTopWidgetWindow) 
    return NS_OK;

  PRBool doConstrain = PR_FALSE; 

  

  RECT screenRect;

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    PRInt32 left, top, width, height;

    
    width = mBounds.width > 0 ? mBounds.width : 1;
    height = mBounds.height > 0 ? mBounds.height : 1;
    screenmgr->ScreenForRect(*aX, *aY, width, height,
                             getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailRect(&left, &top, &width, &height);
      screenRect.left = left;
      screenRect.right = left+width;
      screenRect.top = top;
      screenRect.bottom = top+height;
      doConstrain = PR_TRUE;
    }
  } else {
    if (mWnd) {
      HDC dc = ::GetDC(mWnd);
      if (dc) {
        if (::GetDeviceCaps(dc, TECHNOLOGY) == DT_RASDISPLAY) {
          ::SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
          doConstrain = PR_TRUE;
        }
        ::ReleaseDC(mWnd, dc);
      }
    }
  }

  if (aAllowSlop) {
    if (*aX < screenRect.left - mBounds.width + kWindowPositionSlop)
      *aX = screenRect.left - mBounds.width + kWindowPositionSlop;
    else if (*aX >= screenRect.right - kWindowPositionSlop)
      *aX = screenRect.right - kWindowPositionSlop;

    if (*aY < screenRect.top - mBounds.height + kWindowPositionSlop)
      *aY = screenRect.top - mBounds.height + kWindowPositionSlop;
    else if (*aY >= screenRect.bottom - kWindowPositionSlop)
      *aY = screenRect.bottom - kWindowPositionSlop;

  } else {

    if (*aX < screenRect.left)
      *aX = screenRect.left;
    else if (*aX >= screenRect.right - mBounds.width)
      *aX = screenRect.right - mBounds.width;

    if (*aY < screenRect.top)
      *aY = screenRect.top;
    else if (*aY >= screenRect.bottom - mBounds.height)
      *aY = screenRect.bottom - mBounds.height;
  }

  return NS_OK;
}




void nsWindow::ClearThemeRegion()
{
#ifndef WINCE
  if (nsUXThemeData::sIsVistaOrLater && mTransparencyMode != eTransparencyGlass &&
      mWindowType == eWindowType_popup && (mPopupType == ePopupTypeTooltip || mPopupType == ePopupTypePanel)) {
    SetWindowRgn(mWnd, NULL, false);
  }
#endif
}

void nsWindow::SetThemeRegion()
{
#ifndef WINCE
  
  
  
  
  
  if (nsUXThemeData::sIsVistaOrLater && mTransparencyMode != eTransparencyGlass &&
      mWindowType == eWindowType_popup && (mPopupType == ePopupTypeTooltip || mPopupType == ePopupTypePanel)) {
    HRGN hRgn = nsnull;
    RECT rect = {0,0,mBounds.width,mBounds.height};
    
    nsUXThemeData::getThemeBackgroundRegion(nsUXThemeData::GetTheme(eUXTooltip), GetDC(mWnd), TTP_STANDARD, TS_NORMAL, &rect, &hRgn);
    if (hRgn) {
      if (!SetWindowRgn(mWnd, hRgn, false)) 
        DeleteObject(hRgn);
    }
  }
#endif
}






NS_METHOD nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
  
  
  
  

  
  
  
  if (mWindowType != eWindowType_popup && (mBounds.x == aX) && (mBounds.y == aY))
  {
    
    return NS_OK;
  }

  mBounds.x = aX;
  mBounds.y = aY;

  if (mWnd) {
#ifdef DEBUG
    
    if (mIsTopWidgetWindow) { 
      
      
      HDC dc = ::GetDC(mWnd);
      if (dc) {
        if (::GetDeviceCaps(dc, TECHNOLOGY) == DT_RASDISPLAY) {
          RECT workArea;
          ::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
          
          if (aX < 0 || aX >= workArea.right || aY < 0 || aY >= workArea.bottom)
            printf("window moved to offscreen position\n");
        }
      ::ReleaseDC(mWnd, dc);
      }
    }
#endif

    nsIWidget *par = GetParent();
    HDWP      deferrer = NULL;

    if (nsnull != par) {
      deferrer = ((nsWindow *)par)->mDeferredPositioner;
    }

    if (NULL != deferrer) {
      VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                            mWnd, NULL, aX, aY, 0, 0,
                            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
    }
    else {
      ClearThemeRegion();
      VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, 0, 0,
                            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
      SetThemeRegion();
    }
  }
  return NS_OK;
}






NS_METHOD nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  NS_ASSERTION((aWidth >=0 ) , "Negative width passed to nsWindow::Resize");
  NS_ASSERTION((aHeight >=0 ), "Negative height passed to nsWindow::Resize");

#ifdef MOZ_XUL
  if (eTransparencyTransparent == mTransparencyMode)
    ResizeTranslucentWindow(aWidth, aHeight);
#endif

  
  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  if (mWnd) {
    nsIWidget *par = GetParent();
    HDWP      deferrer = NULL;

    if (nsnull != par) {
      deferrer = ((nsWindow *)par)->mDeferredPositioner;
    }

    UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;
#ifndef WINCE
    if (!aRepaint) {
      flags |= SWP_NOREDRAW;
    }
#endif

    if (NULL != deferrer) {
      VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                            mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), flags));
    }
    else {
      ClearThemeRegion();
      VERIFY(::SetWindowPos(mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), flags));
      SetThemeRegion();
    }
  }

  if (aRepaint)
    Invalidate(PR_FALSE);

  return NS_OK;
}







NS_METHOD nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  NS_ASSERTION((aWidth >=0 ),  "Negative width passed to nsWindow::Resize");
  NS_ASSERTION((aHeight >=0 ), "Negative height passed to nsWindow::Resize");

#ifdef MOZ_XUL
  if (eTransparencyTransparent == mTransparencyMode)
    ResizeTranslucentWindow(aWidth, aHeight);
#endif

  
  mBounds.x      = aX;
  mBounds.y      = aY;
  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  if (mWnd) {
    nsIWidget *par = GetParent();
    HDWP      deferrer = NULL;

    if (nsnull != par) {
      deferrer = ((nsWindow *)par)->mDeferredPositioner;
    }

    UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE;
#ifndef WINCE
    if (!aRepaint) {
      flags |= SWP_NOREDRAW;
    }
#endif

    if (NULL != deferrer) {
      VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                            mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), flags));
    }
    else {
      ClearThemeRegion();
      VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), flags));
      SetThemeRegion();
    }
  }

  if (aRepaint)
    Invalidate(PR_FALSE);

  return NS_OK;
}







NS_METHOD nsWindow::Enable(PRBool bState)
{
  if (mWnd) {
    ::EnableWindow(mWnd, bState);
  }
  return NS_OK;
}


NS_METHOD nsWindow::IsEnabled(PRBool *aState)
{
  NS_ENSURE_ARG_POINTER(aState);

#ifndef WINCE
  *aState = !mWnd || (::IsWindowEnabled(mWnd) && ::IsWindowEnabled(::GetAncestor(mWnd, GA_ROOT)));
#else
  *aState = !mWnd || (::IsWindowEnabled(mWnd) && ::IsWindowEnabled(mWnd));
#endif

  return NS_OK;
}







NS_METHOD nsWindow::SetFocus(PRBool aRaise)
{
  
  
  
  
  nsToolkit* toolkit = (nsToolkit *)mToolkit;
  NS_ASSERTION(toolkit != nsnull, "This should never be null!"); 
  if (toolkit != nsnull && !toolkit->IsGuiThread()) {
    MethodInfo info(this, nsWindow::SET_FOCUS);
    toolkit->CallMethod(&info);
    return NS_ERROR_FAILURE;
  }

  if (mWnd) {
    
    HWND toplevelWnd = GetTopLevelHWND(mWnd);
    if (::IsIconic(toplevelWnd))
      ::ShowWindow(toplevelWnd, SW_RESTORE);
    ::SetFocus(mWnd);
  }
  return NS_OK;
}







NS_METHOD nsWindow::GetBounds(nsIntRect &aRect)
{
  if (mWnd) {
    RECT r;
    VERIFY(::GetWindowRect(mWnd, &r));

    
    aRect.width  = r.right - r.left;
    aRect.height = r.bottom - r.top;

    
    HWND parent = ::GetParent(mWnd);
    if (parent) {
      RECT pr;
      VERIFY(::GetWindowRect(parent, &pr));
      r.left -= pr.left;
      r.top  -= pr.top;
    }
    aRect.x = r.left;
    aRect.y = r.top;
  } else {
    aRect = mBounds;
  }

  return NS_OK;
}






NS_METHOD nsWindow::GetClientBounds(nsIntRect &aRect)
{
  if (mWnd) {
    RECT r;
    VERIFY(::GetClientRect(mWnd, &r));

    
    aRect.x = 0;
    aRect.y = 0;
    aRect.width  = r.right - r.left;
    aRect.height = r.bottom - r.top;

  } else {
    aRect.SetRect(0,0,0,0);
  }
  return NS_OK;
}



void nsWindow::GetNonClientBounds(nsIntRect &aRect)
{
  if (mWnd) {
    RECT r;
    VERIFY(::GetWindowRect(mWnd, &r));

    
    aRect.width = r.right - r.left;
    aRect.height = r.bottom - r.top;

    
    HWND parent = ::GetParent(mWnd);
    if (parent) {
      RECT pr;
      VERIFY(::GetWindowRect(parent, &pr));
      r.left -= pr.left;
      r.top -= pr.top;
    }
    aRect.x = r.left;
    aRect.y = r.top;
  } else {
    aRect.SetRect(0,0,0,0);
  }
}


NS_METHOD nsWindow::GetScreenBounds(nsIntRect &aRect)
{
  if (mWnd) {
    RECT r;
    VERIFY(::GetWindowRect(mWnd, &r));

    aRect.width  = r.right - r.left;
    aRect.height = r.bottom - r.top;
    aRect.x = r.left;
    aRect.y = r.top;
  } else
    aRect = mBounds;

  return NS_OK;
}






NS_METHOD nsWindow::SetBackgroundColor(const nscolor &aColor)
{
  nsBaseWidget::SetBackgroundColor(aColor);

  if (mBrush)
    ::DeleteObject(mBrush);

  mBrush = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
#ifndef WINCE
  if (mWnd != NULL) {
    ::SetClassLongPtrW(mWnd, GCLP_HBRBACKGROUND, (LONG_PTR)mBrush);
  }
#endif
  return NS_OK;
}







NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  

  
  
  
  HCURSOR newCursor = NULL;

  switch (aCursor) {
    case eCursor_select:
      newCursor = ::LoadCursor(NULL, IDC_IBEAM);
      break;

    case eCursor_wait:
      newCursor = ::LoadCursor(NULL, IDC_WAIT);
      break;

    case eCursor_hyperlink:
    {
      newCursor = ::LoadCursor(NULL, IDC_HAND);
      break;
    }

    case eCursor_standard:
      newCursor = ::LoadCursor(NULL, IDC_ARROW);
      break;

    case eCursor_n_resize:
    case eCursor_s_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZENS);
      break;

    case eCursor_w_resize:
    case eCursor_e_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZEWE);
      break;

    case eCursor_nw_resize:
    case eCursor_se_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZENWSE);
      break;

    case eCursor_ne_resize:
    case eCursor_sw_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZENESW);
      break;

    case eCursor_crosshair:
      newCursor = ::LoadCursor(NULL, IDC_CROSS);
      break;

    case eCursor_move:
      newCursor = ::LoadCursor(NULL, IDC_SIZEALL);
      break;

    case eCursor_help:
      newCursor = ::LoadCursor(NULL, IDC_HELP);
      break;

    case eCursor_copy: 
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_COPY));
      break;

    case eCursor_alias:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ALIAS));
      break;

    case eCursor_cell:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_CELL));
      break;

    case eCursor_grab:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_GRAB));
      break;

    case eCursor_grabbing:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_GRABBING));
      break;

    case eCursor_spinning:
      newCursor = ::LoadCursor(NULL, IDC_APPSTARTING);
      break;

    case eCursor_context_menu:
      
      break;

    case eCursor_zoom_in:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ZOOMIN));
      break;

    case eCursor_zoom_out:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ZOOMOUT));
      break;

    case eCursor_not_allowed:
    case eCursor_no_drop:
      newCursor = ::LoadCursor(NULL, IDC_NO);
      break;

    case eCursor_col_resize:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_COLRESIZE));
      break;

    case eCursor_row_resize:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ROWRESIZE));
      break;

    case eCursor_vertical_text:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_VERTICALTEXT));
      break;

    case eCursor_all_scroll:
      
      newCursor = ::LoadCursor(NULL, IDC_SIZEALL);
      break;

    case eCursor_nesw_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZENESW);
      break;

    case eCursor_nwse_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZENWSE);
      break;

    case eCursor_ns_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZENS);
      break;

    case eCursor_ew_resize:
      newCursor = ::LoadCursor(NULL, IDC_SIZEWE);
      break;

    case eCursor_none:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_NONE));
      break;

    default:
      NS_ERROR("Invalid cursor type");
      break;
  }

  if (NULL != newCursor) {
    mCursor = aCursor;
    HCURSOR oldCursor = ::SetCursor(newCursor);
    
    if (gHCursor == oldCursor) {
      NS_IF_RELEASE(gCursorImgContainer);
      if (gHCursor != NULL)
        ::DestroyIcon(gHCursor);
      gHCursor = NULL;
    }
  }

  return NS_OK;
}

static PRUint8* Data32BitTo1Bit(PRUint8* aImageData,
                                PRUint32 aWidth, PRUint32 aHeight)
{
  
  
  PRUint32 outBpr = ((aWidth + 31) / 8) & ~3;

  
  PRUint8* outData = (PRUint8*)PR_Calloc(outBpr, aHeight);
  if (!outData)
    return NULL;

  PRInt32 *imageRow = (PRInt32*)aImageData;
  for (PRUint32 curRow = 0; curRow < aHeight; curRow++) {
    PRUint8 *outRow = outData + curRow * outBpr;
    PRUint8 mask = 0x80;
    for (PRUint32 curCol = 0; curCol < aWidth; curCol++) {
      
      if (*imageRow++ < 0)
        *outRow |= mask;

      mask >>= 1;
      if (!mask) {
        outRow ++;
        mask = 0x80;
      }
    }
  }

  return outData;
}
















static HBITMAP DataToBitmap(PRUint8* aImageData,
                            PRUint32 aWidth,
                            PRUint32 aHeight,
                            PRUint32 aDepth)
{
#ifndef WINCE
  HDC dc = ::GetDC(NULL);

  if (aDepth == 32 && IsCursorTranslucencySupported()) {
    
    BITMAPV4HEADER head = { 0 };
    head.bV4Size = sizeof(head);
    head.bV4Width = aWidth;
    head.bV4Height = aHeight;
    head.bV4Planes = 1;
    head.bV4BitCount = aDepth;
    head.bV4V4Compression = BI_BITFIELDS;
    head.bV4SizeImage = 0; 
    head.bV4XPelsPerMeter = 0;
    head.bV4YPelsPerMeter = 0;
    head.bV4ClrUsed = 0;
    head.bV4ClrImportant = 0;

    head.bV4RedMask   = 0x00FF0000;
    head.bV4GreenMask = 0x0000FF00;
    head.bV4BlueMask  = 0x000000FF;
    head.bV4AlphaMask = 0xFF000000;

    HBITMAP bmp = ::CreateDIBitmap(dc,
                                   reinterpret_cast<CONST BITMAPINFOHEADER*>(&head),
                                   CBM_INIT,
                                   aImageData,
                                   reinterpret_cast<CONST BITMAPINFO*>(&head),
                                   DIB_RGB_COLORS);
    ::ReleaseDC(NULL, dc);
    return bmp;
  }

  char reserved_space[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2];
  BITMAPINFOHEADER& head = *(BITMAPINFOHEADER*)reserved_space;

  head.biSize = sizeof(BITMAPINFOHEADER);
  head.biWidth = aWidth;
  head.biHeight = aHeight;
  head.biPlanes = 1;
  head.biBitCount = (WORD)aDepth;
  head.biCompression = BI_RGB;
  head.biSizeImage = 0; 
  head.biXPelsPerMeter = 0;
  head.biYPelsPerMeter = 0;
  head.biClrUsed = 0;
  head.biClrImportant = 0;
  
  BITMAPINFO& bi = *(BITMAPINFO*)reserved_space;

  if (aDepth == 1) {
    RGBQUAD black = { 0, 0, 0, 0 };
    RGBQUAD white = { 255, 255, 255, 0 };

    bi.bmiColors[0] = white;
    bi.bmiColors[1] = black;
  }

  HBITMAP bmp = ::CreateDIBitmap(dc, &head, CBM_INIT, aImageData, &bi, DIB_RGB_COLORS);
  ::ReleaseDC(NULL, dc);
  return bmp;
#else
  return nsnull;
#endif
}

NS_IMETHODIMP nsWindow::SetCursor(imgIContainer* aCursor,
                                  PRUint32 aHotspotX, PRUint32 aHotspotY)
{
  if (gCursorImgContainer == aCursor && gHCursor) {
    ::SetCursor(gHCursor);
    return NS_OK;
  }

  
  nsCOMPtr<gfxIImageFrame> frame;
  aCursor->GetFrameAt(0, getter_AddRefs(frame));
  if (!frame)
    return NS_ERROR_NOT_AVAILABLE;

  PRInt32 width, height;
  frame->GetWidth(&width);
  frame->GetHeight(&height);

  
  
  
  
  if (width > 128 || height > 128)
    return NS_ERROR_NOT_AVAILABLE;

  frame->LockImageData();

  PRUint32 dataLen;
  PRUint8 *data;
  nsresult rv = frame->GetImageData(&data, &dataLen);
  if (NS_FAILED(rv)) {
    frame->UnlockImageData();
    return rv;
  }

  HBITMAP bmp = DataToBitmap(data, width, -height, 32);
  PRUint8* a1data = Data32BitTo1Bit(data, width, height);
  frame->UnlockImageData();
  if (!a1data) {
    return NS_ERROR_FAILURE;
  }

  HBITMAP mbmp = DataToBitmap(a1data, width, -height, 1);
  PR_Free(a1data);

  ICONINFO info = {0};
  info.fIcon = FALSE;
  info.xHotspot = aHotspotX;
  info.yHotspot = aHotspotY;
  info.hbmMask = mbmp;
  info.hbmColor = bmp;
  
  HCURSOR cursor = ::CreateIconIndirect(&info);
  ::DeleteObject(mbmp);
  ::DeleteObject(bmp);
  if (cursor == NULL) {
    return NS_ERROR_FAILURE;
  }

  mCursor = nsCursor(-1);
  ::SetCursor(cursor);

  NS_IF_RELEASE(gCursorImgContainer);
  gCursorImgContainer = aCursor;
  NS_ADDREF(gCursorImgContainer);

  if (gHCursor != NULL)
    ::DestroyIcon(gHCursor);
  gHCursor = cursor;

  return NS_OK;
}

NS_IMETHODIMP nsWindow::HideWindowChrome(PRBool aShouldHide)
{
  HWND hwnd = GetTopLevelHWND(mWnd, PR_TRUE);
  if (!GetNSWindowPtr(hwnd))
  {
    NS_WARNING("Trying to hide window decorations in an embedded context");
    return NS_ERROR_FAILURE;
  }

  DWORD_PTR style, exStyle;
  if (aShouldHide) {
    DWORD_PTR tempStyle = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
    DWORD_PTR tempExStyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    style = tempStyle & ~(WS_CAPTION | WS_THICKFRAME);
    exStyle = tempExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
                              WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

    mOldStyle = tempStyle;
    mOldExStyle = tempExStyle;
  }
  else {
    if (!mOldStyle || !mOldExStyle) {
      mOldStyle = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
      mOldExStyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    }

    style = mOldStyle;
    exStyle = mOldExStyle;
  }

  VERIFY_WINDOW_STYLE(style);
  ::SetWindowLongPtrW(hwnd, GWL_STYLE, style);
  ::SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);

  return NS_OK;
}







NS_METHOD nsWindow::Validate()
{
  if (mWnd)
    VERIFY(::ValidateRect(mWnd, NULL));
  return NS_OK;
}






NS_METHOD nsWindow::Invalidate(PRBool aIsSynchronous)
{
  if (mWnd)
  {
#ifdef NS_DEBUG
    debug_DumpInvalidate(stdout,
                         this,
                         nsnull,
                         aIsSynchronous,
                         nsCAutoString("noname"),
                         (PRInt32) mWnd);
#endif 

    VERIFY(::InvalidateRect(mWnd, NULL, FALSE));

    if (aIsSynchronous) {
      VERIFY(::UpdateWindow(mWnd));
    }
  }
  return NS_OK;
}






NS_METHOD nsWindow::Invalidate(const nsIntRect & aRect, PRBool aIsSynchronous)
{
  if (mWnd)
  {
#ifdef NS_DEBUG
    debug_DumpInvalidate(stdout,
                         this,
                         &aRect,
                         aIsSynchronous,
                         nsCAutoString("noname"),
                         (PRInt32) mWnd);
#endif 

    RECT rect;

    rect.left   = aRect.x;
    rect.top    = aRect.y;
    rect.right  = aRect.x + aRect.width;
    rect.bottom = aRect.y + aRect.height;

    VERIFY(::InvalidateRect(mWnd, &rect, FALSE));

    if (aIsSynchronous) {
      VERIFY(::UpdateWindow(mWnd));
    }
  }
  return NS_OK;
}






NS_IMETHODIMP nsWindow::Update()
{
  nsresult rv = NS_OK;

  
  
  if (mWnd)
    VERIFY(::UpdateWindow(mWnd));

  return rv;
}






void* nsWindow::GetNativeData(PRUint32 aDataType)
{
  switch (aDataType) {
    case NS_NATIVE_PLUGIN_PORT:
      mIsPluginWindow = 1;
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
      return (void*)mWnd;
    case NS_NATIVE_GRAPHIC:
      
#ifdef MOZ_XUL
      return (void*)(eTransparencyTransparent == mTransparencyMode) ?
        mMemoryDC : ::GetDC(mWnd);
#else
      return (void*)::GetDC(mWnd);
#endif

#ifdef NS_ENABLE_TSF
    case NS_NATIVE_TSF_THREAD_MGR:
      return nsTextStore::GetThreadMgr();
    case NS_NATIVE_TSF_CATEGORY_MGR:
      return nsTextStore::GetCategoryMgr();
    case NS_NATIVE_TSF_DISPLAY_ATTR_MGR:
      return nsTextStore::GetDisplayAttrMgr();
#endif 

    default:
      break;
  }

  return NULL;
}


void nsWindow::FreeNativeData(void * data, PRUint32 aDataType)
{
  switch (aDataType)
  {
    case NS_NATIVE_GRAPHIC:
#ifdef MOZ_XUL
      if (eTransparencyTransparent != mTransparencyMode)
        ::ReleaseDC(mWnd, (HDC)data);
#else
      ::ReleaseDC(mWnd, (HDC)data);
#endif
      break;
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_PLUGIN_PORT:
      break;
    default:
      break;
  }
}




BOOL CALLBACK nsWindow::InvalidateForeignChildWindows(HWND aWnd, LPARAM aMsg)
{
  LONG_PTR proc = ::GetWindowLongPtrW(aWnd, GWLP_WNDPROC);
  if (proc != (LONG_PTR)&nsWindow::WindowProc) {
    
    VERIFY(::InvalidateRect(aWnd, NULL, FALSE));    
  }
  return TRUE;
}







NS_METHOD nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsIntRect *aClipRect)
{
  RECT  trect;

  if (nsnull != aClipRect)
  {
    trect.left = aClipRect->x;
    trect.top = aClipRect->y;
    trect.right = aClipRect->XMost();
    trect.bottom = aClipRect->YMost();
  }

  ::ScrollWindowEx(mWnd, aDx, aDy, NULL, (nsnull != aClipRect) ? &trect : NULL,
                   NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
  
  
  
  ::EnumChildWindows(GetWindowHandle(), nsWindow::InvalidateForeignChildWindows, NULL);
  ::UpdateWindow(mWnd);
  return NS_OK;
}







BOOL nsWindow::CallMethod(MethodInfo *info)
{
  BOOL bRet = TRUE;

  switch (info->methodId) {
    case nsWindow::CREATE:
      NS_ASSERTION(info->nArgs == 7, "Wrong number of arguments to CallMethod");
      Create((nsIWidget*)(info->args[0]),
             (nsIntRect&)*(nsIntRect*)(info->args[1]),
             (EVENT_CALLBACK)(info->args[2]),
             (nsIDeviceContext*)(info->args[3]),
             (nsIAppShell *)(info->args[4]),
             (nsIToolkit*)(info->args[5]),
             (nsWidgetInitData*)(info->args[6]));
      break;

    case nsWindow::CREATE_NATIVE:
      NS_ASSERTION(info->nArgs == 7, "Wrong number of arguments to CallMethod");
      Create((nsNativeWidget)(info->args[0]),
             (nsIntRect&)*(nsIntRect*)(info->args[1]),
             (EVENT_CALLBACK)(info->args[2]),
             (nsIDeviceContext*)(info->args[3]),
             (nsIAppShell *)(info->args[4]),
             (nsIToolkit*)(info->args[5]),
             (nsWidgetInitData*)(info->args[6]));
      return TRUE;

    case nsWindow::DESTROY:
      NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
      Destroy();
      break;

    case nsWindow::SET_FOCUS:
      NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
      SetFocus(PR_FALSE);
      break;

    default:
      bRet = FALSE;
      break;
  }

  return bRet;
}


void nsWindow::SetUpForPaint(HDC aHDC)
{
  ::SetBkColor (aHDC, NSRGB_2_COLOREF(mBackground));
  ::SetTextColor(aHDC, NSRGB_2_COLOREF(mForeground));
  ::SetBkMode (aHDC, TRANSPARENT);
}


NS_METHOD nsWindow::EnableDragDrop(PRBool aEnable)
{
  nsresult rv = NS_ERROR_FAILURE;
#ifndef WINCE
  if (aEnable) {
    if (nsnull == mNativeDragTarget) {
       mNativeDragTarget = new nsNativeDragTarget(this);
       if (NULL != mNativeDragTarget) {
         mNativeDragTarget->AddRef();
         if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget,TRUE,FALSE)) {
           if (S_OK == ::RegisterDragDrop(mWnd, (LPDROPTARGET)mNativeDragTarget)) {
             rv = NS_OK;
           }
         }
       }
    }
  } else {
    if (nsnull != mWnd && NULL != mNativeDragTarget) {
      ::RevokeDragDrop(mWnd);
      if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget, FALSE, TRUE)) {
        rv = NS_OK;
      }
      mNativeDragTarget->mDragCancelled = PR_TRUE;
      NS_RELEASE(mNativeDragTarget);
    }
  }
#endif
  return rv;
}


UINT nsWindow::MapFromNativeToDOM(UINT aNativeKeyCode)
{
#ifndef WINCE
  switch (aNativeKeyCode) {
    case VK_OEM_1:     return NS_VK_SEMICOLON;     
    case VK_OEM_PLUS:  return NS_VK_ADD;           
    case VK_OEM_MINUS: return NS_VK_SUBTRACT;      
  }
#endif

  return aNativeKeyCode;
}






PRBool nsWindow::DispatchKeyEvent(PRUint32 aEventType, WORD aCharCode,
                   const nsTArray<nsAlternativeCharCode>* aAlternativeCharCodes,
                   UINT aVirtualCharCode, const MSG *aMsg,
                   const nsModifierKeyState &aModKeyState,
                   PRUint32 aFlags)
{
  nsKeyEvent event(PR_TRUE, aEventType, this);
  nsIntPoint point(0, 0);

  InitEvent(event, &point); 

  event.flags |= aFlags;
  event.charCode = aCharCode;
  if (aAlternativeCharCodes)
    event.alternativeCharCodes.AppendElements(*aAlternativeCharCodes);
  event.keyCode  = aVirtualCharCode;

#ifdef KE_DEBUG
  static cnt=0;
  printf("%d DispatchKE Type: %s charCode %d  keyCode %d ", cnt++,
        (NS_KEY_PRESS == aEventType) ? "PRESS" : (aEventType == NS_KEY_UP ? "Up" : "Down"),
         event.charCode, event.keyCode);
  printf("Shift: %s Control %s Alt: %s \n", 
         (mIsShiftDown ? "D" : "U"), (mIsControlDown ? "D" : "U"), (mIsAltDown ? "D" : "U"));
  printf("[%c][%c][%c] <==   [%c][%c][%c][ space bar ][%c][%c][%c]\n",
         IS_VK_DOWN(NS_VK_SHIFT) ? 'S' : ' ',
         IS_VK_DOWN(NS_VK_CONTROL) ? 'C' : ' ',
         IS_VK_DOWN(NS_VK_ALT) ? 'A' : ' ',
         IS_VK_DOWN(VK_LSHIFT) ? 'S' : ' ',
         IS_VK_DOWN(VK_LCONTROL) ? 'C' : ' ',
         IS_VK_DOWN(VK_LMENU) ? 'A' : ' ',
         IS_VK_DOWN(VK_RMENU) ? 'A' : ' ',
         IS_VK_DOWN(VK_RCONTROL) ? 'C' : ' ',
         IS_VK_DOWN(VK_RSHIFT) ? 'S' : ' ');
#endif

  event.isShift   = aModKeyState.mIsShiftDown;
  event.isControl = aModKeyState.mIsControlDown;
  event.isMeta    = PR_FALSE;
  event.isAlt     = aModKeyState.mIsAltDown;

  nsPluginEvent pluginEvent;
  if (aMsg && PluginHasFocus()) {
    pluginEvent.event = aMsg->message;
    pluginEvent.wParam = aMsg->wParam;
    pluginEvent.lParam = aMsg->lParam;
    event.nativeMsg = (void *)&pluginEvent;
  }

  PRBool result = DispatchWindowEvent(&event);

  return result;
}

void nsWindow::RemoveMessageAndDispatchPluginEvent(UINT aFirstMsg,
                                                   UINT aLastMsg)
{
  MSG msg;
  ::GetMessageW(&msg, mWnd, aFirstMsg, aLastMsg);
  DispatchPluginEvent(msg);
}

static PRBool
StringCaseInsensitiveEquals(const PRUnichar* aChars1, const PRUint32 aNumChars1,
                            const PRUnichar* aChars2, const PRUint32 aNumChars2)
{
  if (aNumChars1 != aNumChars2)
    return PR_FALSE;

  nsCaseInsensitiveStringComparator comp;
  return comp(aChars1, aChars2, aNumChars1) == 0;
}









struct nsFakeCharMessage {
  UINT mCharCode;
  UINT mScanCode;
};





LRESULT nsWindow::OnKeyDown(const MSG &aMsg,
                            nsModifierKeyState &aModKeyState,
                            PRBool *aEventDispatched,
                            nsFakeCharMessage* aFakeCharMessage)
{
  UINT virtualKeyCode = aMsg.wParam;

#ifndef WINCE
  gKbdLayout.OnKeyDown (virtualKeyCode);
#endif

  
  
  UINT DOMKeyCode = nsIMM32Handler::IsComposing(this) ?
                      virtualKeyCode : MapFromNativeToDOM(virtualKeyCode);

#ifdef DEBUG
  
#endif

  PRBool noDefault =
    DispatchKeyEvent(NS_KEY_DOWN, 0, nsnull, DOMKeyCode, &aMsg, aModKeyState);
  if (aEventDispatched)
    *aEventDispatched = PR_TRUE;

  
  
  switch (DOMKeyCode) {
    case NS_VK_SHIFT:
    case NS_VK_CONTROL:
    case NS_VK_ALT:
    case NS_VK_CAPS_LOCK:
    case NS_VK_NUM_LOCK:
    case NS_VK_SCROLL_LOCK: return noDefault;
  }

  PRUint32 extraFlags = (noDefault ? NS_EVENT_FLAG_NO_DEFAULT : 0);
  MSG msg;
  BOOL gotMsg = aFakeCharMessage ||
    ::PeekMessageW(&msg, mWnd, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE | PM_NOYIELD);
  
  
  if (DOMKeyCode == NS_VK_RETURN || DOMKeyCode == NS_VK_BACK ||
      ((aModKeyState.mIsControlDown || aModKeyState.mIsAltDown)
#ifdef WINCE
       ))
#else
       && !gKbdLayout.IsDeadKey() && KeyboardLayout::IsPrintableCharKey(virtualKeyCode)))
#endif
  {
    
    
    
    
    PRBool anyCharMessagesRemoved = PR_FALSE;

    if (aFakeCharMessage) {
      anyCharMessagesRemoved = PR_TRUE;
    } else {
      while (gotMsg && (msg.message == WM_CHAR || msg.message == WM_SYSCHAR))
      {
        PR_LOG(sWindowsLog, PR_LOG_ALWAYS,
               ("%s charCode=%d scanCode=%d\n", msg.message == WM_SYSCHAR ? "WM_SYSCHAR" : "WM_CHAR",
                msg.wParam, HIWORD(msg.lParam) & 0xFF));
        RemoveMessageAndDispatchPluginEvent(WM_KEYFIRST, WM_KEYLAST);
        anyCharMessagesRemoved = PR_TRUE;

        gotMsg = ::PeekMessageW (&msg, mWnd, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE | PM_NOYIELD);
      }
    }

    if (!anyCharMessagesRemoved && DOMKeyCode == NS_VK_BACK &&
        nsIMM32Handler::IsDoingKakuteiUndo(mWnd)) {
      NS_ASSERTION(!aFakeCharMessage,
                   "We shouldn't be touching the real msg queue");
      RemoveMessageAndDispatchPluginEvent(WM_CHAR, WM_CHAR);
    }
  }
  else if (gotMsg &&
           (aFakeCharMessage ||
            msg.message == WM_CHAR || msg.message == WM_SYSCHAR || msg.message == WM_DEADCHAR)) {
    if (aFakeCharMessage)
      return OnCharRaw(aFakeCharMessage->mCharCode,
                       aFakeCharMessage->mScanCode, aModKeyState, extraFlags);

    
    ::GetMessageW(&msg, mWnd, msg.message, msg.message);

    if (msg.message == WM_DEADCHAR) {
      if (!PluginHasFocus())
        return PR_FALSE;

      
      DispatchPluginEvent(msg);
      return noDefault;
    }

    PR_LOG(sWindowsLog, PR_LOG_ALWAYS,
           ("%s charCode=%d scanCode=%d\n",
            msg.message == WM_SYSCHAR ? "WM_SYSCHAR" : "WM_CHAR",
            msg.wParam, HIWORD(msg.lParam) & 0xFF));

    BOOL result = OnChar(msg, aModKeyState, nsnull, extraFlags);
    
    
    if (!result && msg.message == WM_SYSCHAR)
      ::DefWindowProcW(mWnd, msg.message, msg.wParam, msg.lParam);
    return result;
  }
#ifndef WINCE
  else if (!aModKeyState.mIsControlDown && !aModKeyState.mIsAltDown &&
             (KeyboardLayout::IsPrintableCharKey(virtualKeyCode) ||
              KeyboardLayout::IsNumpadKey(virtualKeyCode)))
  {
    
    
    
    return PluginHasFocus() && noDefault;
  }

  if (gKbdLayout.IsDeadKey ())
    return PluginHasFocus() && noDefault;

  PRUint8 shiftStates[5];
  PRUnichar uniChars[5];
  PRUnichar shiftedChars[5] = {0, 0, 0, 0, 0};
  PRUnichar unshiftedChars[5] = {0, 0, 0, 0, 0};
  PRUnichar shiftedLatinChar = 0;
  PRUnichar unshiftedLatinChar = 0;
  PRUint32 numOfUniChars = 0;
  PRUint32 numOfShiftedChars = 0;
  PRUint32 numOfUnshiftedChars = 0;
  PRUint32 numOfShiftStates = 0;

  switch (virtualKeyCode) {
    
    case VK_ADD:       uniChars [0] = '+';  numOfUniChars = 1;  break;
    case VK_SUBTRACT:  uniChars [0] = '-';  numOfUniChars = 1;  break;
    case VK_DIVIDE:    uniChars [0] = '/';  numOfUniChars = 1;  break;
    case VK_MULTIPLY:  uniChars [0] = '*';  numOfUniChars = 1;  break;
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
      uniChars [0] = virtualKeyCode - VK_NUMPAD0 + '0';
      numOfUniChars = 1;
      break;
    default:
      if (KeyboardLayout::IsPrintableCharKey(virtualKeyCode)) {
        numOfUniChars = numOfShiftStates =
          gKbdLayout.GetUniChars(uniChars, shiftStates,
                                 NS_ARRAY_LENGTH(uniChars));
      }

      if (aModKeyState.mIsControlDown ^ aModKeyState.mIsAltDown) {
        PRUint8 capsLockState = (::GetKeyState(VK_CAPITAL) & 1) ? eCapsLock : 0;
        numOfUnshiftedChars =
          gKbdLayout.GetUniCharsWithShiftState(virtualKeyCode, capsLockState,
                       unshiftedChars, NS_ARRAY_LENGTH(unshiftedChars));
        numOfShiftedChars =
          gKbdLayout.GetUniCharsWithShiftState(virtualKeyCode,
                       capsLockState | eShift,
                       shiftedChars, NS_ARRAY_LENGTH(shiftedChars));

        
        
        
        if (NS_VK_A <= DOMKeyCode && DOMKeyCode <= NS_VK_Z) {
          shiftedLatinChar = unshiftedLatinChar = DOMKeyCode;
          if (capsLockState)
            shiftedLatinChar += 0x20;
          else
            unshiftedLatinChar += 0x20;
          if (unshiftedLatinChar == unshiftedChars[0] &&
              shiftedLatinChar == shiftedChars[0]) {
              shiftedLatinChar = unshiftedLatinChar = 0;
          }
        } else {
          PRUint16 ch = 0;
          if (NS_VK_0 <= DOMKeyCode && DOMKeyCode <= NS_VK_9) {
            ch = DOMKeyCode;
          } else {
            switch (virtualKeyCode) {
              case VK_OEM_PLUS:   ch = '+'; break;
              case VK_OEM_MINUS:  ch = '-'; break;
            }
          }
          if (ch && unshiftedChars[0] != ch && shiftedChars[0] != ch) {
            
            
            
            
            
            unshiftedLatinChar = ch;
          }
        }

        
        
        
        
        
        if (aModKeyState.mIsControlDown) {
          PRUint8 currentState = eCtrl;
          if (aModKeyState.mIsShiftDown)
            currentState |= eShift;

          PRUint32 ch =
            aModKeyState.mIsShiftDown ? shiftedLatinChar : unshiftedLatinChar;
          if (ch &&
              (numOfUniChars == 0 ||
               StringCaseInsensitiveEquals(uniChars, numOfUniChars,
                 aModKeyState.mIsShiftDown ? shiftedChars : unshiftedChars,
                 aModKeyState.mIsShiftDown ? numOfShiftedChars :
                                             numOfUnshiftedChars))) {
            numOfUniChars = numOfShiftStates = 1;
            uniChars[0] = ch;
            shiftStates[0] = currentState;
          }
        }
      }
  }

  if (numOfUniChars > 0 || numOfShiftedChars > 0 || numOfUnshiftedChars > 0) {
    PRUint32 num = PR_MAX(numOfUniChars,
                          PR_MAX(numOfShiftedChars, numOfUnshiftedChars));
    PRUint32 skipUniChars = num - numOfUniChars;
    PRUint32 skipShiftedChars = num - numOfShiftedChars;
    PRUint32 skipUnshiftedChars = num - numOfUnshiftedChars;
    UINT keyCode = numOfUniChars == 0 ? DOMKeyCode : 0;
    for (PRUint32 cnt = 0; cnt < num; cnt++) {
      PRUint16 uniChar, shiftedChar, unshiftedChar;
      uniChar = shiftedChar = unshiftedChar = 0;
      if (skipUniChars <= cnt) {
        if (cnt - skipUniChars  < numOfShiftStates) {
          
          
          
          
          
          
          aModKeyState.mIsShiftDown =
            (shiftStates[cnt - skipUniChars] & eShift) != 0;
          aModKeyState.mIsControlDown =
            (shiftStates[cnt - skipUniChars] & eCtrl) != 0;
          aModKeyState.mIsAltDown =
            (shiftStates[cnt - skipUniChars] & eAlt) != 0;
        }
        uniChar = uniChars[cnt - skipUniChars];
      }
      if (skipShiftedChars <= cnt)
        shiftedChar = shiftedChars[cnt - skipShiftedChars];
      if (skipUnshiftedChars <= cnt)
        unshiftedChar = unshiftedChars[cnt - skipUnshiftedChars];
      nsAutoTArray<nsAlternativeCharCode, 5> altArray;

      if (shiftedChar || unshiftedChar) {
        nsAlternativeCharCode chars(unshiftedChar, shiftedChar);
        altArray.AppendElement(chars);
      }
      if (cnt == num - 1 && (unshiftedLatinChar || shiftedLatinChar)) {
        nsAlternativeCharCode chars(unshiftedLatinChar, shiftedLatinChar);
        altArray.AppendElement(chars);
      }

      DispatchKeyEvent(NS_KEY_PRESS, uniChar, &altArray,
                       keyCode, nsnull, aModKeyState, extraFlags);
    }
  } else
#endif
    DispatchKeyEvent(NS_KEY_PRESS, 0, nsnull, DOMKeyCode, nsnull, aModKeyState,
                     extraFlags);

  return noDefault;
}





LRESULT nsWindow::OnKeyUp(const MSG &aMsg,
                          nsModifierKeyState &aModKeyState,
                          PRBool *aEventDispatched)
{
  UINT virtualKeyCode = aMsg.wParam;

  PR_LOG(sWindowsLog, PR_LOG_ALWAYS,
         ("nsWindow::OnKeyUp VK=%d\n", virtualKeyCode));

  if (!nsIMM32Handler::IsComposing(this)) {
    virtualKeyCode = MapFromNativeToDOM(virtualKeyCode);
  }

  if (aEventDispatched)
    *aEventDispatched = PR_TRUE;
  return DispatchKeyEvent(NS_KEY_UP, 0, nsnull, virtualKeyCode, &aMsg,
                          aModKeyState);
}





LRESULT nsWindow::OnChar(const MSG &aMsg, nsModifierKeyState &aModKeyState,
                         PRBool *aEventDispatched, PRUint32 aFlags)
{
  return OnCharRaw(aMsg.wParam, HIWORD(aMsg.lParam) & 0xFF, aModKeyState,
                   aFlags, &aMsg, aEventDispatched);
}





LRESULT nsWindow::OnCharRaw(UINT charCode, UINT aScanCode,
                            nsModifierKeyState &aModKeyState, PRUint32 aFlags,
                            const MSG *aMsg, PRBool *aEventDispatched)
{
  
  if (aModKeyState.mIsAltDown && !aModKeyState.mIsControlDown &&
      IS_VK_DOWN(NS_VK_SPACE)) {
    return FALSE;
  }
  
  
  if (aModKeyState.mIsControlDown && charCode == 0xA) {
    return FALSE;
  }

  
  PRBool saveIsAltDown = aModKeyState.mIsAltDown;
  PRBool saveIsControlDown = aModKeyState.mIsControlDown;
  if (aModKeyState.mIsAltDown && aModKeyState.mIsControlDown)
    aModKeyState.mIsAltDown = aModKeyState.mIsControlDown = PR_FALSE;

  wchar_t uniChar;

  if (nsIMM32Handler::IsComposing(this)) {
    ResetInputState();
  }

  if (aModKeyState.mIsControlDown && charCode <= 0x1A) { 
    
    if (aModKeyState.mIsShiftDown)
      uniChar = charCode - 1 + 'A';
    else
      uniChar = charCode - 1 + 'a';
    charCode = 0;
  }
  else if (aModKeyState.mIsControlDown && charCode <= 0x1F) {
    
    
    
    
    uniChar = charCode - 1 + 'A';
    charCode = 0;
  } else { 
    if (charCode < 0x20 || (charCode == 0x3D && aModKeyState.mIsControlDown)) {
      uniChar = 0;
    } else {
      uniChar = charCode;
      charCode = 0;
    }
  }

  
  
  if (uniChar && (aModKeyState.mIsControlDown || aModKeyState.mIsAltDown)) {
    UINT virtualKeyCode = ::MapVirtualKeyEx(aScanCode, MAPVK_VSC_TO_VK,
                                            gKbdLayout.GetLayout());
    UINT unshiftedCharCode =
      virtualKeyCode >= '0' && virtualKeyCode <= '9' ? virtualKeyCode :
        aModKeyState.mIsShiftDown ? ::MapVirtualKeyEx(virtualKeyCode,
                                        MAPVK_VK_TO_CHAR,
                                        gKbdLayout.GetLayout()) : 0;
    
    if ((INT)unshiftedCharCode > 0)
      uniChar = unshiftedCharCode;
  }

  
  
  
  if (!aModKeyState.mIsShiftDown && (saveIsAltDown || saveIsControlDown)) {
    uniChar = towlower(uniChar);
  }

  PRBool result = DispatchKeyEvent(NS_KEY_PRESS, uniChar, nsnull,
                                   charCode, aMsg, aModKeyState, aFlags);
  if (aEventDispatched)
    *aEventDispatched = PR_TRUE;
  aModKeyState.mIsAltDown = saveIsAltDown;
  aModKeyState.mIsControlDown = saveIsControlDown;
  return result;
}

static const PRUint32 sModifierKeyMap[][3] = {
  { nsIWidget::CAPS_LOCK, VK_CAPITAL, 0 },
  { nsIWidget::NUM_LOCK, VK_NUMLOCK, 0 },
  { nsIWidget::SHIFT_L, VK_SHIFT, VK_LSHIFT },
  { nsIWidget::SHIFT_R, VK_SHIFT, VK_RSHIFT },
  { nsIWidget::CTRL_L, VK_CONTROL, VK_LCONTROL },
  { nsIWidget::CTRL_R, VK_CONTROL, VK_RCONTROL },
  { nsIWidget::ALT_L, VK_MENU, VK_LMENU },
  { nsIWidget::ALT_R, VK_MENU, VK_RMENU }
};

struct KeyPair {
  PRUint8 mGeneral;
  PRUint8 mSpecific;
  KeyPair(PRUint32 aGeneral, PRUint32 aSpecific)
    : mGeneral(PRUint8(aGeneral)), mSpecific(PRUint8(aSpecific)) {}
};

static void
SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray, PRUint32 aModifiers)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sModifierKeyMap); ++i) {
    const PRUint32* map = sModifierKeyMap[i];
    if (aModifiers & map[0]) {
      aArray->AppendElement(KeyPair(map[1], map[2]));
    }
  }
}

nsresult
nsWindow::SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                   PRInt32 aNativeKeyCode,
                                   PRUint32 aModifierFlags,
                                   const nsAString& aCharacters,
                                   const nsAString& aUnmodifiedCharacters)
{
#ifndef WINCE  
  nsPrintfCString layoutName("%08x", aNativeKeyboardLayout);
  HKL loadedLayout = LoadKeyboardLayoutA(layoutName.get(), KLF_NOTELLSHELL);
  if (loadedLayout == NULL)
    return NS_ERROR_NOT_AVAILABLE;

  
  BYTE originalKbdState[256];
  ::GetKeyboardState(originalKbdState);
  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));
  
  
  ::SetKeyboardState(kbdState);
  HKL oldLayout = gKbdLayout.GetLayout();
  gKbdLayout.LoadLayout(loadedLayout);

  nsAutoTArray<KeyPair,10> keySequence;
  SetupKeyModifiersSequence(&keySequence, aModifierFlags);
  NS_ASSERTION(aNativeKeyCode >= 0 && aNativeKeyCode < 256,
               "Native VK key code out of range");
  keySequence.AppendElement(KeyPair(aNativeKeyCode, 0));

  
  for (PRUint32 i = 0; i < keySequence.Length(); ++i) {
    PRUint8 key = keySequence[i].mGeneral;
    PRUint8 keySpecific = keySequence[i].mSpecific;
    kbdState[key] = 0x81; 
    if (keySpecific) {
      kbdState[keySpecific] = 0x81;
    }
    ::SetKeyboardState(kbdState);
    nsModifierKeyState modKeyState;
    MSG msg = InitMSG(WM_KEYDOWN, key, 0);
    if (i == keySequence.Length() - 1 && aCharacters.Length() > 0) {
      UINT scanCode = ::MapVirtualKeyEx(aNativeKeyCode, MAPVK_VK_TO_VSC,
                                        gKbdLayout.GetLayout());
      nsFakeCharMessage fakeMsg = { aCharacters.CharAt(0), scanCode };
      OnKeyDown(msg, modKeyState, nsnull, &fakeMsg);
    } else {
      OnKeyDown(msg, modKeyState, nsnull, nsnull);
    }
  }
  for (PRUint32 i = keySequence.Length(); i > 0; --i) {
    PRUint8 key = keySequence[i - 1].mGeneral;
    PRUint8 keySpecific = keySequence[i - 1].mSpecific;
    kbdState[key] = 0; 
    if (keySpecific) {
      kbdState[keySpecific] = 0;
    }
    ::SetKeyboardState(kbdState);
    nsModifierKeyState modKeyState;
    MSG msg = InitMSG(WM_KEYUP, key, 0);
    OnKeyUp(msg, modKeyState, nsnull);
  }

  
  ::SetKeyboardState(originalKbdState);
  gKbdLayout.LoadLayout(oldLayout);

  UnloadKeyboardLayout(loadedLayout);
  return NS_OK;
#else  
  return NS_ERROR_NOT_IMPLEMENTED;
#endif  
}

void nsWindow::ConstrainZLevel(HWND *aAfter)
{
  nsZLevelEvent  event(PR_TRUE, NS_SETZLEVEL, this);
  nsWindow      *aboveWindow = 0;

  InitEvent(event);

  if (*aAfter == HWND_BOTTOM)
    event.mPlacement = nsWindowZBottom;
  else if (*aAfter == HWND_TOP || *aAfter == HWND_TOPMOST || *aAfter == HWND_NOTOPMOST)
    event.mPlacement = nsWindowZTop;
  else {
    event.mPlacement = nsWindowZRelative;
    aboveWindow = GetNSWindowPtr(*aAfter);
  }
  event.mReqBelow = aboveWindow;
  event.mActualBelow = nsnull;

  event.mImmediate = PR_FALSE;
  event.mAdjusted = PR_FALSE;
  DispatchWindowEvent(&event);

  if (event.mAdjusted) {
    if (event.mPlacement == nsWindowZBottom)
      *aAfter = HWND_BOTTOM;
    else if (event.mPlacement == nsWindowZTop)
      *aAfter = HWND_TOP;
    else {
      *aAfter = (HWND)event.mActualBelow->GetNativeData(NS_NATIVE_WINDOW);
    }
  }
  NS_IF_RELEASE(event.mActualBelow);
}






static PRBool gJustGotDeactivate = PR_FALSE;
static PRBool gJustGotActivate = PR_FALSE;

#ifdef NS_DEBUG

typedef struct {
  char * mStr;
  long   mId;
} EventMsgInfo;

EventMsgInfo gAllEvents[] = {
  {"WM_NULL",                             0x0000},
  {"WM_CREATE",                           0x0001},
  {"WM_DESTROY",                          0x0002},
  {"WM_MOVE",                             0x0003},
  {"WM_SIZE",                             0x0005},
  {"WM_ACTIVATE",                         0x0006},
  {"WM_SETFOCUS",                         0x0007},
  {"WM_KILLFOCUS",                        0x0008},
  {"WM_ENABLE",                           0x000A},
  {"WM_SETREDRAW",                        0x000B},
  {"WM_SETTEXT",                          0x000C},
  {"WM_GETTEXT",                          0x000D},
  {"WM_GETTEXTLENGTH",                    0x000E},
  {"WM_PAINT",                            0x000F},
  {"WM_CLOSE",                            0x0010},
  {"WM_QUERYENDSESSION",                  0x0011},
  {"WM_QUIT",                             0x0012},
  {"WM_QUERYOPEN",                        0x0013},
  {"WM_ERASEBKGND",                       0x0014},
  {"WM_SYSCOLORCHANGE",                   0x0015},
  {"WM_ENDSESSION",                       0x0016},
  {"WM_SHOWWINDOW",                       0x0018},
  {"WM_SETTINGCHANGE",                    0x001A},
  {"WM_DEVMODECHANGE",                    0x001B},
  {"WM_ACTIVATEAPP",                      0x001C},
  {"WM_FONTCHANGE",                       0x001D},
  {"WM_TIMECHANGE",                       0x001E},
  {"WM_CANCELMODE",                       0x001F},
  {"WM_SETCURSOR",                        0x0020},
  {"WM_MOUSEACTIVATE",                    0x0021},
  {"WM_CHILDACTIVATE",                    0x0022},
  {"WM_QUEUESYNC",                        0x0023},
  {"WM_GETMINMAXINFO",                    0x0024},
  {"WM_PAINTICON",                        0x0026},
  {"WM_ICONERASEBKGND",                   0x0027},
  {"WM_NEXTDLGCTL",                       0x0028},
  {"WM_SPOOLERSTATUS",                    0x002A},
  {"WM_DRAWITEM",                         0x002B},
  {"WM_MEASUREITEM",                      0x002C},
  {"WM_DELETEITEM",                       0x002D},
  {"WM_VKEYTOITEM",                       0x002E},
  {"WM_CHARTOITEM",                       0x002F},
  {"WM_SETFONT",                          0x0030},
  {"WM_GETFONT",                          0x0031},
  {"WM_SETHOTKEY",                        0x0032},
  {"WM_GETHOTKEY",                        0x0033},
  {"WM_QUERYDRAGICON",                    0x0037},
  {"WM_COMPAREITEM",                      0x0039},
  {"WM_GETOBJECT",                        0x003D},
  {"WM_COMPACTING",                       0x0041},
  {"WM_COMMNOTIFY",                       0x0044},
  {"WM_WINDOWPOSCHANGING",                0x0046},
  {"WM_WINDOWPOSCHANGED",                 0x0047},
  {"WM_POWER",                            0x0048},
  {"WM_COPYDATA",                         0x004A},
  {"WM_CANCELJOURNAL",                    0x004B},
  {"WM_NOTIFY",                           0x004E},
  {"WM_INPUTLANGCHANGEREQUEST",           0x0050},
  {"WM_INPUTLANGCHANGE",                  0x0051},
  {"WM_TCARD",                            0x0052},
  {"WM_HELP",                             0x0053},
  {"WM_USERCHANGED",                      0x0054},
  {"WM_NOTIFYFORMAT",                     0x0055},
  {"WM_CONTEXTMENU",                      0x007B},
  {"WM_STYLECHANGING",                    0x007C},
  {"WM_STYLECHANGED",                     0x007D},
  {"WM_DISPLAYCHANGE",                    0x007E},
  {"WM_GETICON",                          0x007F},
  {"WM_SETICON",                          0x0080},
  {"WM_NCCREATE",                         0x0081},
  {"WM_NCDESTROY",                        0x0082},
  {"WM_NCCALCSIZE",                       0x0083},
  {"WM_NCHITTEST",                        0x0084},
  {"WM_NCPAINT",                          0x0085},
  {"WM_NCACTIVATE",                       0x0086},
  {"WM_GETDLGCODE",                       0x0087},
  {"WM_SYNCPAINT",                        0x0088},
  {"WM_NCMOUSEMOVE",                      0x00A0},
  {"WM_NCLBUTTONDOWN",                    0x00A1},
  {"WM_NCLBUTTONUP",                      0x00A2},
  {"WM_NCLBUTTONDBLCLK",                  0x00A3},
  {"WM_NCRBUTTONDOWN",                    0x00A4},
  {"WM_NCRBUTTONUP",                      0x00A5},
  {"WM_NCRBUTTONDBLCLK",                  0x00A6},
  {"WM_NCMBUTTONDOWN",                    0x00A7},
  {"WM_NCMBUTTONUP",                      0x00A8},
  {"WM_NCMBUTTONDBLCLK",                  0x00A9},
  {"EM_GETSEL",                           0x00B0},
  {"EM_SETSEL",                           0x00B1},
  {"EM_GETRECT",                          0x00B2},
  {"EM_SETRECT",                          0x00B3},
  {"EM_SETRECTNP",                        0x00B4},
  {"EM_SCROLL",                           0x00B5},
  {"EM_LINESCROLL",                       0x00B6},
  {"EM_SCROLLCARET",                      0x00B7},
  {"EM_GETMODIFY",                        0x00B8},
  {"EM_SETMODIFY",                        0x00B9},
  {"EM_GETLINECOUNT",                     0x00BA},
  {"EM_LINEINDEX",                        0x00BB},
  {"EM_SETHANDLE",                        0x00BC},
  {"EM_GETHANDLE",                        0x00BD},
  {"EM_GETTHUMB",                         0x00BE},
  {"EM_LINELENGTH",                       0x00C1},
  {"EM_REPLACESEL",                       0x00C2},
  {"EM_GETLINE",                          0x00C4},
  {"EM_LIMITTEXT",                        0x00C5},
  {"EM_CANUNDO",                          0x00C6},
  {"EM_UNDO",                             0x00C7},
  {"EM_FMTLINES",                         0x00C8},
  {"EM_LINEFROMCHAR",                     0x00C9},
  {"EM_SETTABSTOPS",                      0x00CB},
  {"EM_SETPASSWORDCHAR",                  0x00CC},
  {"EM_EMPTYUNDOBUFFER",                  0x00CD},
  {"EM_GETFIRSTVISIBLELINE",              0x00CE},
  {"EM_SETREADONLY",                      0x00CF},
  {"EM_SETWORDBREAKPROC",                 0x00D0},
  {"EM_GETWORDBREAKPROC",                 0x00D1},
  {"EM_GETPASSWORDCHAR",                  0x00D2},
  {"EM_SETMARGINS",                       0x00D3},
  {"EM_GETMARGINS",                       0x00D4},
  {"EM_GETLIMITTEXT",                     0x00D5},
  {"EM_POSFROMCHAR",                      0x00D6},
  {"EM_CHARFROMPOS",                      0x00D7},
  {"EM_SETIMESTATUS",                     0x00D8},
  {"EM_GETIMESTATUS",                     0x00D9},
  {"SBM_SETPOS",                          0x00E0},
  {"SBM_GETPOS",                          0x00E1},
  {"SBM_SETRANGE",                        0x00E2},
  {"SBM_SETRANGEREDRAW",                  0x00E6},
  {"SBM_GETRANGE",                        0x00E3},
  {"SBM_ENABLE_ARROWS",                   0x00E4},
  {"SBM_SETSCROLLINFO",                   0x00E9},
  {"SBM_GETSCROLLINFO",                   0x00EA},
  {"WM_KEYDOWN",                          0x0100},
  {"WM_KEYUP",                            0x0101},
  {"WM_CHAR",                             0x0102},
  {"WM_DEADCHAR",                         0x0103},
  {"WM_SYSKEYDOWN",                       0x0104},
  {"WM_SYSKEYUP",                         0x0105},
  {"WM_SYSCHAR",                          0x0106},
  {"WM_SYSDEADCHAR",                      0x0107},
  {"WM_KEYLAST",                          0x0108},
  {"WM_IME_STARTCOMPOSITION",             0x010D},
  {"WM_IME_ENDCOMPOSITION",               0x010E},
  {"WM_IME_COMPOSITION",                  0x010F},
  {"WM_INITDIALOG",                       0x0110},
  {"WM_COMMAND",                          0x0111},
  {"WM_SYSCOMMAND",                       0x0112},
  {"WM_TIMER",                            0x0113},
  {"WM_HSCROLL",                          0x0114},
  {"WM_VSCROLL",                          0x0115},
  {"WM_INITMENU",                         0x0116},
  {"WM_INITMENUPOPUP",                    0x0117},
  {"WM_MENUSELECT",                       0x011F},
  {"WM_MENUCHAR",                         0x0120},
  {"WM_ENTERIDLE",                        0x0121},
  {"WM_MENURBUTTONUP",                    0x0122},
  {"WM_MENUDRAG",                         0x0123},
  {"WM_MENUGETOBJECT",                    0x0124},
  {"WM_UNINITMENUPOPUP",                  0x0125},
  {"WM_MENUCOMMAND",                      0x0126},
  {"WM_CTLCOLORMSGBOX",                   0x0132},
  {"WM_CTLCOLOREDIT",                     0x0133},
  {"WM_CTLCOLORLISTBOX",                  0x0134},
  {"WM_CTLCOLORBTN",                      0x0135},
  {"WM_CTLCOLORDLG",                      0x0136},
  {"WM_CTLCOLORSCROLLBAR",                0x0137},
  {"WM_CTLCOLORSTATIC",                   0x0138},
  {"CB_GETEDITSEL",                       0x0140},
  {"CB_LIMITTEXT",                        0x0141},
  {"CB_SETEDITSEL",                       0x0142},
  {"CB_ADDSTRING",                        0x0143},
  {"CB_DELETESTRING",                     0x0144},
  {"CB_DIR",                              0x0145},
  {"CB_GETCOUNT",                         0x0146},
  {"CB_GETCURSEL",                        0x0147},
  {"CB_GETLBTEXT",                        0x0148},
  {"CB_GETLBTEXTLEN",                     0x0149},
  {"CB_INSERTSTRING",                     0x014A},
  {"CB_RESETCONTENT",                     0x014B},
  {"CB_FINDSTRING",                       0x014C},
  {"CB_SELECTSTRING",                     0x014D},
  {"CB_SETCURSEL",                        0x014E},
  {"CB_SHOWDROPDOWN",                     0x014F},
  {"CB_GETITEMDATA",                      0x0150},
  {"CB_SETITEMDATA",                      0x0151},
  {"CB_GETDROPPEDCONTROLRECT",            0x0152},
  {"CB_SETITEMHEIGHT",                    0x0153},
  {"CB_GETITEMHEIGHT",                    0x0154},
  {"CB_SETEXTENDEDUI",                    0x0155},
  {"CB_GETEXTENDEDUI",                    0x0156},
  {"CB_GETDROPPEDSTATE",                  0x0157},
  {"CB_FINDSTRINGEXACT",                  0x0158},
  {"CB_SETLOCALE",                        0x0159},
  {"CB_GETLOCALE",                        0x015A},
  {"CB_GETTOPINDEX",                      0x015b},
  {"CB_SETTOPINDEX",                      0x015c},
  {"CB_GETHORIZONTALEXTENT",              0x015d},
  {"CB_SETHORIZONTALEXTENT",              0x015e},
  {"CB_GETDROPPEDWIDTH",                  0x015f},
  {"CB_SETDROPPEDWIDTH",                  0x0160},
  {"CB_INITSTORAGE",                      0x0161},
  {"CB_MSGMAX",                           0x0162},
  {"LB_ADDSTRING",                        0x0180},
  {"LB_INSERTSTRING",                     0x0181},
  {"LB_DELETESTRING",                     0x0182},
  {"LB_SELITEMRANGEEX",                   0x0183},
  {"LB_RESETCONTENT",                     0x0184},
  {"LB_SETSEL",                           0x0185},
  {"LB_SETCURSEL",                        0x0186},
  {"LB_GETSEL",                           0x0187},
  {"LB_GETCURSEL",                        0x0188},
  {"LB_GETTEXT",                          0x0189},
  {"LB_GETTEXTLEN",                       0x018A},
  {"LB_GETCOUNT",                         0x018B},
  {"LB_SELECTSTRING",                     0x018C},
  {"LB_DIR",                              0x018D},
  {"LB_GETTOPINDEX",                      0x018E},
  {"LB_FINDSTRING",                       0x018F},
  {"LB_GETSELCOUNT",                      0x0190},
  {"LB_GETSELITEMS",                      0x0191},
  {"LB_SETTABSTOPS",                      0x0192},
  {"LB_GETHORIZONTALEXTENT",              0x0193},
  {"LB_SETHORIZONTALEXTENT",              0x0194},
  {"LB_SETCOLUMNWIDTH",                   0x0195},
  {"LB_ADDFILE",                          0x0196},
  {"LB_SETTOPINDEX",                      0x0197},
  {"LB_GETITEMRECT",                      0x0198},
  {"LB_GETITEMDATA",                      0x0199},
  {"LB_SETITEMDATA",                      0x019A},
  {"LB_SELITEMRANGE",                     0x019B},
  {"LB_SETANCHORINDEX",                   0x019C},
  {"LB_GETANCHORINDEX",                   0x019D},
  {"LB_SETCARETINDEX",                    0x019E},
  {"LB_GETCARETINDEX",                    0x019F},
  {"LB_SETITEMHEIGHT",                    0x01A0},
  {"LB_GETITEMHEIGHT",                    0x01A1},
  {"LB_FINDSTRINGEXACT",                  0x01A2},
  {"LB_SETLOCALE",                        0x01A5},
  {"LB_GETLOCALE",                        0x01A6},
  {"LB_SETCOUNT",                         0x01A7},
  {"LB_INITSTORAGE",                      0x01A8},
  {"LB_ITEMFROMPOINT",                    0x01A9},
  {"LB_MSGMAX",                           0x01B0},
  {"WM_MOUSEMOVE",                        0x0200},
  {"WM_LBUTTONDOWN",                      0x0201},
  {"WM_LBUTTONUP",                        0x0202},
  {"WM_LBUTTONDBLCLK",                    0x0203},
  {"WM_RBUTTONDOWN",                      0x0204},
  {"WM_RBUTTONUP",                        0x0205},
  {"WM_RBUTTONDBLCLK",                    0x0206},
  {"WM_MBUTTONDOWN",                      0x0207},
  {"WM_MBUTTONUP",                        0x0208},
  {"WM_MBUTTONDBLCLK",                    0x0209},
  {"WM_MOUSEWHEEL",                       0x020A},
  {"WM_MOUSEHWHEEL",                      0x020E},
  {"WM_PARENTNOTIFY",                     0x0210},
  {"WM_ENTERMENULOOP",                    0x0211},
  {"WM_EXITMENULOOP",                     0x0212},
  {"WM_NEXTMENU",                         0x0213},
  {"WM_SIZING",                           0x0214},
  {"WM_CAPTURECHANGED",                   0x0215},
  {"WM_MOVING",                           0x0216},
  {"WM_POWERBROADCAST",                   0x0218},
  {"WM_DEVICECHANGE",                     0x0219},
  {"WM_MDICREATE",                        0x0220},
  {"WM_MDIDESTROY",                       0x0221},
  {"WM_MDIACTIVATE",                      0x0222},
  {"WM_MDIRESTORE",                       0x0223},
  {"WM_MDINEXT",                          0x0224},
  {"WM_MDIMAXIMIZE",                      0x0225},
  {"WM_MDITILE",                          0x0226},
  {"WM_MDICASCADE",                       0x0227},
  {"WM_MDIICONARRANGE",                   0x0228},
  {"WM_MDIGETACTIVE",                     0x0229},
  {"WM_MDISETMENU",                       0x0230},
  {"WM_ENTERSIZEMOVE",                    0x0231},
  {"WM_EXITSIZEMOVE",                     0x0232},
  {"WM_DROPFILES",                        0x0233},
  {"WM_MDIREFRESHMENU",                   0x0234},
  {"WM_IME_SETCONTEXT",                   0x0281},
  {"WM_IME_NOTIFY",                       0x0282},
  {"WM_IME_CONTROL",                      0x0283},
  {"WM_IME_COMPOSITIONFULL",              0x0284},
  {"WM_IME_SELECT",                       0x0285},
  {"WM_IME_CHAR",                         0x0286},
  {"WM_IME_REQUEST",                      0x0288},
  {"WM_IME_KEYDOWN",                      0x0290},
  {"WM_IME_KEYUP",                        0x0291},
  {"WM_NCMOUSEHOVER",                     0x02A0},
  {"WM_MOUSEHOVER",                       0x02A1},
  {"WM_MOUSELEAVE",                       0x02A3},
  {"WM_CUT",                              0x0300},
  {"WM_COPY",                             0x0301},
  {"WM_PASTE",                            0x0302},
  {"WM_CLEAR",                            0x0303},
  {"WM_UNDO",                             0x0304},
  {"WM_RENDERFORMAT",                     0x0305},
  {"WM_RENDERALLFORMATS",                 0x0306},
  {"WM_DESTROYCLIPBOARD",                 0x0307},
  {"WM_DRAWCLIPBOARD",                    0x0308},
  {"WM_PAINTCLIPBOARD",                   0x0309},
  {"WM_VSCROLLCLIPBOARD",                 0x030A},
  {"WM_SIZECLIPBOARD",                    0x030B},
  {"WM_ASKCBFORMATNAME",                  0x030C},
  {"WM_CHANGECBCHAIN",                    0x030D},
  {"WM_HSCROLLCLIPBOARD",                 0x030E},
  {"WM_QUERYNEWPALETTE",                  0x030F},
  {"WM_PALETTEISCHANGING",                0x0310},
  {"WM_PALETTECHANGED",                   0x0311},
  {"WM_HOTKEY",                           0x0312},
  {"WM_PRINT",                            0x0317},
  {"WM_PRINTCLIENT",                      0x0318},
  {"WM_THEMECHANGED",                     0x031A},
  {"WM_HANDHELDFIRST",                    0x0358},
  {"WM_HANDHELDLAST",                     0x035F},
  {"WM_AFXFIRST",                         0x0360},
  {"WM_AFXLAST",                          0x037F},
  {"WM_PENWINFIRST",                      0x0380},
  {"WM_PENWINLAST",                       0x038F},
  {"WM_APP",                              0x8000},
  {"WM_DWMCOMPOSITIONCHANGED",            0x031E},
  {"WM_DWMNCRENDERINGCHANGED",            0x031F},
  {"WM_DWMCOLORIZATIONCOLORCHANGED",      0x0320},
  {"WM_DWMWINDOWMAXIMIZEDCHANGE",         0x0321},
  {"WM_TABLET_QUERYSYSTEMGESTURESTATUS",  0x02CC},
  {"WM_GESTURE",                          0x0119},
  {"WM_GESTURENOTIFY",                    0x011A},
  {NULL, 0x0}
};


static long gEventCounter = 0;
static long gLastEventMsg = 0;

void PrintEvent(UINT msg, PRBool aShowAllEvents, PRBool aShowMouseMoves)
{
  int inx = 0;
  while (gAllEvents[inx].mId != (long)msg && gAllEvents[inx].mStr != NULL) {
    inx++;
  }
  if (aShowAllEvents || (!aShowAllEvents && gLastEventMsg != (long)msg)) {
    if (aShowMouseMoves || (!aShowMouseMoves && msg != 0x0020 && msg != 0x0200 && msg != 0x0084)) {
      printf("%6d - 0x%04X %s\n", gEventCounter++, msg, gAllEvents[inx].mStr ? gAllEvents[inx].mStr : "Unknown");
      gLastEventMsg = msg;
    }
  }
}

#endif

#define WM_XP_THEMECHANGED                 0x031A


static nsresult HeapDump(const char *filename, const char *heading)
{
#ifdef WINCE
  return NS_ERROR_NOT_IMPLEMENTED;
#else

  PRFileDesc *prfd = PR_Open(filename, PR_CREATE_FILE | PR_APPEND | PR_WRONLY, 0777);
  if (!prfd)
    return NS_ERROR_FAILURE;

  char buf[1024];
  PRUint32 n;
  PRUint32 written = 0;
  HANDLE heapHandle[64];
  DWORD nheap = GetProcessHeaps(64, heapHandle);
  if (nheap == 0 || nheap > 64) {
    return NS_ERROR_FAILURE;
  }

  n = PR_snprintf(buf, sizeof buf, "BEGIN HEAPDUMP : %s\n", heading);
  PR_Write(prfd, buf, n);
  for (DWORD i = 0; i < nheap; i++) {
    
    PROCESS_HEAP_ENTRY ent = {0};
    n = PR_snprintf(buf, sizeof buf, "BEGIN heap %d : 0x%p\n", i+1, heapHandle[i]);
    PR_Write(prfd, buf, n);
    ent.lpData = NULL;
    while (HeapWalk(heapHandle[i], &ent)) {
      if (ent.wFlags & PROCESS_HEAP_REGION)
        n = PR_snprintf(buf, sizeof buf, "REGION %08p : overhead %d committed %d uncommitted %d firstblock %08p lastblock %08p\n",
                        ent.lpData, ent.cbOverhead,
                        ent.Region.dwCommittedSize, ent.Region.dwUnCommittedSize,
                        ent.Region.lpFirstBlock, ent.Region.lpLastBlock);
      else
        n = PR_snprintf(buf, sizeof buf, "%s %08p : %6d overhead %2d\n",
                        (ent.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) ? "----" : ((ent.wFlags & PROCESS_HEAP_ENTRY_BUSY) ? "USED" : "FREE"),
                        ent.lpData, ent.cbData, ent.cbOverhead);
      PR_Write(prfd, buf, n);
    }
    n = PR_snprintf(buf, sizeof buf, "END heap %d : 0x%p\n", i+1, heapHandle[i]);
    PR_Write(prfd, buf, n);
  }
  n = PR_snprintf(buf, sizeof buf, "END HEAPDUMP : %s\n", heading);
  PR_Write(prfd, buf, n);

  PR_Close(prfd);
  return NS_OK;
#endif 
}




BOOL CALLBACK nsWindow::DispatchStarvedPaints(HWND aWnd, LPARAM aMsg)
{
  LONG_PTR proc = ::GetWindowLongPtrW(aWnd, GWLP_WNDPROC);
  if (proc == (LONG_PTR)&nsWindow::WindowProc) {
    
    
    
    if (GetUpdateRect(aWnd, NULL, FALSE)) {
      VERIFY(::UpdateWindow(aWnd));
    }
  }
  return TRUE;
}








void nsWindow::DispatchPendingEvents()
{
  gLastInputEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  
  
  
  static int recursionBlocker = 0;
  if (recursionBlocker++ == 0) {
    NS_ProcessPendingEvents(nsnull, PR_MillisecondsToInterval(100));
    --recursionBlocker;
  }

  
  
  if (::GetQueueStatus(QS_PAINT)) {
    
    HWND topWnd = GetTopLevelHWND(mWnd);

    
    
    
    ::EnumChildWindows(topWnd, nsWindow::DispatchStarvedPaints, NULL);
  }
}

#ifndef WINCE
void nsWindow::PostSleepWakeNotification(const char* aNotification)
{
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
  {
    observerService->NotifyObservers(nsnull, aNotification, nsnull);
  }
}
#endif

PRBool nsWindow::ProcessMessage(UINT msg, WPARAM &wParam, LPARAM &lParam,
                                LRESULT *aRetValue)
{
  PRBool eatMessage;
  if (nsIMM32Handler::ProcessMessage(this, msg, wParam, lParam, aRetValue,
                                     eatMessage)) {
    return mWnd ? eatMessage : PR_TRUE;
  }

  if (PluginHasFocus()) {
    PRBool callDefaultWndProc;
    MSG nativeMsg = InitMSG(msg, wParam, lParam);
    if (ProcessMessageForPlugin(nativeMsg, aRetValue, callDefaultWndProc)) {
      return mWnd ? !callDefaultWndProc : PR_TRUE;
    }
  }

  static UINT vkKeyCached = 0;              
  PRBool result = PR_FALSE;                 
  static PRBool getWheelInfo = PR_TRUE;
  *aRetValue = 0;
  nsPaletteInfo palInfo;

  
  
  
  

  switch (msg) {
    case WM_COMMAND:
    {
      WORD wNotifyCode = HIWORD(wParam); 
      if ((CBN_SELENDOK == wNotifyCode) || (CBN_SELENDCANCEL == wNotifyCode)) { 
        nsGUIEvent event(PR_TRUE, NS_CONTROL_CHANGE, this);
        nsIntPoint point(0,0);
        InitEvent(event, &point); 
        result = DispatchWindowEvent(&event);
      } else if (wNotifyCode == 0) { 
        nsMenuEvent event(PR_TRUE, NS_MENU_SELECTED, this);
        event.mCommand = LOWORD(wParam);
        InitEvent(event);
        result = DispatchWindowEvent(&event);
      }
    }
    break;

#ifndef WINCE
    
    
    case WM_QUERYENDSESSION:
      if (sCanQuit == TRI_UNKNOWN)
      {
        
        
        nsCOMPtr<nsIObserverService> obsServ =
          do_GetService("@mozilla.org/observer-service;1");
        nsCOMPtr<nsISupportsPRBool> cancelQuit =
          do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
        cancelQuit->SetData(PR_FALSE);
        obsServ->NotifyObservers(cancelQuit, "quit-application-requested", nsnull);

        PRBool abortQuit;
        cancelQuit->GetData(&abortQuit);
        sCanQuit = abortQuit ? TRI_FALSE : TRI_TRUE;
      }
      *aRetValue = sCanQuit ? TRUE : FALSE;
      result = PR_TRUE;
      break;

    case WM_ENDSESSION:
      if (wParam == TRUE && sCanQuit == TRI_TRUE)
      {
        
        
        
        
        nsCOMPtr<nsIObserverService> obsServ =
          do_GetService("@mozilla.org/observer-service;1");
        NS_NAMED_LITERAL_STRING(context, "shutdown-persist");
        obsServ->NotifyObservers(nsnull, "quit-application-granted", nsnull);
        obsServ->NotifyObservers(nsnull, "quit-application-forced", nsnull);
        obsServ->NotifyObservers(nsnull, "quit-application", nsnull);
        obsServ->NotifyObservers(nsnull, "profile-change-net-teardown", context.get());
        obsServ->NotifyObservers(nsnull, "profile-change-teardown", context.get());
        obsServ->NotifyObservers(nsnull, "profile-before-change", context.get());
        
        _exit(0);
      }
      sCanQuit = TRI_UNKNOWN;
      result = PR_TRUE;
      break;

    case WM_DISPLAYCHANGE:
      DispatchStandardEvent(NS_DISPLAYCHANGED);
      break;
#endif

    case WM_SYSCOLORCHANGE:
      
      
      
      
      
      DispatchStandardEvent(NS_SYSCOLORCHANGED);
      break;

    case WM_NOTIFY:
      
    {
      LPNMHDR pnmh = (LPNMHDR) lParam;

        switch (pnmh->code) {
          case TCN_SELCHANGE:
          {
            DispatchStandardEvent(NS_TABCHANGE);
            result = PR_TRUE;
          }
          break;
        }
    }
    break;

    case WM_XP_THEMECHANGED:
    {
      DispatchStandardEvent(NS_THEMECHANGED);

      
      
      Invalidate(PR_FALSE);
    }
    break;

    case WM_FONTCHANGE:
    {
      nsresult rv;
      PRBool didChange = PR_FALSE;

      
      nsCOMPtr<nsIFontEnumerator> fontEnum = do_GetService("@mozilla.org/gfx/fontenumerator;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        fontEnum->UpdateFontList(&didChange);
        
        if (didChange)  {
          
          
          
          
          
          nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
          if (prefs) {
            nsCOMPtr<nsIPrefBranch> fiPrefs;
            prefs->GetBranch("font.internaluseonly.", getter_AddRefs(fiPrefs));
            if (fiPrefs) {
              PRBool fontInternalChange = PR_FALSE;
              fiPrefs->GetBoolPref("changed", &fontInternalChange);
              fiPrefs->SetBoolPref("changed", !fontInternalChange);
            }
          }
        }
      } 
    }
    break;

#ifndef WINCE
    case WM_POWERBROADCAST:
      
      
      if (mWindowType == eWindowType_invisible) {
        switch (wParam)
        {
          case PBT_APMSUSPEND:
            PostSleepWakeNotification("sleep_notification");
            break;
          case PBT_APMRESUMEAUTOMATIC:
          case PBT_APMRESUMECRITICAL:
          case PBT_APMRESUMESUSPEND:
            PostSleepWakeNotification("wake_notification");
            break;
        }
      }
      break;
#endif

    case WM_MOVE: 
    {
      PRInt32 x = GET_X_LPARAM(lParam); 
      PRInt32 y = GET_Y_LPARAM(lParam); 
      result = OnMove(x, y);
    }
    break;

    case WM_CLOSE: 
      DispatchStandardEvent(NS_XUL_CLOSE);
      result = PR_TRUE; 
      break;

    case WM_DESTROY:
      
      OnDestroy();
      result = PR_TRUE;
      break;

    case WM_PAINT:
      *aRetValue = (int) OnPaint();
      result = PR_TRUE;
      break;

#ifndef WINCE
    case WM_PRINTCLIENT:
      result = OnPaint((HDC) wParam);
      break;
#endif

#ifdef WINCE
      
    case WM_HOTKEY:
    {
      
      
      
      
      
      
      
      if (VK_TSOFT1 == HIWORD(lParam) && (0 != (MOD_KEYUP & LOWORD(lParam))))
      {
        keybd_event(VK_F19, 0, 0, 0);
        keybd_event(VK_F19, 0, KEYEVENTF_KEYUP, 0);
        result = 0;
        break;
      }
      
      if (VK_TSOFT2 == HIWORD(lParam) && (0 != (MOD_KEYUP & LOWORD(lParam))))
      {
        keybd_event(VK_F20, 0, 0, 0);
        keybd_event(VK_F20, 0, KEYEVENTF_KEYUP, 0);
        result = 0;
        break;
      }
      
      if (VK_TBACK == HIWORD(lParam) && (0 != (MOD_KEYUP & LOWORD(lParam))))
      {
        keybd_event(VK_BACK, 0, 0, 0);
        keybd_event(VK_BACK, 0, KEYEVENTF_KEYUP, 0);
        result = 0;
        break;
      }

      switch (wParam) 
      {
        case VK_APP1:
          keybd_event(VK_F1, 0, 0, 0);
          keybd_event(VK_F1, 0, KEYEVENTF_KEYUP, 0);
          result = 0;
          break;

        case VK_APP2:
          keybd_event(VK_F2, 0, 0, 0);
          keybd_event(VK_F2, 0, KEYEVENTF_KEYUP, 0);
          result = 0;
          break;

        case VK_APP3:
          keybd_event(VK_F3, 0, 0, 0);
          keybd_event(VK_F3, 0, KEYEVENTF_KEYUP, 0);
          result = 0;
          break;

        case VK_APP4:
          keybd_event(VK_F4, 0, 0, 0);
          keybd_event(VK_F4, 0, KEYEVENTF_KEYUP, 0);
          result = 0;
          break;

        case VK_APP5:
          keybd_event(VK_F5, 0, 0, 0);
          keybd_event(VK_F5, 0, KEYEVENTF_KEYUP, 0);
          result = 0;
          break;

        case VK_APP6:
          keybd_event(VK_F6, 0, 0, 0);
          keybd_event(VK_F6, 0, KEYEVENTF_KEYUP, 0);
          result = 0;
          break;
      }
    }
    break;
#endif

    case WM_SYSCHAR:
    case WM_CHAR:
    {
      MSG nativeMsg = InitMSG(msg, wParam, lParam);
      result = ProcessCharMessage(nativeMsg, nsnull);
      DispatchPendingEvents();
    }
    break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
      MSG nativeMsg = InitMSG(msg, wParam, lParam);
      result = ProcessKeyUpMessage(nativeMsg, nsnull);
      DispatchPendingEvents();
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
      MSG nativeMsg = InitMSG(msg, wParam, lParam);
      result = ProcessKeyDownMessage(nativeMsg, nsnull);
      DispatchPendingEvents();
    }
    break;

    
    
    case WM_ERASEBKGND:
      if (! AutoErase()) {
        *aRetValue = 1;
        result = PR_TRUE;
      }
      break;

    case WM_GETDLGCODE:
      *aRetValue = DLGC_WANTALLKEYS;
      result = PR_TRUE;
      break;

    case WM_MOUSEMOVE:
    {
      
      
      
      LPARAM lParamScreen = lParamToScreen(lParam);
      POINT mp;
      mp.x      = GET_X_LPARAM(lParamScreen);
      mp.y      = GET_Y_LPARAM(lParamScreen);
      PRBool userMovedMouse = PR_FALSE;
      if ((gLastMouseMovePoint.x != mp.x) || (gLastMouseMovePoint.y != mp.y)) {
        userMovedMouse = PR_TRUE;
      }

      result = DispatchMouseEvent(NS_MOUSE_MOVE, wParam, lParam);
      if (userMovedMouse) {
        DispatchPendingEvents();
      }
    }
    break;

    case WM_LBUTTONDOWN:
      
      
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam,
                                  PR_FALSE, nsMouseEvent::eLeftButton);
      DispatchPendingEvents();
    }
    break;

    case WM_LBUTTONUP:
      
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam,
                                  PR_FALSE, nsMouseEvent::eLeftButton);
      DispatchPendingEvents();
      break;

#ifndef WINCE
    case WM_MOUSELEAVE:
    {
      
      
      WPARAM mouseState = (GetKeyState(VK_LBUTTON) ? MK_LBUTTON : 0)
        | (GetKeyState(VK_MBUTTON) ? MK_MBUTTON : 0)
        | (GetKeyState(VK_RBUTTON) ? MK_RBUTTON : 0);
      
      
      LPARAM pos = lParamToClient(::GetMessagePos());
      DispatchMouseEvent(NS_MOUSE_EXIT, mouseState, pos);
    }
    break;
#endif

    case WM_CONTEXTMENU:
    {
      
      
      LPARAM pos;
      PRBool contextMenukey = PR_FALSE;
      if (lParam == 0xFFFFFFFF)
      {
        contextMenukey = PR_TRUE;
        pos = lParamToClient(GetMessagePos());
      }
      else
      {
        pos = lParamToClient(lParam);
      }
      result = DispatchMouseEvent(NS_CONTEXTMENU, wParam, pos, contextMenukey,
                                  contextMenukey ?
                                    nsMouseEvent::eLeftButton :
                                    nsMouseEvent::eRightButton);
    }
    break;

    case WM_LBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eLeftButton);
      break;

    case WM_MBUTTONDOWN:
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eMiddleButton);
      DispatchPendingEvents();
    }
    break;

    case WM_MBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eMiddleButton);
      DispatchPendingEvents();
      break;

    case WM_MBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eMiddleButton);
      break;

    case WM_RBUTTONDOWN:
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eRightButton);
      DispatchPendingEvents();
    }
    break;

    case WM_RBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eRightButton);
      DispatchPendingEvents();
      break;

    case WM_RBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam, lParam, PR_FALSE,
                                  nsMouseEvent::eRightButton);
      break;

    case WM_APPCOMMAND:
    {
      PRUint32 appCommand = GET_APPCOMMAND_LPARAM(lParam);

      switch (appCommand)
      {
        case APPCOMMAND_BROWSER_BACKWARD:
        case APPCOMMAND_BROWSER_FORWARD:
        case APPCOMMAND_BROWSER_REFRESH:
        case APPCOMMAND_BROWSER_STOP:
        case APPCOMMAND_BROWSER_SEARCH:
        case APPCOMMAND_BROWSER_FAVORITES:
        case APPCOMMAND_BROWSER_HOME:
          DispatchCommandEvent(appCommand);
          
          *aRetValue = 1;
          result = PR_TRUE;
          break;
      }
      
    }
    break;

    case WM_HSCROLL:
    case WM_VSCROLL:
      
      
      
      if (lParam) {
        nsWindow* scrollbar = GetNSWindowPtr((HWND)lParam);

        if (scrollbar) {
          result = scrollbar->OnScroll(LOWORD(wParam), (short)HIWORD(wParam));
        }
      }
      break;

    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
    
    case WM_CTLCOLORSTATIC:
      if (lParam) {
        nsWindow* control = GetNSWindowPtr((HWND)lParam);
          if (control) {
            control->SetUpForPaint((HDC)wParam);
            *aRetValue = (LPARAM)control->OnControlColor();
          }
      }

      result = PR_TRUE;
      break;

    case WM_ACTIVATE:
      
      
      
      
      
      
      if (mEventCallback) {
        PRInt32 fActive = LOWORD(wParam);

#if defined(WINCE_HAVE_SOFTKB)
        if (mIsTopWidgetWindow && gSoftKeyboardState)
          ToggleSoftKB(fActive);
#endif

        if (WA_INACTIVE == fActive) {
          
          
          
          if (HIWORD(wParam))
            result = DispatchFocusToTopLevelWindow(NS_DEACTIVATE);
          else
            gJustGotDeactivate = PR_TRUE;

#ifndef WINCE
          if (mIsTopWidgetWindow)
            mLastKeyboardLayout = gKbdLayout.GetLayout();
#endif

        } else {
          StopFlashing();

          gJustGotActivate = PR_TRUE;
          nsMouseEvent event(PR_TRUE, NS_MOUSE_ACTIVATE, this,
                             nsMouseEvent::eReal);
          InitEvent(event);

          event.acceptActivation = PR_TRUE;
  
          PRBool result = DispatchWindowEvent(&event);
#ifndef WINCE
          if (event.acceptActivation)
            *aRetValue = MA_ACTIVATE;
          else
            *aRetValue = MA_NOACTIVATE;

          if (gSwitchKeyboardLayout && mLastKeyboardLayout)
            ActivateKeyboardLayout(mLastKeyboardLayout, 0);
#else
          *aRetValue = 0;
#endif
        }
      }
      break;

#ifndef WINCE

    case WM_MOUSEACTIVATE:
      if (mWindowType == eWindowType_popup) {
        
        
        
        
        HWND owner = ::GetWindow(mWnd, GW_OWNER);
        if (owner && owner == ::GetForegroundWindow()) {
          *aRetValue = MA_NOACTIVATE;
          result = PR_TRUE;
        }
      }
      break;

    case WM_WINDOWPOSCHANGING:
    {
      LPWINDOWPOS info = (LPWINDOWPOS) lParam;
      
      if (!(info->flags & SWP_NOZORDER))
        ConstrainZLevel(&info->hwndInsertAfter);
      
      if (mWindowType == eWindowType_invisible)
        info->flags &= ~SWP_SHOWWINDOW;
    }
    break;
#endif

    case WM_SETFOCUS:
      if (gJustGotActivate)
        result = DispatchFocusToTopLevelWindow(NS_ACTIVATE);

#ifdef ACCESSIBILITY
      if (nsWindow::gIsAccessibilityOn) {
        
        nsCOMPtr<nsIAccessible> rootAccessible = GetRootAccessible();
      }
#endif

#if defined(WINCE_HAVE_SOFTKB)
      {
        
        
        
      
        nsIMEContext IMEContext(mWnd);
        
        ImmSetOpenStatus(IMEContext.get(), TRUE);
      }
#endif
      break;

    case WM_KILLFOCUS:
#if defined(WINCE_HAVE_SOFTKB)
      {
        nsIMEContext IMEContext(mWnd);
        ImmSetOpenStatus(IMEContext.get(), FALSE);
      }
#endif
      if (gJustGotDeactivate)
        result = DispatchFocusToTopLevelWindow(NS_DEACTIVATE);
      
      break;

    case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS *wp = (LPWINDOWPOS)lParam;

      
      
      
      if (0 == (wp->flags & SWP_NOSIZE)) {
        
        
        
        RECT r;
        ::GetWindowRect(mWnd, &r);
        PRInt32 newWidth, newHeight;
        newWidth = PRInt32(r.right - r.left);
        newHeight = PRInt32(r.bottom - r.top);
        nsIntRect rect(wp->x, wp->y, newWidth, newHeight);

#ifdef MOZ_XUL
        if (eTransparencyTransparent == mTransparencyMode)
          ResizeTranslucentWindow(newWidth, newHeight);
#endif

        if (newWidth > mLastSize.width)
        {
          RECT drect;

          
          drect.left = wp->x + mLastSize.width;
          drect.top = wp->y;
          drect.right = drect.left + (newWidth - mLastSize.width);
          drect.bottom = drect.top + newHeight;

          ::RedrawWindow(mWnd, &drect, NULL,
                         RDW_INVALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ERASENOW | RDW_ALLCHILDREN);
        }
        if (newHeight > mLastSize.height)
        {
          RECT drect;

          
          drect.left = wp->x;
          drect.top = wp->y + mLastSize.height;
          drect.right = drect.left + newWidth;
          drect.bottom = drect.top + (newHeight - mLastSize.height);

          ::RedrawWindow(mWnd, &drect, NULL,
                         RDW_INVALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ERASENOW | RDW_ALLCHILDREN);
        }

        mBounds.width  = newWidth;
        mBounds.height = newHeight;
        mLastSize.width = newWidth;
        mLastSize.height = newHeight;
        

        
        
        
        
        HWND toplevelWnd = GetTopLevelHWND(mWnd);
        if ( !newWidth && !newHeight && IsIconic(toplevelWnd)) {
          result = PR_FALSE;
          break;
        }

        
        
        if (::GetClientRect(mWnd, &r)) {
          rect.width  = PRInt32(r.right - r.left);
          rect.height = PRInt32(r.bottom - r.top);
        }
        result = OnResize(rect);
      }

      





      if (wp->flags & SWP_FRAMECHANGED && ::IsWindowVisible(mWnd)) {
        nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);
#ifndef WINCE
        WINDOWPLACEMENT pl;
        pl.length = sizeof(pl);
        ::GetWindowPlacement(mWnd, &pl);

        if (pl.showCmd == SW_SHOWMAXIMIZED)
          event.mSizeMode = nsSizeMode_Maximized;
        else if (pl.showCmd == SW_SHOWMINIMIZED)
          event.mSizeMode = nsSizeMode_Minimized;
        else
          event.mSizeMode = nsSizeMode_Normal;
#else
        event.mSizeMode = nsSizeMode_Normal;
#endif
        InitEvent(event);

        result = DispatchWindowEvent(&event);
      }
    }
    break;

    case WM_SETTINGCHANGE:
        getWheelInfo = PR_TRUE;
#ifdef WINCE_WINDOWS_MOBILE
        if (wParam == SPI_SETSIPINFO) {
          NotifySoftKbObservers();
	} else if (wParam == SETTINGCHANGE_RESET) {

	  if (glpDDSecondary) {
	    glpDDSecondary->Release();
	    glpDDSecondary = NULL;
	  }

	  gfxIntSize oldSize = gSharedSurfaceSize;
	  gSharedSurfaceSize.height = GetSystemMetrics(SM_CYSCREEN);
	  gSharedSurfaceSize.width = GetSystemMetrics(SM_CXSCREEN);

	  
	  if (gSharedSurfaceSize.height * gSharedSurfaceSize.width !=
	      oldSize.height * oldSize.width)
	    gSharedSurfaceData = nsnull;

	  glpDD->RestoreAllSurfaces();
	}
#endif
      break;

    case WM_PALETTECHANGED:
      if ((HWND)wParam == mWnd) {
        
        
        result = PR_TRUE;
        break;
      }
      

    case WM_QUERYNEWPALETTE:      
      mContext->GetPaletteInfo(palInfo);
      if (palInfo.isPaletteDevice && palInfo.palette) {
        HDC hDC = ::GetDC(mWnd);
        HPALETTE hOldPal = ::SelectPalette(hDC, (HPALETTE)palInfo.palette, TRUE);

        
        int i = ::RealizePalette(hDC);

#ifdef DEBUG
        
#endif
        
        ::InvalidateRect(mWnd, (LPRECT)NULL, TRUE);

        ::ReleaseDC(mWnd, hDC);
        *aRetValue = TRUE;
      }
      result = PR_TRUE;
      break;

#ifndef WINCE
    case WM_INPUTLANGCHANGEREQUEST:
      *aRetValue = TRUE;
      result = PR_FALSE;
      break;

    case WM_INPUTLANGCHANGE:
      result = OnInputLangChange((HKL)lParam);
      break;

    case WM_DROPFILES:
    {
#if 0
      HDROP hDropInfo = (HDROP) wParam;
      UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);

      for (UINT iFile = 0; iFile < nFiles; iFile++) {
        TCHAR szFileName[_MAX_PATH];
        ::DragQueryFile(hDropInfo, iFile, szFileName, _MAX_PATH);
#ifdef DEBUG
        printf("szFileName [%s]\n", szFileName);
#endif  
        nsAutoString fileStr(szFileName);
        nsEventStatus status;
        nsDragDropEvent event(NS_DRAGDROP_EVENT, this);
        InitEvent(event);
        event.mType      = nsDragDropEventStatus_eDrop;
        event.mIsFileURL = PR_FALSE;
        event.mURL       = fileStr.get();
        DispatchEvent(&event, status);
      }
#endif 
    }
    break;
#endif 

    case WM_DESTROYCLIPBOARD:
    {
      nsIClipboard* clipboard;
      nsresult rv = CallGetService(kCClipboardCID, &clipboard);
      clipboard->EmptyClipboard(nsIClipboard::kGlobalClipboard);
      NS_RELEASE(clipboard);
    }
    break;

#ifdef ACCESSIBILITY
    case WM_GETOBJECT:
    {
      *aRetValue = 0;
      if (lParam == OBJID_CLIENT) { 
        nsCOMPtr<nsIAccessible> rootAccessible = GetRootAccessible(); 
        if (rootAccessible) {
          IAccessible *msaaAccessible = NULL;
          rootAccessible->GetNativeInterface((void**)&msaaAccessible); 
          if (msaaAccessible) {
            *aRetValue = LresultFromObject(IID_IAccessible, wParam, msaaAccessible); 
            msaaAccessible->Release(); 
            result = PR_TRUE;  
          }
        }
      }
    }
#endif

#ifndef WINCE
    case WM_SYSCOMMAND:
      
      if (!gTrimOnMinimize && wParam == SC_MINIMIZE) {
        ::ShowWindow(mWnd, SW_SHOWMINIMIZED);
        result = PR_TRUE;
      }
      break;
#endif


#ifdef WINCE
  case WM_HIBERNATE:        
    nsMemory::HeapMinimize(PR_TRUE);
    break;
#endif
    default:
    {
      
#ifndef WINCE
      if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
        static int iDeltaPerLine, iDeltaPerChar;
        static ULONG ulScrollLines, ulScrollChars = 1;
        static int currentVDelta, currentHDelta;
        static HWND currentWindow = 0;

        PRBool isVertical = msg == WM_MOUSEWHEEL;

        
        if (getWheelInfo) {
          getWheelInfo = PR_FALSE;

          SystemParametersInfo (SPI_GETWHEELSCROLLLINES, 0, &ulScrollLines, 0);

          
          

          
          
          
          
          

          iDeltaPerLine = 0;
          if (ulScrollLines) {
            if (ulScrollLines <= WHEEL_DELTA) {
              iDeltaPerLine = WHEEL_DELTA / ulScrollLines;
            } else {
              ulScrollLines = WHEEL_PAGESCROLL;
            }
          }

          if (!SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0,
                                    &ulScrollChars, 0)) {
            
            ulScrollChars = 1;
          }

          iDeltaPerChar = 0;
          if (ulScrollChars) {
            if (ulScrollChars <= WHEEL_DELTA) {
              iDeltaPerChar = WHEEL_DELTA / ulScrollChars;
            } else {
              ulScrollChars = WHEEL_PAGESCROLL;
            }
          }
        }

        if ((isVertical  && ulScrollLines != WHEEL_PAGESCROLL && !iDeltaPerLine) ||
            (!isVertical && ulScrollChars != WHEEL_PAGESCROLL && !iDeltaPerChar))
          return 0;

        
        

        POINT point;
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        HWND destWnd = ::WindowFromPoint(point);

        
        
        
        

        if (!destWnd) {
          
          break;
        }

        
        DWORD processId = 0;
        GetWindowThreadProcessId(destWnd, &processId);
        if (processId != GetCurrentProcessId())
        {
          
          break;
        }

        nsWindow* destWindow = GetNSWindowPtr(destWnd);
        if (!destWindow || destWindow->mIsPluginWindow) {
          
          
          
          
          
          
          
          
          
          HWND parentWnd = ::GetParent(destWnd);
          while (parentWnd) {
            nsWindow* parentWindow = GetNSWindowPtr(parentWnd);
            if (parentWindow) {
              
              
              
              
              
              if (mIsInMouseWheelProcessing) {
                destWnd = parentWnd;
                destWindow = parentWindow;
              } else {
                
                
                
                
                mIsInMouseWheelProcessing = PR_TRUE;
                if (0 == ::SendMessageW(destWnd, msg, wParam, lParam)) {
                  result = PR_TRUE; 
                }
                destWnd = nsnull;
                mIsInMouseWheelProcessing = PR_FALSE;
              }
              break; 
            }
            parentWnd = ::GetParent(parentWnd);
          } 
        }
        if (destWnd == nsnull)
          break; 
        if (destWnd != mWnd) {
          if (destWindow) {
            return destWindow->ProcessMessage(msg, wParam, lParam, aRetValue);
          }
#ifdef DEBUG
          else
            printf("WARNING: couldn't get child window for MW event\n");
#endif
        }

        
        
        if (currentWindow != mWnd) {
          currentVDelta = 0;
          currentHDelta = 0;
          currentWindow = mWnd;
        }

        nsMouseScrollEvent scrollEvent(PR_TRUE, NS_MOUSE_SCROLL, this);
        scrollEvent.delta = 0;
        if (isVertical) {
          scrollEvent.scrollFlags = nsMouseScrollEvent::kIsVertical;
          if (ulScrollLines == WHEEL_PAGESCROLL) {
            scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
            scrollEvent.delta = (((short) HIWORD (wParam)) > 0) ? -1 : 1;
          } else {
            currentVDelta -= (short) HIWORD (wParam);
            if (PR_ABS(currentVDelta) >= iDeltaPerLine) {
              scrollEvent.delta = currentVDelta / iDeltaPerLine;
              currentVDelta %= iDeltaPerLine;
            }
          }
        } else {
          scrollEvent.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
          if (ulScrollChars == WHEEL_PAGESCROLL) {
            scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
            scrollEvent.delta = (((short) HIWORD (wParam)) > 0) ? 1 : -1;
          } else {
            currentHDelta += (short) HIWORD (wParam);
            if (PR_ABS(currentHDelta) >= iDeltaPerChar) {
              scrollEvent.delta = currentHDelta / iDeltaPerChar;
              currentHDelta %= iDeltaPerChar;
            }
          }
        }

        scrollEvent.isShift   = IS_VK_DOWN(NS_VK_SHIFT);
        scrollEvent.isControl = IS_VK_DOWN(NS_VK_CONTROL);
        scrollEvent.isMeta    = PR_FALSE;
        scrollEvent.isAlt     = IS_VK_DOWN(NS_VK_ALT);
        InitEvent(scrollEvent);
        if (nsnull != mEventCallback) {
          result = DispatchWindowEvent(&scrollEvent);
        }
        
        
        if (result)
          *aRetValue = isVertical ? 0 : TRUE;
      } 

      else if (msg == nsWindow::uWM_HEAP_DUMP) {
        
        
        HeapDump("c:\\heapdump.txt", "whatever");
        result = PR_TRUE;
      }
#endif 

#ifdef NS_ENABLE_TSF
      else if (msg == WM_USER_TSF_TEXTCHANGE) {
        nsTextStore::OnTextChangeMsg();
      }
#endif 

    }
    break;
#ifndef WINCE
  case WM_DWMCOMPOSITIONCHANGED:
    BroadcastMsg(mWnd, WM_DWMCOMPOSITIONCHANGED);
    DispatchStandardEvent(NS_THEMECHANGED);
    if (nsUXThemeData::CheckForCompositor() && mTransparencyMode == eTransparencyGlass) {
      MARGINS margins = { -1, -1, -1, -1 };
      nsUXThemeData::dwmExtendFrameIntoClientAreaPtr(mWnd, &margins);
    }
    Invalidate(PR_FALSE);
    break;
#endif

    

    case WM_TABLET_QUERYSYSTEMGESTURESTATUS:
      
      
      result = PR_TRUE;
      *aRetValue = TABLET_ROTATE_GESTURE_ENABLE;
      break;
    
    case WM_GESTURE:
      result = ProcessGestureMessage(wParam, lParam);
      break;
  }

  
  if (mWnd) {
    return result;
  }
  else {
    
    
    return PR_TRUE;
  }
}

PRBool nsWindow::ProcessGestureMessage(WPARAM wParam, LPARAM lParam)
{
  
  if (mGesture.IsPanEvent(lParam)) {
    nsMouseScrollEvent event(PR_TRUE, NS_MOUSE_PIXEL_SCROLL, this);

    if ( !mGesture.ProcessPanMessage(mWnd, wParam, lParam) )
      return PR_FALSE; 

    nsEventStatus status;

    event.isShift   = IS_VK_DOWN(NS_VK_SHIFT);
    event.isControl = IS_VK_DOWN(NS_VK_CONTROL);
    event.isMeta    = PR_FALSE;
    event.isAlt     = IS_VK_DOWN(NS_VK_ALT);
    event.button    = 0;
    event.time      = ::GetMessageTime();

    PRBool endFeedback = PR_TRUE;
    
    if (mGesture.PanDeltaToPixelScrollX(event)) {
      DispatchEvent(&event, status);
    }
    mGesture.UpdatePanFeedbackX(mWnd, event, endFeedback);
    
    if (mGesture.PanDeltaToPixelScrollY(event)) {
      DispatchEvent(&event, status);
    }
    mGesture.UpdatePanFeedbackY(mWnd, event, endFeedback);
    mGesture.PanFeedbackFinalize(mWnd, endFeedback);
    mGesture.CloseGestureInfoHandle((HGESTUREINFO)lParam);

    return PR_TRUE;
  }

  
  nsSimpleGestureEvent event(PR_TRUE, 0, this, 0, 0.0);
  if ( !mGesture.ProcessGestureMessage(mWnd, wParam, lParam, event) ) {
    return PR_FALSE; 
  }
  
  
  event.isShift   = IS_VK_DOWN(NS_VK_SHIFT);
  event.isControl = IS_VK_DOWN(NS_VK_CONTROL);
  event.isMeta    = PR_FALSE;
  event.isAlt     = IS_VK_DOWN(NS_VK_ALT);
  event.button    = 0;
  event.time      = ::GetMessageTime();

  nsEventStatus status;
  DispatchEvent(&event, status);
  if (status == nsEventStatus_eIgnore) {
    return PR_FALSE; 
  }

  
  mGesture.CloseGestureInfoHandle((HGESTUREINFO)lParam);

  return PR_TRUE; 
}

LRESULT nsWindow::ProcessCharMessage(const MSG &aMsg, PRBool *aEventDispatched)
{
  NS_PRECONDITION(aMsg.message == WM_CHAR || aMsg.message == WM_SYSCHAR,
                  "message is not keydown event");
  PR_LOG(sWindowsLog, PR_LOG_ALWAYS,
         ("%s charCode=%d scanCode=%d\n",
         aMsg.message == WM_SYSCHAR ? "WM_SYSCHAR" : "WM_CHAR",
         aMsg.wParam, HIWORD(aMsg.lParam) & 0xFF));

  
  
  nsModifierKeyState modKeyState;
  return OnChar(aMsg, modKeyState, aEventDispatched);
}

LRESULT nsWindow::ProcessKeyUpMessage(const MSG &aMsg, PRBool *aEventDispatched)
{
  NS_PRECONDITION(aMsg.message == WM_KEYUP || aMsg.message == WM_SYSKEYUP,
                  "message is not keydown event");
  PR_LOG(sWindowsLog, PR_LOG_ALWAYS,
         ("%s VK=%d\n", aMsg.message == WM_SYSKEYDOWN ?
                          "WM_SYSKEYUP" : "WM_KEYUP", aMsg.wParam));

  nsModifierKeyState modKeyState;

  
  
  
  
  
  
  
  
  

  
  if (modKeyState.mIsAltDown && !modKeyState.mIsControlDown &&
      IS_VK_DOWN(NS_VK_SPACE)) {
    return FALSE;
  }

  if (!nsIMM32Handler::IsComposing(this) &&
      (aMsg.message != WM_KEYUP || aMsg.message != VK_MENU)) {
    
    
    
    
    return OnKeyUp(aMsg, modKeyState, aEventDispatched);
  }

  return 0;
}

LRESULT nsWindow::ProcessKeyDownMessage(const MSG &aMsg,
                                        PRBool *aEventDispatched)
{
  PR_LOG(sWindowsLog, PR_LOG_ALWAYS,
         ("%s VK=%d\n", aMsg.message == WM_SYSKEYDOWN ?
                          "WM_SYSKEYDOWN" : "WM_KEYDOWN", aMsg.wParam));
  NS_PRECONDITION(aMsg.message == WM_KEYDOWN || aMsg.message == WM_SYSKEYDOWN,
                  "message is not keydown event");

  nsModifierKeyState modKeyState;

  
  
  
  
  
  
  
  
  

  
  if (modKeyState.mIsAltDown && !modKeyState.mIsControlDown &&
      IS_VK_DOWN(NS_VK_SPACE))
    return FALSE;

  LRESULT result = 0;
  if (modKeyState.mIsAltDown && nsIMM32Handler::IsStatusChanged()) {
    nsIMM32Handler::NotifyEndStatusChange();
  } else if (!nsIMM32Handler::IsComposing(this)) {
    result = OnKeyDown(aMsg, modKeyState, aEventDispatched, nsnull);
  }

#ifndef WINCE
  if (aMsg.wParam == VK_MENU ||
      (aMsg.wParam == VK_F10 && !modKeyState.mIsShiftDown)) {
    
    
    
    
    
    PRBool hasNativeMenu = PR_FALSE;
    HWND hWnd = mWnd;
    while (hWnd) {
      if (::GetMenu(hWnd)) {
        hasNativeMenu = PR_TRUE;
        break;
      }
      hWnd = ::GetParent(hWnd);
    }
    result = !hasNativeMenu;
  }
#endif

  return result;
}

PRBool
nsWindow::ProcessMessageForPlugin(const MSG &aMsg,
                                  LRESULT *aResult,
                                  PRBool &aCallDefWndProc)
{
  NS_PRECONDITION(aResult, "aResult must be non-null.");
  *aResult = 0;

  aCallDefWndProc = PR_FALSE;
  PRBool fallBackToNonPluginProcess = PR_FALSE;
  PRBool eventDispatched = PR_FALSE;
  PRBool dispatchPendingEvents = PR_TRUE;
  switch (aMsg.message) {
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_INPUTLANGCHANGE:
      DispatchPluginEvent(aMsg);
      return PR_FALSE; 

    case WM_CHAR:
    case WM_SYSCHAR:
      *aResult = ProcessCharMessage(aMsg, &eventDispatched);
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      *aResult = ProcessKeyUpMessage(aMsg, &eventDispatched);
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      *aResult = ProcessKeyDownMessage(aMsg, &eventDispatched);
      break;

    case WM_DEADCHAR:
    case WM_SYSDEADCHAR:
    case WM_CONTEXTMENU:

    case WM_CUT:
    case WM_COPY:
    case WM_PASTE:
    case WM_CLEAR:
    case WM_UNDO:

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_CHAR:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CONTROL:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
    case WM_IME_NOTIFY:
    case WM_IME_REQUEST:
    case WM_IME_SELECT:
      break;

    case WM_IME_SETCONTEXT:
      
      
      dispatchPendingEvents = PR_FALSE;
      break;

    default:
      return PR_FALSE;
  }

  if (!eventDispatched)
    aCallDefWndProc = !DispatchPluginEvent(aMsg);
  if (dispatchPendingEvents)
    DispatchPendingEvents();
  return PR_TRUE;
}

PRBool nsWindow::DispatchPluginEvent(const MSG &aMsg)
{
  if (!PluginHasFocus())
    return PR_FALSE;

  nsGUIEvent event(PR_TRUE, NS_PLUGIN_EVENT, this);
  nsIntPoint point(0, 0);
  InitEvent(event, &point);
  nsPluginEvent pluginEvent;
  pluginEvent.event = aMsg.message;
  pluginEvent.wParam = aMsg.wParam;
  pluginEvent.lParam = aMsg.lParam;
  event.nativeMsg = (void *)&pluginEvent;
  return DispatchWindowEvent(&event);
}







#define CS_XP_DROPSHADOW       0x00020000

LPCWSTR nsWindow::WindowClass()
{
  if (!nsWindow::sIsRegistered) {
    WNDCLASSW wc;


    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = ::DefWindowProcW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = nsToolkit::mDllInstance;
    wc.hIcon         = ::LoadIconW(::GetModuleHandleW(NULL), (LPWSTR)IDI_APPLICATION);
    wc.hCursor       = NULL;
    wc.hbrBackground = mBrush;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = kClassNameHidden;

    BOOL succeeded = ::RegisterClassW(&wc) != 0 && 
      ERROR_CLASS_ALREADY_EXISTS != GetLastError();
    nsWindow::sIsRegistered = succeeded;

    wc.lpszClassName = kClassNameContentFrame;
    if (!::RegisterClassW(&wc) && 
      ERROR_CLASS_ALREADY_EXISTS != GetLastError()) {
      nsWindow::sIsRegistered = FALSE;
    }

    wc.lpszClassName = kClassNameContent;
    if (!::RegisterClassW(&wc) && 
      ERROR_CLASS_ALREADY_EXISTS != GetLastError()) {
      nsWindow::sIsRegistered = FALSE;
    }

    wc.lpszClassName = kClassNameUI;
    if (!::RegisterClassW(&wc) && 
      ERROR_CLASS_ALREADY_EXISTS != GetLastError()) {
      nsWindow::sIsRegistered = FALSE;
    }

    wc.lpszClassName = kClassNameGeneral;
    ATOM generalClassAtom = ::RegisterClassW(&wc);
    if (!generalClassAtom && 
      ERROR_CLASS_ALREADY_EXISTS != GetLastError()) {
      nsWindow::sIsRegistered = FALSE;
    }

    wc.lpszClassName = kClassNameDialog;
    wc.hIcon = 0;
    if (!::RegisterClassW(&wc) && 
      ERROR_CLASS_ALREADY_EXISTS != GetLastError()) {
      nsWindow::sIsRegistered = FALSE;
    }
  }

  if (mWindowType == eWindowType_invisible) {
    return kClassNameHidden;
  }
  if (mWindowType == eWindowType_dialog) {
    return kClassNameDialog;
  }
  if (mContentType == eContentTypeContent) {
    return kClassNameContent;
  }
  if (mContentType == eContentTypeContentFrame) {
    return kClassNameContentFrame;
  }
  if (mContentType == eContentTypeUI) {
    return kClassNameUI;
  }
  return kClassNameGeneral;
}

LPCWSTR nsWindow::WindowPopupClass()
{
  const LPCWSTR className = L"MozillaDropShadowWindowClass";

  if (!nsWindow::sIsPopupClassRegistered) {
    WNDCLASSW wc;

    wc.style = CS_DBLCLKS | CS_XP_DROPSHADOW;
    wc.lpfnWndProc   = ::DefWindowProcW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = nsToolkit::mDllInstance;
    wc.hIcon         = ::LoadIconW(::GetModuleHandleW(NULL), (LPWSTR)IDI_APPLICATION);
    wc.hCursor       = NULL;
    wc.hbrBackground = mBrush;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = className;

    nsWindow::sIsPopupClassRegistered = ::RegisterClassW(&wc);
    if (!nsWindow::sIsPopupClassRegistered) {
      
      
      wc.style = CS_DBLCLKS;
      nsWindow::sIsPopupClassRegistered = ::RegisterClassW(&wc);
    }
  }

  return className;
}






DWORD nsWindow::WindowStyle()
{
  DWORD style;

  









  


#if defined(WINCE)
  

  switch (mWindowType) {
    case eWindowType_child:
      style = WS_CHILD;
      break;

    case eWindowType_dialog:
      style = WS_BORDER | WS_POPUP;
#if !defined(WINCE_WINDOWS_MOBILE)
      style |= WS_SYSMENU;
      if (mBorderStyle != eBorderStyle_default)
        style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
#endif
      break;

    case eWindowType_popup:
      style = WS_POPUP | WS_BORDER;
      break;

    default:
      NS_ASSERTION(0, "unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      style = WS_BORDER;
#if !defined(WINCE_WINDOWS_MOBILE)
      style |= WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
#endif
      break;
  }
#else
  switch (mWindowType) {
    case eWindowType_child:
      style = WS_OVERLAPPED;
      break;

    case eWindowType_dialog:
      style = WS_OVERLAPPED | WS_BORDER | WS_DLGFRAME | WS_SYSMENU | DS_3DLOOK | DS_MODALFRAME;
      if (mBorderStyle != eBorderStyle_default)
        style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
      break;

    case eWindowType_popup:
      style = WS_POPUP;
      if (mTransparencyMode == eTransparencyGlass) {
        

        style |= WS_THICKFRAME;
      } else {
        style |= WS_OVERLAPPED;
      }
      break;

    default:
      NS_ASSERTION(0, "unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      style = WS_OVERLAPPED | WS_BORDER | WS_DLGFRAME | WS_SYSMENU |
              WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
      break;
  }
#endif

#ifndef WINCE_WINDOWS_MOBILE
  if (mBorderStyle != eBorderStyle_default && mBorderStyle != eBorderStyle_all) {
    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_border))
      style &= ~WS_BORDER;

    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_title)) {
      style &= ~WS_DLGFRAME;
      style |= WS_POPUP;
      style &= ~WS_CHILD;
    }

    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_close))
      style &= ~0;
    
    

    if (mBorderStyle == eBorderStyle_none ||
      !(mBorderStyle & (eBorderStyle_menu | eBorderStyle_close)))
      style &= ~WS_SYSMENU;
    
    
    
    

    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_resizeh))
      style &= ~WS_THICKFRAME;

    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_minimize))
      style &= ~WS_MINIMIZEBOX;

    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_maximize))
      style &= ~WS_MAXIMIZEBOX;
  }
#endif 
  VERIFY_WINDOW_STYLE(style);
  return style;
}







DWORD nsWindow::WindowExStyle()
{
  switch (mWindowType)
  {
    case eWindowType_child:
      return 0;

    case eWindowType_dialog:
      return WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;

    case eWindowType_popup:
      return
#if defined(WINCE) && !defined(WINCE_WINDOWS_MOBILE)
        WS_EX_NOACTIVATE |
#endif
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

    default:
      NS_ASSERTION(0, "unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      return WS_EX_WINDOWEDGE;
  }
}







void nsWindow::SubclassWindow(BOOL bState)
{
  if (NULL != mWnd) {
    NS_PRECONDITION(::IsWindow(mWnd), "Invalid window handle");

    if (bState) {
      
      if (mUnicodeWidget)
        mPrevWndProc = (WNDPROC)::SetWindowLongPtrW(mWnd, GWLP_WNDPROC,
                                                (LONG_PTR)nsWindow::WindowProc);
      else
        mPrevWndProc = (WNDPROC)::SetWindowLongPtrA(mWnd, GWLP_WNDPROC,
                                                (LONG_PTR)nsWindow::WindowProc);
      NS_ASSERTION(mPrevWndProc, "Null standard window procedure");
      
      SetNSWindowPtr(mWnd, this);
    }
    else {
      if (mUnicodeWidget)
        ::SetWindowLongPtrW(mWnd, GWLP_WNDPROC, (LONG_PTR)mPrevWndProc);
      else
        ::SetWindowLongPtrA(mWnd, GWLP_WNDPROC, (LONG_PTR)mPrevWndProc);
      SetNSWindowPtr(mWnd, NULL);
      mPrevWndProc = NULL;
    }
  }
}







void nsWindow::OnDestroy()
{
  mOnDestroyCalled = PR_TRUE;

  SubclassWindow(FALSE);

  
  
  EnableDragDrop(PR_FALSE);

  mWnd = NULL;

  
  if (mBrush) {
    VERIFY(::DeleteObject(mBrush));
    mBrush = NULL;
  }

#if 0
  if (mPalette) {
    VERIFY(::DeleteObject(mPalette));
    mPalette = NULL;
  }
#endif

  
  
  if (mDeferredPositioner) {
    VERIFY(::EndDeferWindowPos(mDeferredPositioner));
    mDeferredPositioner = NULL;
  }

  
  nsBaseWidget::OnDestroy();

  
  if (!mIsDestroying) {
    
    
    
    AddRef();
    DispatchStandardEvent(NS_DESTROY);
    Release();
  }
}






PRBool nsWindow::OnMove(PRInt32 aX, PRInt32 aY)
{
  mBounds.x = aX;
  mBounds.y = aY;

  nsGUIEvent event(PR_TRUE, NS_MOVE, this);
  InitEvent(event);
  event.refPoint.x = aX;
  event.refPoint.y = aY;

  return DispatchWindowEvent(&event);
}

static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

static void
AddRECTToRegion(const RECT& aRect, nsIRegion* aRegion)
{
  aRegion->Union(aRect.left, aRect.top, aRect.right - aRect.left, aRect.bottom - aRect.top);
}

static already_AddRefed<nsIRegion>
ConvertHRGNToRegion(HRGN aRgn)
{
  NS_ASSERTION(aRgn, "Don't pass NULL region here");

  nsCOMPtr<nsIRegion> region = do_CreateInstance(kRegionCID);
  if (!region)
    return nsnull;

  region->Init();

  DWORD size = ::GetRegionData(aRgn, 0, NULL);
  nsAutoTArray<PRUint8,100> buffer;
  if (!buffer.SetLength(size))
    return region.forget();

  RGNDATA* data = reinterpret_cast<RGNDATA*>(buffer.Elements());
  if (!::GetRegionData(aRgn, size, data))
    return region.forget();

  if (data->rdh.nCount > MAX_RECTS_IN_REGION) {
    AddRECTToRegion(data->rdh.rcBound, region);
    return region.forget();
  }

  RECT* rects = reinterpret_cast<RECT*>(data->Buffer);
  for (PRUint32 i = 0; i < data->rdh.nCount; ++i) {
    RECT* r = rects + i;
    AddRECTToRegion(*r, region);
  }

  return region.forget();
}







#ifdef CAIRO_HAS_DDRAW_SURFACE

static PRBool
InitDDraw()
{
  HRESULT hr;

  hr = DirectDrawCreate(NULL, &glpDD, NULL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  hr = glpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  DDSURFACEDESC ddsd;
  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

  hr = glpDD->CreateSurface(&ddsd, &glpDDPrimary, NULL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  hr = glpDD->CreateClipper(0, &glpDDClipper, NULL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  hr = glpDDPrimary->SetClipper(glpDDClipper);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  
  
  if (gRenderMode != RENDER_IMAGE_DDRAW16)
  {
    gfxIntSize screen_size(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    gpDDSurf = new gfxDDrawSurface(glpDD, screen_size, gfxASurface::ImageFormatRGB24);
    if (!gpDDSurf) {
      
      fprintf(stderr, "couldn't create ddsurf\n");
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

#endif

PRBool nsWindow::OnPaint(HDC aDC)
{
#ifdef CAIRO_HAS_DDRAW_SURFACE
  if (gRenderMode == RENDER_IMAGE_DDRAW16) {
    return OnPaintImageDDraw16();
  }
#endif

  PRBool result = PR_TRUE;
  PAINTSTRUCT ps;
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

#ifdef MOZ_XUL
  if (!aDC && (eTransparencyTransparent == mTransparencyMode))
  {
    
    
    
    
    
    
    ::BeginPaint(mWnd, &ps);
    ::EndPaint(mWnd, &ps);

    aDC = mMemoryDC;
  }
#endif

  mPainting = PR_TRUE;

#ifdef NS_DEBUG
  HRGN debugPaintFlashRegion = NULL;
  HDC debugPaintFlashDC = NULL;

  if (debug_WantPaintFlashing())
  {
    debugPaintFlashRegion = ::CreateRectRgn(0, 0, 0, 0);
    ::GetUpdateRgn(mWnd, debugPaintFlashRegion, TRUE);
    debugPaintFlashDC = ::GetDC(mWnd);
  }
#endif 

  HDC hDC = aDC ? aDC : (::BeginPaint(mWnd, &ps));
  mPaintDC = hDC;
  HRGN paintRgn = NULL;

#ifdef MOZ_XUL
  if (aDC || (eTransparencyTransparent == mTransparencyMode)) {
#else
  if (aDC) {
#endif

    RECT paintRect;
    ::GetClientRect(mWnd, &paintRect);
    paintRgn = ::CreateRectRgn(paintRect.left, paintRect.top, paintRect.right, paintRect.bottom);
  }
  else {
#ifndef WINCE
    paintRgn = ::CreateRectRgn(0, 0, 0, 0);
    if (paintRgn != NULL) {
      int result = GetRandomRgn(hDC, paintRgn, SYSRGN);
      if (result == 1) {
        POINT pt = {0,0};
        ::MapWindowPoints(NULL, mWnd, &pt, 1);
        ::OffsetRgn(paintRgn, pt.x, pt.y);
      }
    }
#else
    paintRgn = ::CreateRectRgn(ps.rcPaint.left, ps.rcPaint.top, 
                               ps.rcPaint.right, ps.rcPaint.bottom);
#endif
  }

#ifdef DEBUG_vladimir
  nsFunctionTimer ft("OnPaint [%d %d %d %d]",
                     ps.rcPaint.left, ps.rcPaint.top, 
                     ps.rcPaint.right - ps.rcPaint.left,
                     ps.rcPaint.bottom - ps.rcPaint.top);
#endif

  nsCOMPtr<nsIRegion> paintRgnWin;
  if (paintRgn) {
    paintRgnWin = ConvertHRGNToRegion(paintRgn);
    ::DeleteObject(paintRgn);
  }

  if (paintRgnWin &&
      !paintRgnWin->IsEmpty() &&
      mEventCallback)
  {
    
    nsPaintEvent event(PR_TRUE, NS_PAINT, this);

    InitEvent(event);

    event.region = paintRgnWin;
    event.rect = nsnull;
 
    
    

#ifdef NS_DEBUG
    debug_DumpPaintEvent(stdout,
                         this,
                         &event,
                         nsCAutoString("noname"),
                         (PRInt32) mWnd);
#endif 

    nsRefPtr<gfxASurface> targetSurface;

#if defined(MOZ_XUL)
    
    if (gRenderMode == RENDER_GDI && eTransparencyTransparent == mTransparencyMode) {
      if (mTransparentSurface == nsnull)
        SetupTranslucentWindowMemoryBitmap(mTransparencyMode);
      targetSurface = mTransparentSurface;
    }
#endif

    nsRefPtr<gfxWindowsSurface> targetSurfaceWin;
    if (!targetSurface &&
        gRenderMode == RENDER_GDI)
    {
      targetSurfaceWin = new gfxWindowsSurface(hDC);
      targetSurface = targetSurfaceWin;
    }

#ifdef CAIRO_HAS_DDRAW_SURFACE
    nsRefPtr<gfxDDrawSurface> targetSurfaceDDraw;
    if (!targetSurface &&
        gRenderMode == RENDER_DDRAW)
    {
      if (!glpDD) {
        if (!InitDDraw()) {
          NS_WARNING("DirectDraw init failed; falling back to RENDER_IMAGE_STRETCH24");
          gRenderMode = RENDER_IMAGE_STRETCH24;
          goto DDRAW_FAILED;
        }
      }

      
      
      RECT winrect;
      GetClientRect(mWnd, &winrect);
      MapWindowPoints(mWnd, NULL, (LPPOINT)&winrect, 2);

      targetSurfaceDDraw = new gfxDDrawSurface(gpDDSurf.get(), winrect);
      targetSurface = targetSurfaceDDraw;
    }
#endif

DDRAW_FAILED:
    nsRefPtr<gfxImageSurface> targetSurfaceImage;
    if (!targetSurface &&
        (gRenderMode == RENDER_IMAGE_STRETCH32 ||
         gRenderMode == RENDER_IMAGE_STRETCH24))
    {
      if (!gSharedSurfaceData) {
        gSharedSurfaceSize.height = GetSystemMetrics(SM_CYSCREEN);
        gSharedSurfaceSize.width = GetSystemMetrics(SM_CXSCREEN);
        gSharedSurfaceData = (PRUint8*) malloc(gSharedSurfaceSize.width * gSharedSurfaceSize.height * 4);
      }

      gfxIntSize surfaceSize(ps.rcPaint.right - ps.rcPaint.left,
                             ps.rcPaint.bottom - ps.rcPaint.top);

      if (!gSharedSurfaceData ||
          surfaceSize.width > gSharedSurfaceSize.width ||
          surfaceSize.height > gSharedSurfaceSize.height)
      {
        
        
        targetSurfaceImage = new gfxImageSurface(surfaceSize, gfxASurface::ImageFormatRGB24);
      } else {
        
        
        targetSurfaceImage = new gfxImageSurface(gSharedSurfaceData.get(),
                                                 surfaceSize,
                                                 surfaceSize.width * 4,
                                                 gfxASurface::ImageFormatRGB24);
      }

      if (targetSurfaceImage && !targetSurfaceImage->CairoStatus()) {
        targetSurfaceImage->SetDeviceOffset(gfxPoint(-ps.rcPaint.left, -ps.rcPaint.top));
        targetSurface = targetSurfaceImage;
      }
    }

    if (!targetSurface) {
      NS_ERROR("Invalid gRenderMode!");
      return NS_ERROR_FAILURE;
    }

    nsRefPtr<gfxContext> thebesContext = new gfxContext(targetSurface);
    thebesContext->SetFlag(gfxContext::FLAG_DESTINED_FOR_SCREEN);

#ifdef WINCE
    thebesContext->SetFlag(gfxContext::FLAG_SIMPLIFY_OPERATORS);
#endif

    
    if (gRenderMode == RENDER_GDI) {
# if defined(MOZ_XUL) && !defined(WINCE)
      if (eTransparencyGlass == mTransparencyMode && nsUXThemeData::sHaveCompositor) {
        thebesContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
      } else if (eTransparencyTransparent == mTransparencyMode) {
        
        
        thebesContext->SetOperator(gfxContext::OPERATOR_CLEAR);
        thebesContext->Paint();
        thebesContext->SetOperator(gfxContext::OPERATOR_OVER);
      } else
#endif
      {
        
        thebesContext->PushGroup(gfxASurface::CONTENT_COLOR);
      }
    }

    nsCOMPtr<nsIRenderingContext> rc;
    nsresult rv = mContext->CreateRenderingContextInstance (*getter_AddRefs(rc));
    if (NS_FAILED(rv)) {
      NS_WARNING("CreateRenderingContextInstance failed");
      return PR_FALSE;
    }

    rv = rc->Init(mContext, thebesContext);
    if (NS_FAILED(rv)) {
      NS_WARNING("RC::Init failed");
      return PR_FALSE;
    }

#ifdef DEBUG_vladimir
    ft.Mark("Init");
#endif

    event.renderingContext = rc;
    result = DispatchWindowEvent(&event, eventStatus);
    event.renderingContext = nsnull;

#ifdef DEBUG_vladimir
    ft.Mark("Dispatch");
#endif

#ifdef MOZ_XUL
    if (gRenderMode == RENDER_GDI &&
        eTransparencyTransparent == mTransparencyMode) {
      
      
      
      UpdateTranslucentWindow();
    } else
#endif
    if (result) {
      if (gRenderMode == RENDER_GDI) {
        
        
        thebesContext->PopGroupToSource();
        thebesContext->SetOperator(gfxContext::OPERATOR_SOURCE);
        thebesContext->Paint();
      } else if (gRenderMode == RENDER_DDRAW) {
#ifdef CAIRO_HAS_DDRAW_SURFACE
        
        HRESULT hr = glpDDClipper->SetHWnd(0, mWnd);
        if (FAILED(hr))
          DDError("SetHWnd", hr);

        
        
        RECT dst_rect = ps.rcPaint;
        MapWindowPoints(mWnd, NULL, (LPPOINT)&dst_rect, 2);
        hr = glpDDPrimary->Blt(&dst_rect,
                               gpDDSurf->GetDDSurface(),
                               &dst_rect,
                               DDBLT_WAITNOTBUSY,
                               NULL);
        if (FAILED(hr))
          DDError("Blt", hr);
#endif
      } else if (gRenderMode == RENDER_IMAGE_STRETCH24 ||
                 gRenderMode == RENDER_IMAGE_STRETCH32) 
      {
        gfxIntSize surfaceSize = targetSurfaceImage->GetSize();

        
        BITMAPINFOHEADER bi;
        memset(&bi, 0, sizeof(BITMAPINFOHEADER));
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = surfaceSize.width;
        bi.biHeight = - surfaceSize.height;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;

        if (gRenderMode == RENDER_IMAGE_STRETCH24) {
          
          
          
          
          
          int srcstride = surfaceSize.width*4;
          int dststride = surfaceSize.width*3;
          dststride = (dststride + 3) & ~3;

          
          for (int j = 0; j < surfaceSize.height; ++j) {
            unsigned int *src = (unsigned int*) (targetSurfaceImage->Data() + j*srcstride);
            unsigned int *dst = (unsigned int*) (targetSurfaceImage->Data() + j*dststride);

            
            
            
            
            
            
            int width_left = surfaceSize.width;
            while (width_left >= 4) {
              unsigned int a = *src++;
              unsigned int b = *src++;
              unsigned int c = *src++;
              unsigned int d = *src++;

              *dst++ =  (a & 0x00ffffff)        | (b << 24);
              *dst++ = ((b & 0x00ffff00) >> 8)  | (c << 16);
              *dst++ = ((c & 0x00ff0000) >> 16) | (d << 8);

              width_left -= 4;
            }

            
            
            unsigned char *bsrc = (unsigned char*) src;
            unsigned char *bdst = (unsigned char*) dst;
            switch (width_left) {
              case 3:
                *bdst++ = *bsrc++;
                *bdst++ = *bsrc++;
                *bdst++ = *bsrc++;
                bsrc++;
              case 2:
                *bdst++ = *bsrc++;
                *bdst++ = *bsrc++;
                *bdst++ = *bsrc++;
                bsrc++;
              case 1:
                *bdst++ = *bsrc++;
                *bdst++ = *bsrc++;
                *bdst++ = *bsrc++;
                bsrc++;
              case 0:
                break;
            }
          }

          bi.biBitCount = 24;
        }

        StretchDIBits(hDC,
                      ps.rcPaint.left, ps.rcPaint.top,
                      surfaceSize.width, surfaceSize.height,
                      0, 0,
                      surfaceSize.width, surfaceSize.height,
                      targetSurfaceImage->Data(),
                      (BITMAPINFO*) &bi,
                      DIB_RGB_COLORS,
                      SRCCOPY);
      }

#ifdef DEBUG_vladimir
      ft.Mark("Blit");
#endif
    } else {
#ifdef DEBUG_vladimir
      ft.Mark("Discard!");
#endif
    }
  }

  if (!aDC) {
    ::EndPaint(mWnd, &ps);
  }

  mPaintDC = nsnull;

#if defined(NS_DEBUG) && !defined(WINCE)
  if (debug_WantPaintFlashing())
  {
    
    
    
    if (nsEventStatus_eIgnore != eventStatus) {
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
    }
    ::ReleaseDC(mWnd, debugPaintFlashDC);
    ::DeleteObject(debugPaintFlashRegion);
  }
#endif 

  mPainting = PR_FALSE;

  return result;
}

#ifdef CAIRO_HAS_DDRAW_SURFACE

PRBool nsWindow::OnPaintImageDDraw16()
{
  PRBool result = PR_TRUE;
  PAINTSTRUCT ps;
  gfxIntSize surfaceSize;
  nsPaintEvent event(PR_TRUE, NS_PAINT, this);
  RECT renderArea;

  nsEventStatus eventStatus = nsEventStatus_eIgnore;
  nsCOMPtr<nsIRenderingContext> rc;
  nsRefPtr<gfxImageSurface> targetSurfaceImage;
  nsRefPtr<gfxContext> thebesContext;

  mPainting = PR_TRUE;

  HDC hDC = ::BeginPaint(mWnd, &ps);
  mPaintDC = hDC;

  HRGN paintRgn = ::CreateRectRgn(ps.rcPaint.left, ps.rcPaint.top, 
                                  ps.rcPaint.right, ps.rcPaint.bottom);
#ifdef DEBUG_vladimir
  nsFunctionTimer ft("OnPaint [%d %d %d %d]",
                     ps.rcPaint.left, ps.rcPaint.top, 
                     ps.rcPaint.right - ps.rcPaint.left,
                     ps.rcPaint.bottom - ps.rcPaint.top);
#endif

  nsCOMPtr<nsIRegion> paintRgnWin;
  if (paintRgn) {
    paintRgnWin = ConvertHRGNToRegion(paintRgn);
    ::DeleteObject(paintRgn);
  }

  if (!paintRgnWin || paintRgnWin->IsEmpty() || !mEventCallback) {
    printf("nothing to paint\n");
    goto cleanup;
  }

  InitEvent(event);
  
  event.region = paintRgnWin;
  event.rect = nsnull;
  
  if (!glpDD) {
    if (!InitDDraw()) {
      NS_WARNING("DirectDraw init failed.  Giving up.");
      goto cleanup;
    }
  }  
  
  if (!gSharedSurfaceData) {
    gSharedSurfaceSize.height = GetSystemMetrics(SM_CYSCREEN);
    gSharedSurfaceSize.width = GetSystemMetrics(SM_CXSCREEN);
    gSharedSurfaceData = (PRUint8*) malloc(gSharedSurfaceSize.width * gSharedSurfaceSize.height * 4);
  }

  if (!glpDDSecondary) {

    memset(&gDDSDSecondary, 0, sizeof (gDDSDSecondary));
    memset(&gDDSDSecondary.ddpfPixelFormat, 0, sizeof(gDDSDSecondary.ddpfPixelFormat));
    
    gDDSDSecondary.dwSize = sizeof (gDDSDSecondary);
    gDDSDSecondary.ddpfPixelFormat.dwSize = sizeof(gDDSDSecondary.ddpfPixelFormat);
    
    gDDSDSecondary.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

    gDDSDSecondary.dwHeight = gSharedSurfaceSize.height;
    gDDSDSecondary.dwWidth  = gSharedSurfaceSize.width;

    gDDSDSecondary.ddpfPixelFormat.dwFlags = DDPF_RGB;
    gDDSDSecondary.ddpfPixelFormat.dwRGBBitCount = 16;
    gDDSDSecondary.ddpfPixelFormat.dwRBitMask = 0xf800;
    gDDSDSecondary.ddpfPixelFormat.dwGBitMask = 0x07e0;
    gDDSDSecondary.ddpfPixelFormat.dwBBitMask = 0x001f;
    
    HRESULT hr = glpDD->CreateSurface(&gDDSDSecondary, &glpDDSecondary, 0);
    if (FAILED(hr)) {
      DDError("CreateSurface renderer", hr);
      goto cleanup;
    }
  }

  surfaceSize = gfxIntSize(ps.rcPaint.right - ps.rcPaint.left,
                           ps.rcPaint.bottom - ps.rcPaint.top);

  targetSurfaceImage = new gfxImageSurface(gSharedSurfaceData.get(),
					   surfaceSize,
					   surfaceSize.width * 4,
					   gfxASurface::ImageFormatRGB24);

  if (!targetSurfaceImage || targetSurfaceImage->CairoStatus()) {
    NS_ERROR("Invalid targetSurfaceImage!");
    goto cleanup;
  }

  targetSurfaceImage->SetDeviceOffset(gfxPoint(-ps.rcPaint.left, -ps.rcPaint.top));

  thebesContext = new gfxContext(targetSurfaceImage);
  thebesContext->SetFlag(gfxContext::FLAG_DESTINED_FOR_SCREEN);
  thebesContext->SetFlag(gfxContext::FLAG_SIMPLIFY_OPERATORS);
  
  nsresult rv = mContext->CreateRenderingContextInstance (*getter_AddRefs(rc));
  if (NS_FAILED(rv)) {
    NS_WARNING("CreateRenderingContextInstance failed");
    goto cleanup;
  }
  
  rv = rc->Init(mContext, thebesContext);
  if (NS_FAILED(rv)) {
    NS_WARNING("RC::Init failed");
    goto cleanup;
  }
  
#ifdef DEBUG_vladimir
  ft.Mark("Init");
#endif
  event.renderingContext = rc;
  result = DispatchWindowEvent(&event, eventStatus);
  event.renderingContext = nsnull;
  
#ifdef DEBUG_vladimir
  ft.Mark("Dispatch");
#endif
  
  if (!result) {
    printf("result is null from dispatch\n");
    goto cleanup;
  }
    
  HRESULT hr;  
  hr = glpDDSecondary->Lock(0, &gDDSDSecondary, DDLOCK_WAITNOTBUSY | DDLOCK_DISCARD, 0);  
  if (FAILED(hr)) {
    DDError("Failed to lock renderer", hr);
    goto cleanup;
  }
  
#ifdef DEBUG_vladimir
  ft.Mark("Locked");
#endif
  
  pixman_image_t *srcPixmanImage = pixman_image_create_bits(PIXMAN_x8r8g8b8,
                                                            surfaceSize.width,
                                                            surfaceSize.height,
                                                            (uint32_t*) gSharedSurfaceData.get(),
                                                            surfaceSize.width * 4);
  
  pixman_image_t *dstPixmanImage = pixman_image_create_bits(PIXMAN_r5g6b5,
                                                            surfaceSize.width,
                                                            surfaceSize.height,
                                                            (uint32_t*) gDDSDSecondary.lpSurface,
                                                            gDDSDSecondary.dwWidth * 2);
 
#ifdef DEBUG_vladimir
  ft.Mark("created pixman images");
#endif

  pixman_image_composite(PIXMAN_OP_SRC,
                         srcPixmanImage,
                         NULL,
                         dstPixmanImage,
                         0, 0,
                         0, 0,
                         0, 0,
                         surfaceSize.width,
                         surfaceSize.height);

  pixman_image_unref(dstPixmanImage);
  pixman_image_unref(srcPixmanImage);

#ifdef DEBUG_vladimir
  ft.Mark("composite");
#endif
  
  hr = glpDDSecondary->Unlock(0);
  if (FAILED(hr)) {
    DDError("Failed to unlock renderer", hr);
    goto cleanup;
  }

#ifdef DEBUG_vladimir
  ft.Mark("unlock");
#endif

  hr = glpDDClipper->SetHWnd(0, mWnd);
  if (FAILED(hr)) {
    DDError("SetHWnd", hr);
    goto cleanup;
  }

#ifdef DEBUG_vladimir
  ft.Mark("sethwnd");
#endif

  
  renderArea = ps.rcPaint;
  MapWindowPoints(mWnd, 0, (LPPOINT)&renderArea, 2);

#ifdef DEBUG_vladimir
  ft.Mark("preblt");
#endif

  
  ps.rcPaint.right = surfaceSize.width;
  ps.rcPaint.bottom = surfaceSize.height;
  ps.rcPaint.left = ps.rcPaint.top = 0;
  
  hr = glpDDPrimary->Blt(&renderArea,
                         glpDDSecondary,
                         &ps.rcPaint,
                         DDBLT_WAITNOTBUSY, 
                         NULL);
  if (FAILED(hr)) {
    DDError("Blt", hr);
    goto cleanup;
  }

#ifdef DEBUG_vladimir
  ft.Mark("Blit");
#endif

cleanup:
 
  ::EndPaint(mWnd, &ps);
  mPaintDC = nsnull;
  mPainting = PR_FALSE;
  return result;
}
#endif






PRBool nsWindow::OnResize(nsIntRect &aWindowRect)
{
  
  if (mEventCallback) {
    nsSizeEvent event(PR_TRUE, NS_SIZE, this);
    InitEvent(event);
    event.windowSize = &aWindowRect;
    RECT r;
    if (::GetWindowRect(mWnd, &r)) {
      event.mWinWidth  = PRInt32(r.right - r.left);
      event.mWinHeight = PRInt32(r.bottom - r.top);
    } else {
      event.mWinWidth  = 0;
      event.mWinHeight = 0;
    }
    return DispatchWindowEvent(&event);
  }

  return PR_FALSE;
}

static PRBool IsTopLevelMouseExit(HWND aWnd)
{
  DWORD pos = ::GetMessagePos();
  POINT mp;
  mp.x = GET_X_LPARAM(pos);
  mp.y = GET_Y_LPARAM(pos);
  HWND mouseWnd = ::WindowFromPoint(mp);

  
  
  
  HWND mouseTopLevel = nsWindow::GetTopLevelHWND(mouseWnd);
  if (mouseWnd == mouseTopLevel)
    return PR_TRUE;

  return nsWindow::GetTopLevelHWND(aWnd) != mouseTopLevel;
}






PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType, WPARAM wParam,
                                    LPARAM lParam, PRBool aIsContextMenuKey,
                                    PRInt16 aButton)
{
  PRBool result = PR_FALSE;

  if (!mEventCallback) {
    return result;
  }

  nsIntPoint eventPoint;
  eventPoint.x = GET_X_LPARAM(lParam);
  eventPoint.y = GET_Y_LPARAM(lParam);

  nsMouseEvent event(PR_TRUE, aEventType, this, nsMouseEvent::eReal,
                     aIsContextMenuKey
                     ? nsMouseEvent::eContextMenuKey
                     : nsMouseEvent::eNormal);
  if (aEventType == NS_CONTEXTMENU && aIsContextMenuKey) {
    nsIntPoint zero(0, 0);
    InitEvent(event, &zero);
  } else {
    InitEvent(event, &eventPoint);
  }

  event.isShift   = IS_VK_DOWN(NS_VK_SHIFT);
  event.isControl = IS_VK_DOWN(NS_VK_CONTROL);
  event.isMeta    = PR_FALSE;
  event.isAlt     = IS_VK_DOWN(NS_VK_ALT);
  event.button    = aButton;

  nsIntPoint mpScreen = eventPoint + WidgetToScreenOffset();

  
  if (aEventType == NS_MOUSE_MOVE) 
  {
    if ((gLastMouseMovePoint.x == mpScreen.x) && (gLastMouseMovePoint.y == mpScreen.y))
      return result;
    gLastMouseMovePoint.x = mpScreen.x;
    gLastMouseMovePoint.y = mpScreen.y;
  }

  PRBool insideMovementThreshold = (abs(gLastMousePoint.x - eventPoint.x) < (short)::GetSystemMetrics(SM_CXDOUBLECLK)) &&
                                   (abs(gLastMousePoint.y - eventPoint.y) < (short)::GetSystemMetrics(SM_CYDOUBLECLK));

  BYTE eventButton;
  switch (aButton) {
    case nsMouseEvent::eLeftButton:
      eventButton = VK_LBUTTON;
      break;
    case nsMouseEvent::eMiddleButton:
      eventButton = VK_MBUTTON;
      break;
    case nsMouseEvent::eRightButton:
      eventButton = VK_RBUTTON;
      break;
    default:
      eventButton = 0;
      break;
  }

  
  
  LONG curMsgTime = ::GetMessageTime();

  if (aEventType == NS_MOUSE_DOUBLECLICK) {
    event.message = NS_MOUSE_BUTTON_DOWN;
    event.button = aButton;
    gLastClickCount = 2;
  }
  else if (aEventType == NS_MOUSE_BUTTON_UP) {
    
    gLastMousePoint.x = eventPoint.x;
    gLastMousePoint.y = eventPoint.y;
    gLastMouseButton = eventButton;
  }
  else if (aEventType == NS_MOUSE_BUTTON_DOWN) {
    

#ifdef NS_DEBUG_XX
    printf("Msg: %d Last: %d Dif: %d Max %d\n", curMsgTime, gLastMouseDownTime, curMsgTime-gLastMouseDownTime, ::GetDoubleClickTime());
    printf("Mouse %d %d\n", abs(gLastMousePoint.x - mp.x), abs(gLastMousePoint.y - mp.y));
#endif
    if (((curMsgTime - gLastMouseDownTime) < (LONG)::GetDoubleClickTime()) && insideMovementThreshold &&
        eventButton == gLastMouseButton) {
      gLastClickCount ++;
    } else {
      
      gLastClickCount = 1;
    }
    
    gLastMouseDownTime = curMsgTime;
  }
  else if (aEventType == NS_MOUSE_MOVE && !insideMovementThreshold) {
    gLastClickCount = 0;
  }
  else if (aEventType == NS_MOUSE_EXIT) {
    event.exit = IsTopLevelMouseExit(mWnd) ? nsMouseEvent::eTopLevel : nsMouseEvent::eChild;
  }
  event.clickCount = gLastClickCount;

#ifdef NS_DEBUG_XX
  printf("Msg Time: %d Click Count: %d\n", curMsgTime, event.clickCount);
#endif

  nsPluginEvent pluginEvent;

  switch (aEventType)
  {
    case NS_MOUSE_BUTTON_DOWN:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_LBUTTONDOWN;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_MBUTTONDOWN;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_RBUTTONDOWN;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_BUTTON_UP:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_LBUTTONUP;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_MBUTTONUP;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_RBUTTONUP;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_DOUBLECLICK:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_LBUTTONDBLCLK;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_MBUTTONDBLCLK;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_RBUTTONDBLCLK;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_MOVE:
      pluginEvent.event = WM_MOUSEMOVE;
      break;
    default:
      pluginEvent.event = WM_NULL;
      break;
  }

  pluginEvent.wParam = wParam;     
  pluginEvent.lParam = lParam;

  event.nativeMsg = (void *)&pluginEvent;

  
  if (nsnull != mEventCallback) {
    if (nsToolkit::gMouseTrailer)
      nsToolkit::gMouseTrailer->Disable();
    if (aEventType == NS_MOUSE_MOVE) {
      if (nsToolkit::gMouseTrailer && !mIsInMouseCapture) {
        nsToolkit::gMouseTrailer->SetMouseTrailerWindow(mWnd);
      }
      nsIntRect rect;
      GetBounds(rect);
      rect.x = 0;
      rect.y = 0;

      if (rect.Contains(event.refPoint)) {
        if (gCurrentWindow == NULL || gCurrentWindow != this) {
          if ((nsnull != gCurrentWindow) && (!gCurrentWindow->mIsDestroying)) {
            LPARAM pos = gCurrentWindow->lParamToClient(lParamToScreen(lParam));
            gCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT, wParam, pos);
          }
          gCurrentWindow = this;
          if (!mIsDestroying) {
            LPARAM pos = gCurrentWindow->lParamToClient(lParamToScreen(lParam));
            gCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER, wParam, pos);
          }
        }
      }
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (gCurrentWindow == this) {
        gCurrentWindow = nsnull;
      }
    }

    result = DispatchWindowEvent(&event);

    if (nsToolkit::gMouseTrailer)
      nsToolkit::gMouseTrailer->Enable();

    
    
    
    return result;
  }

  return result;
}






#ifdef ACCESSIBILITY
PRBool nsWindow::DispatchAccessibleEvent(PRUint32 aEventType, nsIAccessible** aAcc, nsIntPoint* aPoint)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback) {
    return result;
  }

  *aAcc = nsnull;

  nsAccessibleEvent event(PR_TRUE, aEventType, this);
  InitEvent(event, aPoint);

  event.isShift   = IS_VK_DOWN(NS_VK_SHIFT);
  event.isControl = IS_VK_DOWN(NS_VK_CONTROL);
  event.isMeta    = PR_FALSE;
  event.isAlt     = IS_VK_DOWN(NS_VK_ALT);
  event.accessible = nsnull;

  result = DispatchWindowEvent(&event);

  
  if (event.accessible)
    *aAcc = event.accessible;

  return result;
}
#endif






PRBool nsWindow::DispatchFocusToTopLevelWindow(PRUint32 aEventType)
{
  if (aEventType == NS_ACTIVATE)
    gJustGotActivate = PR_FALSE;
  gJustGotDeactivate = PR_FALSE;

  
  
  
  HWND toplevelWnd = GetTopLevelHWND(mWnd);
  if (toplevelWnd) {
    nsWindow *win = GetNSWindowPtr(toplevelWnd);
    if (win)
      return win->DispatchFocus(aEventType);
  }

  return PR_FALSE;
}


PRBool nsWindow::DispatchFocus(PRUint32 aEventType)
{
  
  if (mEventCallback) {
    nsGUIEvent event(PR_TRUE, aEventType, this);
    InitEvent(event);

    
    event.refPoint.x = 0;
    event.refPoint.y = 0;

    nsPluginEvent pluginEvent;

    switch (aEventType)
    {
      case NS_ACTIVATE:
        pluginEvent.event = WM_SETFOCUS;
        break;
      case NS_DEACTIVATE:
        pluginEvent.event = WM_KILLFOCUS;
        break;
      case NS_PLUGIN_ACTIVATE:
        pluginEvent.event = WM_KILLFOCUS;
        break;
      default:
        break;
    }

    event.nativeMsg = (void *)&pluginEvent;

    return DispatchWindowEvent(&event);
  }
  return PR_FALSE;
}







PRBool nsWindow::OnScroll(UINT scrollCode, int cPos)
{
  return PR_FALSE;
}







HBRUSH nsWindow::OnControlColor()
{
  return mBrush;
}






PRBool ChildWindow::DispatchMouseEvent(PRUint32 aEventType, WPARAM wParam, LPARAM lParam,
                                       PRBool aIsContextMenuKey, PRInt16 aButton)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback) {
    return result;
  }

  switch (aEventType) {
    case NS_MOUSE_BUTTON_DOWN:
      CaptureMouse(PR_TRUE);
      break;

    
    
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_MOVE:
    case NS_MOUSE_EXIT:
      if (!(wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)) && mIsInMouseCapture)
        CaptureMouse(PR_FALSE);
      break;

    default:
      break;

  } 

  return nsWindow::DispatchMouseEvent(aEventType, wParam, lParam,
                                      aIsContextMenuKey, aButton);
}






DWORD ChildWindow::WindowStyle()
{
  DWORD style = WS_CLIPCHILDREN | nsWindow::WindowStyle();
  if (!(style & WS_POPUP))
    style |= WS_CHILD; 
  VERIFY_WINDOW_STYLE(style);
  return style;
}

NS_METHOD nsWindow::SetTitle(const nsAString& aTitle)
{
  const nsString& strTitle = PromiseFlatString(aTitle);
  ::SendMessageW(mWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)(LPCWSTR)strTitle.get());
  return NS_OK;
}

NS_METHOD nsWindow::SetIcon(const nsAString& aIconSpec) 
{
#ifndef WINCE
  

  nsCOMPtr<nsILocalFile> iconFile;
  ResolveIconName(aIconSpec, NS_LITERAL_STRING(".ico"),
                  getter_AddRefs(iconFile));
  if (!iconFile)
    return NS_OK; 

  nsAutoString iconPath;
  iconFile->GetPath(iconPath);

  

  ::SetLastError(0);

  HICON bigIcon = (HICON)::LoadImageW(NULL,
                                      (LPCWSTR)iconPath.get(),
                                      IMAGE_ICON,
                                      ::GetSystemMetrics(SM_CXICON),
                                      ::GetSystemMetrics(SM_CYICON),
                                      LR_LOADFROMFILE );
  HICON smallIcon = (HICON)::LoadImageW(NULL,
                                        (LPCWSTR)iconPath.get(),
                                        IMAGE_ICON,
                                        ::GetSystemMetrics(SM_CXSMICON),
                                        ::GetSystemMetrics(SM_CYSMICON),
                                        LR_LOADFROMFILE );

  if (bigIcon) {
    HICON icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)bigIcon);
    if (icon)
      ::DestroyIcon(icon);
  }
#ifdef DEBUG_SetIcon
  else {
    NS_LossyConvertUTF16toASCII cPath(iconPath);
    printf( "\nIcon load error; icon=%s, rc=0x%08X\n\n", cPath.get(), ::GetLastError() );
  }
#endif
  if (smallIcon) {
    HICON icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)smallIcon);
    if (icon)
      ::DestroyIcon(icon);
  }
#ifdef DEBUG_SetIcon
  else {
    NS_LossyConvertUTF16toASCII cPath(iconPath);
    printf( "\nSmall icon load error; icon=%s, rc=0x%08X\n\n", cPath.get(), ::GetLastError() );
  }
#endif
#endif 
  return NS_OK;
}


PRBool nsWindow::AutoErase()
{
  return PR_FALSE;
}


BOOL nsWindow::OnInputLangChange(HKL aHKL)
{
#ifdef KE_DEBUG
  printf("OnInputLanguageChange\n");
#endif

#ifndef WINCE
  gKbdLayout.LoadLayout(aHKL);
#endif

  return PR_FALSE;   
}

NS_IMETHODIMP nsWindow::ResetInputState()
{
#ifdef DEBUG_KBSTATE
  printf("ResetInputState\n");
#endif

#ifdef NS_ENABLE_TSF
  nsTextStore::CommitComposition(PR_FALSE);
#endif 

  nsIMEContext IMEContext(mWnd);
  if (IMEContext.IsValid()) {
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_COMPLETE, NULL);
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_CANCEL, NULL);
  }
  return NS_OK;
}


NS_IMETHODIMP nsWindow::SetIMEOpenState(PRBool aState)
{
#ifdef DEBUG_KBSTATE
  printf("SetIMEOpenState %s\n", (aState ? "Open" : "Close"));
#endif 

#ifdef NS_ENABLE_TSF
  nsTextStore::SetIMEOpenState(aState);
#endif 

  nsIMEContext IMEContext(mWnd);
  if (IMEContext.IsValid()) {
    ::ImmSetOpenStatus(IMEContext.get(), aState ? TRUE : FALSE);
  }
  return NS_OK;
}


NS_IMETHODIMP nsWindow::GetIMEOpenState(PRBool* aState)
{
  nsIMEContext IMEContext(mWnd);
  if (IMEContext.IsValid()) {
    BOOL isOpen = ::ImmGetOpenStatus(IMEContext.get());
    *aState = isOpen ? PR_TRUE : PR_FALSE;
  } else 
    *aState = PR_FALSE;

#ifdef NS_ENABLE_TSF
  *aState |= nsTextStore::GetIMEOpenState();
#endif 

  return NS_OK;
}


NS_IMETHODIMP nsWindow::SetIMEEnabled(PRUint32 aState)
{
#ifdef NS_ENABLE_TSF
  nsTextStore::SetIMEEnabled(aState);
#endif 
#ifdef DEBUG_KBSTATE
  printf("SetIMEEnabled: %s\n", (aState == nsIWidget::IME_STATUS_ENABLED ||
                                 aState == nsIWidget::IME_STATUS_PLUGIN)? 
                                "Enabled": "Disabled");
#endif 
  if (nsIMM32Handler::IsComposing(this))
    ResetInputState();
  mIMEEnabled = aState;
  PRBool enable = (aState == nsIWidget::IME_STATUS_ENABLED ||
                   aState == nsIWidget::IME_STATUS_PLUGIN);

#if defined(WINCE_HAVE_SOFTKB)
  gSoftKeyboardState = (aState != nsIWidget::IME_STATUS_DISABLED);
  ToggleSoftKB(gSoftKeyboardState);
#endif

  if (!enable != !mOldIMC)
    return NS_OK;
  mOldIMC = ::ImmAssociateContext(mWnd, enable ? mOldIMC : NULL);
  NS_ASSERTION(!enable || !mOldIMC, "Another IMC was associated");

  return NS_OK;
}


NS_IMETHODIMP nsWindow::GetIMEEnabled(PRUint32* aState)
{
#ifdef DEBUG_KBSTATE
  printf("GetIMEEnabled: %s\n", mIMEEnabled? "Enabled": "Disabled");
#endif 
  *aState = mIMEEnabled;
  return NS_OK;
}


NS_IMETHODIMP nsWindow::CancelIMEComposition()
{
#ifdef DEBUG_KBSTATE
  printf("CancelIMEComposition\n");
#endif 

#ifdef NS_ENABLE_TSF
  nsTextStore::CommitComposition(PR_TRUE);
#endif 

  nsIMEContext IMEContext(mWnd);
  if (IMEContext.IsValid()) {
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_CANCEL, NULL);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsWindow::GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState)
{
#ifdef DEBUG_KBSTATE
  printf("GetToggledKeyState\n");
#endif 
  NS_ENSURE_ARG_POINTER(aLEDState);
  *aLEDState = (::GetKeyState(aKeyCode) & 1) != 0;
  return NS_OK;
}

#ifdef NS_ENABLE_TSF
NS_IMETHODIMP
nsWindow::OnIMEFocusChange(PRBool aFocus)
{
  nsresult rv = nsTextStore::OnFocusChange(aFocus, this, mIMEEnabled);
  if (rv == NS_ERROR_NOT_AVAILABLE)
    rv = NS_OK; 
  return rv;
}

NS_IMETHODIMP
nsWindow::OnIMETextChange(PRUint32 aStart,
                          PRUint32 aOldEnd,
                          PRUint32 aNewEnd)
{
  return nsTextStore::OnTextChange(aStart, aOldEnd, aNewEnd);
}

NS_IMETHODIMP
nsWindow::OnIMESelectionChange(void)
{
  return nsTextStore::OnSelectionChange();
}
#endif 



NS_IMETHODIMP
nsWindow::GetAttention(PRInt32 aCycleCount)
{
#ifndef WINCE
  
  if (!mWnd)
    return NS_ERROR_NOT_INITIALIZED;

  
  
  HWND fgWnd = ::GetForegroundWindow();
  if (aCycleCount == 0 || fgWnd == GetTopLevelHWND(mWnd))
    return NS_OK;

  HWND flashWnd = mWnd;
  while (HWND ownerWnd = ::GetWindow(flashWnd, GW_OWNER)) {
    flashWnd = ownerWnd;
  }

  
  if (fgWnd == flashWnd)
    return NS_OK;

  DWORD defaultCycleCount = 0;
  ::SystemParametersInfo(SPI_GETFOREGROUNDFLASHCOUNT, 0, &defaultCycleCount, 0);

  FLASHWINFO flashInfo = { sizeof(FLASHWINFO), flashWnd,
    FLASHW_ALL, aCycleCount > 0 ? aCycleCount : defaultCycleCount, 0 };
  ::FlashWindowEx(&flashInfo);
#endif
  return NS_OK;
}

void nsWindow::StopFlashing()
{
#ifndef WINCE
  HWND flashWnd = mWnd;
  while (HWND ownerWnd = ::GetWindow(flashWnd, GW_OWNER)) {
    flashWnd = ownerWnd;
  }

  FLASHWINFO flashInfo = { sizeof(FLASHWINFO), flashWnd,
    FLASHW_STOP, 0, 0 };
  ::FlashWindowEx(&flashInfo);
#endif
}

PRBool
nsWindow::HasPendingInputEvent()
{
  
  
  
  
  
  WORD qstatus = HIWORD(GetQueueStatus(QS_INPUT));
  nsToolkit* toolkit = (nsToolkit *)mToolkit;
  return qstatus || (toolkit && toolkit->UserIsMovingWindow());
}









#ifdef DISPLAY_NOISY_MSGF_MSG
typedef struct {
  char * mStr;
  int    mId;
} MSGFEventMsgInfo;

MSGFEventMsgInfo gMSGFEvents[] = {
  "MSGF_DIALOGBOX",      0,
  "MSGF_MESSAGEBOX",     1,
  "MSGF_MENU",           2,
  "MSGF_SCROLLBAR",      5,
  "MSGF_NEXTWINDOW",     6,
  "MSGF_MAX",            8,
  "MSGF_USER",           4096,
  NULL, 0};

  void PrintEvent(UINT msg, PRBool aShowAllEvents, PRBool aShowMouseMoves);
  int gLastMsgCode = 0;

#define DISPLAY_NMM_PRT(_arg) printf((_arg));
#else
#define DISPLAY_NMM_PRT(_arg)
#endif



#ifndef WINCE


void nsWindow::ScheduleHookTimer(HWND aWnd, UINT aMsgId)
{
  
  
  if (gHookTimerId == 0) {
    
    gRollupMsgId = aMsgId;
    gRollupMsgWnd = aWnd;
    
    
    gHookTimerId = ::SetTimer(NULL, 0, 0, (TIMERPROC)HookTimerForPopups);
    NS_ASSERTION(gHookTimerId, "Timer couldn't be created.");
  }
}





LRESULT CALLBACK nsWindow::MozSpecialMsgFilter(int code, WPARAM wParam, LPARAM lParam)
{
#ifdef DISPLAY_NOISY_MSGF_MSG
  if (gProcessHook) {
    MSG* pMsg = (MSG*)lParam;

    int inx = 0;
    while (gMSGFEvents[inx].mId != code && gMSGFEvents[inx].mStr != NULL) {
      inx++;
    }
    if (code != gLastMsgCode) {
      if (gMSGFEvents[inx].mId == code) {
#ifdef DEBUG
        printf("MozSpecialMessageProc - code: 0x%X  - %s  hw: %p\n", code, gMSGFEvents[inx].mStr, pMsg->hwnd);
#endif
      } else {
#ifdef DEBUG
        printf("MozSpecialMessageProc - code: 0x%X  - %d  hw: %p\n", code, gMSGFEvents[inx].mId, pMsg->hwnd);
#endif
      }
      gLastMsgCode = code;
    }
    PrintEvent(pMsg->message, FALSE, FALSE);
  }
#endif

  if (gProcessHook && code == MSGF_MENU) {
    MSG* pMsg = (MSG*)lParam;
    ScheduleHookTimer( pMsg->hwnd, pMsg->message);
  }

  return ::CallNextHookEx(gMsgFilterHook, code, wParam, lParam);
}




LRESULT CALLBACK nsWindow::MozSpecialMouseProc(int code, WPARAM wParam, LPARAM lParam)
{
  if (gProcessHook) {
    switch (wParam) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL:
      {
        MOUSEHOOKSTRUCT* ms = (MOUSEHOOKSTRUCT*)lParam;
        nsIWidget* mozWin = (nsIWidget*)GetNSWindowPtr(ms->hwnd);
        if (mozWin) {
          
          
          if (static_cast<nsWindow*>(mozWin)->mIsPluginWindow)
            ScheduleHookTimer(ms->hwnd, (UINT)wParam);
        } else {
          ScheduleHookTimer(ms->hwnd, (UINT)wParam);
        }
        break;
      }
    }
  }
  return ::CallNextHookEx(gCallMouseHook, code, wParam, lParam);
}




LRESULT CALLBACK nsWindow::MozSpecialWndProc(int code, WPARAM wParam, LPARAM lParam)
{
#ifdef DISPLAY_NOISY_MSGF_MSG
  if (gProcessHook) {
    CWPSTRUCT* cwpt = (CWPSTRUCT*)lParam;
    PrintEvent(cwpt->message, FALSE, FALSE);
  }
#endif

  if (gProcessHook) {
    CWPSTRUCT* cwpt = (CWPSTRUCT*)lParam;
    if (cwpt->message == WM_MOVING ||
        cwpt->message == WM_SIZING ||
        cwpt->message == WM_GETMINMAXINFO) {
      ScheduleHookTimer(cwpt->hwnd, (UINT)cwpt->message);
    }
  }

  return ::CallNextHookEx(gCallProcHook, code, wParam, lParam);
}




void nsWindow::RegisterSpecialDropdownHooks()
{
  NS_ASSERTION(!gMsgFilterHook, "gMsgFilterHook must be NULL!");
  NS_ASSERTION(!gCallProcHook,  "gCallProcHook must be NULL!");

  DISPLAY_NMM_PRT("***************** Installing Msg Hooks ***************\n");

  

  
  if (!gMsgFilterHook) {
    DISPLAY_NMM_PRT("***** Hooking gMsgFilterHook!\n");
    gMsgFilterHook = SetWindowsHookEx(WH_MSGFILTER, MozSpecialMsgFilter, NULL, GetCurrentThreadId());
#ifdef DISPLAY_NOISY_MSGF_MSG
    if (!gMsgFilterHook) {
      printf("***** SetWindowsHookEx is NOT installed for WH_MSGFILTER!\n");
    }
#endif
  }

  
  if (!gCallProcHook) {
    DISPLAY_NMM_PRT("***** Hooking gCallProcHook!\n");
    gCallProcHook  = SetWindowsHookEx(WH_CALLWNDPROC, MozSpecialWndProc, NULL, GetCurrentThreadId());
#ifdef DISPLAY_NOISY_MSGF_MSG
    if (!gCallProcHook) {
      printf("***** SetWindowsHookEx is NOT installed for WH_CALLWNDPROC!\n");
    }
#endif
  }

  
  if (!gCallMouseHook) {
    DISPLAY_NMM_PRT("***** Hooking gCallMouseHook!\n");
    gCallMouseHook  = SetWindowsHookEx(WH_MOUSE, MozSpecialMouseProc, NULL, GetCurrentThreadId());
#ifdef DISPLAY_NOISY_MSGF_MSG
    if (!gCallMouseHook) {
      printf("***** SetWindowsHookEx is NOT installed for WH_MOUSE!\n");
    }
#endif
  }
}




void nsWindow::UnregisterSpecialDropdownHooks()
{
  DISPLAY_NMM_PRT("***************** De-installing Msg Hooks ***************\n");

  if (gCallProcHook) {
    DISPLAY_NMM_PRT("***** Unhooking gCallProcHook!\n");
    if (!::UnhookWindowsHookEx(gCallProcHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for gCallProcHook!\n");
    }
    gCallProcHook = NULL;
  }

  if (gMsgFilterHook) {
    DISPLAY_NMM_PRT("***** Unhooking gMsgFilterHook!\n");
    if (!::UnhookWindowsHookEx(gMsgFilterHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for gMsgFilterHook!\n");
    }
    gMsgFilterHook = NULL;
  }

  if (gCallMouseHook) {
    DISPLAY_NMM_PRT("***** Unhooking gCallMouseHook!\n");
    if (!::UnhookWindowsHookEx(gCallMouseHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for gCallMouseHook!\n");
    }
    gCallMouseHook = NULL;
  }
}











VOID CALLBACK nsWindow::HookTimerForPopups(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
  if (gHookTimerId != 0) {
    
    BOOL status = ::KillTimer(NULL, gHookTimerId);
    NS_ASSERTION(status, "Hook Timer was not killed.");
    gHookTimerId = 0;
  }

  if (gRollupMsgId != 0) {
    
    
    LRESULT popupHandlingResult;
    nsAutoRollup autoRollup;
    DealWithPopups(gRollupMsgWnd, gRollupMsgId, 0, 0, &popupHandlingResult);
    gRollupMsgId = 0;
    gRollupMsgWnd = NULL;
  }
}
#endif 

static PRBool IsDifferentThreadWindow(HWND aWnd)
{
  return ::GetCurrentThreadId() != ::GetWindowThreadProcessId(aWnd, NULL);
}







BOOL
nsWindow :: DealWithPopups ( HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam, LRESULT* outResult )
{
  if (gRollupListener && gRollupWidget && ::IsWindowVisible(inWnd)) {

    if (inMsg == WM_LBUTTONDOWN || inMsg == WM_RBUTTONDOWN || inMsg == WM_MBUTTONDOWN ||
        inMsg == WM_MOUSEWHEEL || inMsg == WM_MOUSEHWHEEL || inMsg == WM_ACTIVATE ||
        (inMsg == WM_KILLFOCUS && IsDifferentThreadWindow((HWND)inWParam))
#ifndef WINCE
        || 
        inMsg == WM_NCRBUTTONDOWN || 
        inMsg == WM_MOVING || 
        inMsg == WM_SIZING || 
        inMsg == WM_NCLBUTTONDOWN || 
        inMsg == WM_NCMBUTTONDOWN ||
        inMsg == WM_MOUSEACTIVATE ||
        inMsg == WM_ACTIVATEAPP ||
        inMsg == WM_MENUSELECT
#endif
        )
    {
      
      PRBool rollup = !nsWindow::EventIsInsideWindow(inMsg, (nsWindow*)gRollupWidget);

      if (rollup && (inMsg == WM_MOUSEWHEEL || inMsg == WM_MOUSEHWHEEL))
      {
        gRollupListener->ShouldRollupOnMouseWheelEvent(&rollup);
        *outResult = PR_TRUE;
      }

      
      
      PRUint32 popupsToRollup = PR_UINT32_MAX;
      if (rollup) {
        nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(gRollupListener) );
        if ( menuRollup ) {
          nsAutoTArray<nsIWidget*, 5> widgetChain;
          PRUint32 sameTypeCount = menuRollup->GetSubmenuWidgetChain(&widgetChain);
          for ( PRUint32 i = 0; i < widgetChain.Length(); ++i ) {
            nsIWidget* widget = widgetChain[i];
            if ( nsWindow::EventIsInsideWindow(inMsg, (nsWindow*)widget) ) {
              
              
              
              
              if (i < sameTypeCount) {
                rollup = PR_FALSE;
              }
              else {
                popupsToRollup = sameTypeCount;
              }
              break;
            }
          } 
        } 
      }

#ifndef WINCE
      if (inMsg == WM_MOUSEACTIVATE && popupsToRollup == PR_UINT32_MAX) {
        
        
        
        
        if (!rollup) {
          *outResult = MA_NOACTIVATE;
          return TRUE;
        }
        else
        {
          UINT uMsg = HIWORD(inLParam);
          if (uMsg == WM_MOUSEMOVE)
          {
            
            
            gRollupListener->ShouldRollupOnMouseActivate(&rollup);
            if (!rollup)
            {
              *outResult = MA_NOACTIVATE;
              return true;
            }
          }
        }
      }
      
      else
#endif
      if ( rollup ) {
        
        
        PRBool consumeRollupEvent = gRollupConsumeRollupEvent;
        
        gRollupListener->Rollup(popupsToRollup, inMsg == WM_LBUTTONDOWN ? &mLastRollup : nsnull);

        
        gProcessHook = PR_FALSE;
        gRollupMsgId = 0;
        gRollupMsgWnd = NULL;

        
        
        
        
        if (consumeRollupEvent && inMsg != WM_RBUTTONDOWN) {
          *outResult = TRUE;
          return TRUE;
        }
#ifndef WINCE
        
        
        
        if (popupsToRollup != PR_UINT32_MAX && inMsg == WM_MOUSEACTIVATE) {
          *outResult = MA_NOACTIVATEANDEAT;
          return TRUE;
        }
#endif
      }
    } 
  } 

  return FALSE;
} 



#ifdef ACCESSIBILITY
already_AddRefed<nsIAccessible> nsWindow::GetRootAccessible()
{
  nsWindow::gIsAccessibilityOn = TRUE;

  if (mIsDestroying || mOnDestroyCalled || mWindowType == eWindowType_invisible) {
    return nsnull;
  }

  nsIAccessible *rootAccessible = nsnull;

  
  
  
  nsWindow* accessibleWindow = nsnull;
  if (mContentType != eContentTypeInherit) {
    
    
    
    HWND accessibleWnd = ::GetTopWindow(mWnd);
    while (accessibleWnd) {
      
      accessibleWindow = GetNSWindowPtr(accessibleWnd);
      if (accessibleWindow) {
        accessibleWindow->DispatchAccessibleEvent(NS_GETACCESSIBLE, &rootAccessible);
        if (rootAccessible) {
          break;  
        }
      }
      accessibleWnd = ::GetNextWindow(accessibleWnd, GW_HWNDNEXT);
    }
  }
  else {
    DispatchAccessibleEvent(NS_GETACCESSIBLE, &rootAccessible);
  }
  return rootAccessible;
}

HINSTANCE nsWindow::gmAccLib = 0;
LPFNLRESULTFROMOBJECT nsWindow::gmLresultFromObject = 0;

STDMETHODIMP_(LRESULT) nsWindow::LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN pAcc)
{
  
  if (!gmAccLib)
    gmAccLib =::LoadLibraryW(L"OLEACC.DLL");

  if (gmAccLib) {
    if (!gmLresultFromObject)
      gmLresultFromObject = (LPFNLRESULTFROMOBJECT)GetProcAddress(gmAccLib,"LresultFromObject");

    if (gmLresultFromObject)
      return gmLresultFromObject(riid,wParam,pAcc);
  }

  return 0;
}
#endif

#ifdef MOZ_XUL

nsWindow* nsWindow::GetTopLevelWindow(PRBool aStopOnDialogOrPopup)
{
  nsWindow* curWindow = this;

  while (PR_TRUE) {
    if (aStopOnDialogOrPopup) {
      switch (curWindow->mWindowType) {
        case eWindowType_dialog:
        case eWindowType_popup:
          return curWindow;
      }
    }

    
    nsWindow* parentWindow = curWindow->GetParentWindow(PR_TRUE);

    if (!parentWindow)
      return curWindow;

    curWindow = parentWindow;
  }
}

gfxASurface *nsWindow::GetThebesSurface()
{
  if (mPaintDC)
    return (new gfxWindowsSurface(mPaintDC));

  return (new gfxWindowsSurface(mWnd));
}

void nsWindow::ResizeTranslucentWindow(PRInt32 aNewWidth, PRInt32 aNewHeight, PRBool force)
{
  if (!force && aNewWidth == mBounds.width && aNewHeight == mBounds.height)
    return;

  mTransparentSurface = new gfxWindowsSurface(gfxIntSize(aNewWidth, aNewHeight), gfxASurface::ImageFormatARGB32);
  mMemoryDC = mTransparentSurface->GetDC();
}

nsTransparencyMode nsWindow::GetTransparencyMode()
{
  return GetTopLevelWindow(PR_TRUE)->GetWindowTranslucencyInner();
}

void nsWindow::SetTransparencyMode(nsTransparencyMode aMode)
{
  GetTopLevelWindow(PR_TRUE)->SetWindowTranslucencyInner(aMode);
}

void nsWindow::SetWindowTranslucencyInner(nsTransparencyMode aMode)
{
#ifndef WINCE

  if (aMode == mTransparencyMode)
    return;

  HWND hWnd = GetTopLevelHWND(mWnd, PR_TRUE);
  nsWindow* topWindow = GetNSWindowPtr(hWnd);

  if (!topWindow)
  {
    NS_WARNING("Trying to use transparent chrome in an embedded context");
    return;
  }

  LONG_PTR style = 0, exStyle = 0;
  switch(aMode) {
    case eTransparencyTransparent:
      exStyle |= WS_EX_LAYERED;
    case eTransparencyOpaque:
    case eTransparencyGlass:
      topWindow->mTransparencyMode = aMode;
      break;
  }

  style |= topWindow->WindowStyle();
  exStyle |= topWindow->WindowExStyle();

  if (aMode == eTransparencyTransparent) {
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
  }

  VERIFY_WINDOW_STYLE(style);
  ::SetWindowLongPtrW(hWnd, GWL_STYLE, style);
  ::SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle);

  mTransparencyMode = aMode;

  SetupTranslucentWindowMemoryBitmap(aMode);
  MARGINS margins = { 0, 0, 0, 0 };
  if(eTransparencyGlass == aMode)
    margins.cxLeftWidth = -1;
  if(nsUXThemeData::sHaveCompositor)
    nsUXThemeData::dwmExtendFrameIntoClientAreaPtr(hWnd, &margins);
#endif
}

void nsWindow::SetupTranslucentWindowMemoryBitmap(nsTransparencyMode aMode)
{
  if (eTransparencyTransparent == aMode) {
    ResizeTranslucentWindow(mBounds.width, mBounds.height, PR_TRUE);
  } else {
    mTransparentSurface = nsnull;
    mMemoryDC = NULL;
  }
}

nsresult nsWindow::UpdateTranslucentWindow()
{
#ifndef WINCE
  if (mBounds.IsEmpty())
    return NS_OK;

  ::GdiFlush();

  BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
  SIZE winSize = { mBounds.width, mBounds.height };
  POINT srcPos = { 0, 0 };
  HWND hWnd = GetTopLevelHWND(mWnd, PR_TRUE);
  RECT winRect;
  ::GetWindowRect(hWnd, &winRect);

  
  if (!::UpdateLayeredWindow(hWnd, NULL, (POINT*)&winRect, &winSize, mMemoryDC, &srcPos, 0, &bf, ULW_ALPHA))
    return NS_ERROR_FAILURE;
#endif

  return NS_OK;
}

#endif 

























































#include "mozilla/MathAlgorithms.h"
#include "mozilla/Util.h"

#include "mozilla/ipc/RPCChannel.h"
#include <algorithm>

#include "nsWindow.h"

#include <shellapi.h>
#include <windows.h>
#include <process.h>
#include <commctrl.h>
#include <unknwn.h>
#include <psapi.h>

#include "prlog.h"
#include "prtime.h"
#include "prprf.h"
#include "prmem.h"

#include "mozilla/WidgetTraceEvent.h"
#include "nsIAppShell.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMMouseEvent.h"
#include "nsITheme.h"
#include "nsIObserverService.h"
#include "nsIScreenManager.h"
#include "imgIContainer.h"
#include "nsIFile.h"
#include "nsIRollupListener.h"
#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsIMM32Handler.h"
#include "WinMouseScrollHandler.h"
#include "nsFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsGUIEvent.h"
#include "nsFont.h"
#include "nsRect.h"
#include "nsThreadUtils.h"
#include "nsNativeCharsetUtils.h"
#include "nsGkAtoms.h"
#include "nsCRT.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsWidgetsCID.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsString.h"
#include "mozilla/Services.h"
#include "nsNativeThemeWin.h"
#include "nsWindowsDllInterceptor.h"
#include "nsIWindowMediator.h"
#include "nsIServiceManager.h"
#include "nsWindowGfx.h"
#include "gfxWindowsPlatform.h"
#include "Layers.h"
#include "nsPrintfCString.h"
#include "mozilla/Preferences.h"
#include "nsISound.h"
#include "WinTaskbar.h"
#include "WinUtils.h"
#include "WidgetUtils.h"
#include "nsIWidgetListener.h"
#include "nsDOMTouchEvent.h"

#ifdef MOZ_ENABLE_D3D9_LAYER
#include "LayerManagerD3D9.h"
#endif

#ifdef MOZ_ENABLE_D3D10_LAYER
#include "LayerManagerD3D10.h"
#endif

#include "LayerManagerOGL.h"
#include "nsIGfxInfo.h"
#include "BasicLayers.h"
#include "nsUXThemeConstants.h"
#include "KeyboardLayout.h"
#include "nsNativeDragTarget.h"
#include <mmsystem.h> 
#include <zmouse.h>
#include <richedit.h>

#if defined(ACCESSIBILITY)
#include "oleidl.h"
#include <winuser.h>
#include "nsAccessibilityService.h"
#include "mozilla/a11y/Platform.h"
#if !defined(WINABLEAPI)
#include <winable.h>
#endif 
#endif 

#include "nsIWinTaskbar.h"
#define NS_TASKBAR_CONTRACTID "@mozilla.org/windows-taskbar;1"


#include "npapi.h"

#include "nsWindowDefs.h"

#include "nsCrashOnException.h"
#include "nsIXULRuntime.h"

#include "nsIContent.h"

#include "mozilla/HangMonitor.h"
#include "WinIMEHandler.h"

using namespace mozilla::widget;
using namespace mozilla::layers;
using namespace mozilla;

















bool            nsWindow::sDropShadowEnabled      = true;
uint32_t        nsWindow::sInstanceCount          = 0;
bool            nsWindow::sSwitchKeyboardLayout   = false;
BOOL            nsWindow::sIsOleInitialized       = FALSE;
HCURSOR         nsWindow::sHCursor                = NULL;
imgIContainer*  nsWindow::sCursorImgContainer     = nullptr;
nsWindow*       nsWindow::sCurrentWindow          = nullptr;
bool            nsWindow::sJustGotDeactivate      = false;
bool            nsWindow::sJustGotActivate        = false;
bool            nsWindow::sIsInMouseCapture       = false;


TriStateBool    nsWindow::sCanQuit                = TRI_UNKNOWN;




HHOOK           nsWindow::sMsgFilterHook          = NULL;
HHOOK           nsWindow::sCallProcHook           = NULL;
HHOOK           nsWindow::sCallMouseHook          = NULL;
bool            nsWindow::sProcessHook            = false;
UINT            nsWindow::sRollupMsgId            = 0;
HWND            nsWindow::sRollupMsgWnd           = NULL;
UINT            nsWindow::sHookTimerId            = 0;



POINT           nsWindow::sLastMousePoint         = {0};
POINT           nsWindow::sLastMouseMovePoint     = {0};
LONG            nsWindow::sLastMouseDownTime      = 0L;
LONG            nsWindow::sLastClickCount         = 0L;
BYTE            nsWindow::sLastMouseButton        = 0;


int             nsWindow::sTrimOnMinimize         = 2;


const char*     nsWindow::sDefaultMainWindowClass = kClassNameGeneral;


bool            nsWindow::sAllowD3D9              = false;

TriStateBool nsWindow::sHasBogusPopupsDropShadowOnMultiMonitor = TRI_UNKNOWN;


const PRUnichar* kOOPPPluginFocusEventId   = L"OOPP Plugin Focus Widget Event";
uint32_t        nsWindow::sOOPPPluginFocusEvent   =
                  RegisterWindowMessageW(kOOPPPluginFocusEventId);

MSG             nsWindow::sRedirectedKeyDown;







static const char *sScreenManagerContractID       = "@mozilla.org/gfx/screenmanager;1";

#ifdef PR_LOGGING
PRLogModuleInfo* gWindowsLog                      = nullptr;
#endif


static KeyboardLayout gKbdLayout;


static bool     gWindowsVisible                   = false;


static bool     gIsSleepMode                      = false;

static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);


static WindowsDllInterceptor sUser32Intercept;




static const int32_t kGlassMarginAdjustment = 2;





static const int32_t kResizableBorderMinSize = 3;






#define MAX_ACCELERATED_DIMENSION 8192



















nsWindow::nsWindow() : nsWindowBase()
{
#ifdef PR_LOGGING
  if (!gWindowsLog) {
    gWindowsLog = PR_NewLogModule("nsWindow");
  }
#endif

  mIconSmall            = nullptr;
  mIconBig              = nullptr;
  mWnd                  = nullptr;
  mPaintDC              = nullptr;
  mPrevWndProc          = nullptr;
  mNativeDragTarget     = nullptr;
  mInDtor               = false;
  mIsVisible            = false;
  mIsTopWidgetWindow    = false;
  mUnicodeWidget        = true;
  mDisplayPanFeedback   = false;
  mTouchWindow          = false;
  mCustomNonClient      = false;
  mHideChrome           = false;
  mFullscreenMode       = false;
  mMousePresent         = false;
  mDestroyCalled        = false;
  mPickerDisplayCount   = 0;
  mWindowType           = eWindowType_child;
  mBorderStyle          = eBorderStyle_default;
  mOldSizeMode          = nsSizeMode_Normal;
  mLastSizeMode         = nsSizeMode_Normal;
  mLastPoint.x          = 0;
  mLastPoint.y          = 0;
  mLastSize.width       = 0;
  mLastSize.height      = 0;
  mOldStyle             = 0;
  mOldExStyle           = 0;
  mPainting             = 0;
  mLastKeyboardLayout   = 0;
  mBlurSuppressLevel    = 0;
  mLastPaintEndTime     = TimeStamp::Now();
#ifdef MOZ_XUL
  mTransparentSurface   = nullptr;
  mMemoryDC             = nullptr;
  mTransparencyMode     = eTransparencyOpaque;
  memset(&mGlassMargins, 0, sizeof mGlassMargins);
#endif
  mBackground           = ::GetSysColor(COLOR_BTNFACE);
  mBrush                = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
  mForeground           = ::GetSysColor(COLOR_WINDOWTEXT);

  mTaskbarPreview = nullptr;
  mHasTaskbarIconBeenCreated = false;

  
  if (!sInstanceCount) {
    
    
    mozilla::widget::WinTaskbar::RegisterAppUserModelID();
    gKbdLayout.LoadLayout(::GetKeyboardLayout(0));
    IMEHandler::Initialize();
    if (SUCCEEDED(::OleInitialize(NULL))) {
      sIsOleInitialized = TRUE;
    }
    NS_ASSERTION(sIsOleInitialized, "***** OLE is not initialized!\n");
    MouseScrollHandler::Initialize();
    
    nsUXThemeData::InitTitlebarInfo();
    
    nsUXThemeData::UpdateNativeThemeInfo();
    ForgetRedirectedKeyDownMessage();
  } 

  mIdleService = nullptr;

  sInstanceCount++;
}

nsWindow::~nsWindow()
{
  mInDtor = true;

  
  
  
  
  if (NULL != mWnd)
    Destroy();

  
  if (mIconSmall)
    ::DestroyIcon(mIconSmall);

  if (mIconBig)
    ::DestroyIcon(mIconBig);

  sInstanceCount--;

  
  if (sInstanceCount == 0) {
    IMEHandler::Terminate();
    NS_IF_RELEASE(sCursorImgContainer);
    if (sIsOleInitialized) {
      ::OleFlushClipboard();
      ::OleUninitialize();
      sIsOleInitialized = FALSE;
    }
  }

  NS_IF_RELEASE(mNativeDragTarget);
}

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)











int32_t nsWindow::GetHeight(int32_t aProposedHeight)
{
  return aProposedHeight;
}


nsresult
nsWindow::Create(nsIWidget *aParent,
                 nsNativeWidget aNativeParent,
                 const nsIntRect &aRect,
                 nsDeviceContext *aContext,
                 nsWidgetInitData *aInitData)
{
  nsWidgetInitData defaultInitData;
  if (!aInitData)
    aInitData = &defaultInitData;

  mUnicodeWidget = aInitData->mUnicode;

  nsIWidget *baseParent = aInitData->mWindowType == eWindowType_dialog ||
                          aInitData->mWindowType == eWindowType_toplevel ||
                          aInitData->mWindowType == eWindowType_invisible ?
                          nullptr : aParent;

  mIsTopWidgetWindow = (nullptr == baseParent);
  mBounds = aRect;

  
  nsToolkit::GetToolkit();

  BaseCreate(baseParent, aRect, aContext, aInitData);

  HWND parent;
  if (aParent) { 
    parent = aParent ? (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW) : NULL;
    mParent = aParent;
  } else { 
    parent = (HWND)aNativeParent;
    mParent = aNativeParent ?
      WinUtils::GetNSWindowPtr((HWND)aNativeParent) : nullptr;
  }

  mIsRTL = aInitData->mRTL;

  DWORD style = WindowStyle();
  DWORD extendedStyle = WindowExStyle();

  if (mWindowType == eWindowType_popup) {
    if (!aParent) {
      parent = NULL;
    }

    if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
        WinUtils::GetWindowsVersion() <= WinUtils::WIN7_VERSION) {
      extendedStyle |= WS_EX_COMPOSITED;
    }

    if (aInitData->mIsDragPopup) {
      
      extendedStyle |= WS_EX_TRANSPARENT;
    }
  } else if (mWindowType == eWindowType_invisible) {
    
    style &= ~0x40000000; 
  } else {
    
    if (aInitData->clipChildren) {
      style |= WS_CLIPCHILDREN;
    } else {
      style &= ~WS_CLIPCHILDREN;
    }
    if (aInitData->clipSiblings) {
      style |= WS_CLIPSIBLINGS;
    }
  }

  nsAutoString className;
  if (aInitData->mDropShadow) {
    GetWindowPopupClass(className);
  } else {
    GetWindowClass(className);
  }
  
  
  
  if(aInitData->mWindowType == eWindowType_plugin) {
    style |= WS_DISABLED;
  }
  mWnd = ::CreateWindowExW(extendedStyle,
                           className.get(),
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

  if (!mWnd) {
    NS_WARNING("nsWindow CreateWindowEx failed.");
    return NS_ERROR_FAILURE;
  }

  if (mIsRTL && nsUXThemeData::dwmSetWindowAttributePtr) {
    DWORD dwAttribute = TRUE;    
    nsUXThemeData::dwmSetWindowAttributePtr(mWnd, DWMWA_NONCLIENT_RTL_LAYOUT, &dwAttribute, sizeof dwAttribute);
  }

  if (mWindowType != eWindowType_plugin &&
      mWindowType != eWindowType_invisible &&
      MouseScrollHandler::Device::IsFakeScrollableWindowNeeded()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    HWND scrollContainerWnd = ::CreateWindowW
      (className.get(), L"FAKETRACKPOINTSCROLLCONTAINER",
       WS_CHILD | WS_VISIBLE,
       0, 0, 0, 0, mWnd, NULL, nsToolkit::mDllInstance, NULL);
    HWND scrollableWnd = ::CreateWindowW
      (className.get(), L"FAKETRACKPOINTSCROLLABLE",
       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | 0x30,
       0, 0, 0, 0, scrollContainerWnd, NULL, nsToolkit::mDllInstance, NULL);

    
    
    
    ::SetWindowLongPtrW(scrollableWnd, GWLP_ID, eFakeTrackPointScrollableID);

    
    
    WNDPROC oldWndProc;
    if (mUnicodeWidget)
      oldWndProc = (WNDPROC)::SetWindowLongPtrW(scrollableWnd, GWLP_WNDPROC,
                                                (LONG_PTR)nsWindow::WindowProc);
    else
      oldWndProc = (WNDPROC)::SetWindowLongPtrA(scrollableWnd, GWLP_WNDPROC,
                                                (LONG_PTR)nsWindow::WindowProc);
    ::SetWindowLongPtrW(scrollableWnd, GWLP_USERDATA, (LONG_PTR)oldWndProc);
  }

  SubclassWindow(TRUE);

  IMEHandler::InitInputContext(this, mInputContext);

  
  
  
  
  if (sTrimOnMinimize == 2 && mWindowType == eWindowType_invisible) {
    
    
    
    
    sTrimOnMinimize =
      Preferences::GetBool("config.trim_on_minimize",
        (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION)) ? 1 : 0;
    sSwitchKeyboardLayout =
      Preferences::GetBool("intl.keyboard.per_window_layout", false);
  }

  return NS_OK;
}


NS_METHOD nsWindow::Destroy()
{
  
  if (mOnDestroyCalled)
    return NS_OK;

  
  
  mDestroyCalled = true;
  if (mPickerDisplayCount)
    return NS_OK;

  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);

  



  if (mLayerManager) {
    mLayerManager->Destroy();
  }
  mLayerManager = nullptr;

  

  ClearCachedResources();

  
  
  
  
  
  
  
  
  
  
  VERIFY(::DestroyWindow(mWnd));
  
  
  
  if (false == mOnDestroyCalled) {
    LRESULT result;
    mWindowHook.Notify(mWnd, WM_DESTROY, 0, 0, &result);
    OnDestroy();
  }

  return NS_OK;
}










void nsWindow::RegisterWindowClass(const nsString& aClassName, UINT aExtraStyle,
                                   LPWSTR aIconID)
{
  WNDCLASSW wc;
  if (::GetClassInfoW(nsToolkit::mDllInstance, aClassName.get(), &wc)) {
    
    return;
  }

  wc.style         = CS_DBLCLKS | aExtraStyle;
  wc.lpfnWndProc   = ::DefWindowProcW;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = nsToolkit::mDllInstance;
  wc.hIcon         = aIconID ? ::LoadIconW(::GetModuleHandleW(NULL), aIconID) : NULL;
  wc.hCursor       = NULL;
  wc.hbrBackground = mBrush;
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = aClassName.get();

  if (!::RegisterClassW(&wc)) {
    
    
    wc.style = CS_DBLCLKS;
    ::RegisterClassW(&wc);
  }
}

static LPWSTR const gStockApplicationIcon = MAKEINTRESOURCEW(32512);


void nsWindow::GetWindowClass(nsString& aWindowClass)
{
  switch (mWindowType) {
  case eWindowType_invisible:
    aWindowClass.AssignLiteral(kClassNameHidden);
    RegisterWindowClass(aWindowClass, 0, gStockApplicationIcon);
    break;
  case eWindowType_dialog:
    aWindowClass.AssignLiteral(kClassNameDialog);
    RegisterWindowClass(aWindowClass, 0, 0);
    break;
  default:
    GetMainWindowClass(aWindowClass);
    RegisterWindowClass(aWindowClass, 0, gStockApplicationIcon);
    break;
  }
}


void nsWindow::GetWindowPopupClass(nsString& aWindowClass)
{
  aWindowClass.AssignLiteral(kClassNameDropShadow);
  RegisterWindowClass(aWindowClass, CS_XP_DROPSHADOW, gStockApplicationIcon);
}










DWORD nsWindow::WindowStyle()
{
  DWORD style;

  switch (mWindowType) {
    case eWindowType_plugin:
    case eWindowType_child:
      style = WS_OVERLAPPED;
      break;

    case eWindowType_dialog:
      style = WS_OVERLAPPED | WS_BORDER | WS_DLGFRAME | WS_SYSMENU | DS_3DLOOK |
              DS_MODALFRAME | WS_CLIPCHILDREN;
      if (mBorderStyle != eBorderStyle_default)
        style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
      break;

    case eWindowType_popup:
      style = WS_POPUP;
      if (!HasGlass()) {
        style |= WS_OVERLAPPED;
      }
      break;

    default:
      NS_ERROR("unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      style = WS_OVERLAPPED | WS_BORDER | WS_DLGFRAME | WS_SYSMENU |
              WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN;
      break;
  }

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

    if (IsPopupWithTitleBar()) {
      style |= WS_CAPTION;
      if (mBorderStyle & eBorderStyle_close) {
        style |= WS_SYSMENU;
      }
    }
  }

  VERIFY_WINDOW_STYLE(style);
  return style;
}


DWORD nsWindow::WindowExStyle()
{
  switch (mWindowType)
  {
    case eWindowType_plugin:
    case eWindowType_child:
      return 0;

    case eWindowType_dialog:
      return WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;

    case eWindowType_popup:
    {
      DWORD extendedStyle = WS_EX_TOOLWINDOW;
      if (mPopupLevel == ePopupLevelTop)
        extendedStyle |= WS_EX_TOPMOST;
      return extendedStyle;
    }
    default:
      NS_ERROR("unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      return WS_EX_WINDOWEDGE;
  }
}











void nsWindow::SubclassWindow(BOOL bState)
{
  if (bState) {
    if (!mWnd || !IsWindow(mWnd)) {
      NS_ERROR("Invalid window handle");
    }

    if (mUnicodeWidget) {
      mPrevWndProc =
        reinterpret_cast<WNDPROC>(
          SetWindowLongPtrW(mWnd,
                            GWLP_WNDPROC,
                            reinterpret_cast<LONG_PTR>(nsWindow::WindowProc)));
    } else {
      mPrevWndProc =
        reinterpret_cast<WNDPROC>(
          SetWindowLongPtrA(mWnd,
                            GWLP_WNDPROC,
                            reinterpret_cast<LONG_PTR>(nsWindow::WindowProc)));
    }
    NS_ASSERTION(mPrevWndProc, "Null standard window procedure");
    
    WinUtils::SetNSWindowPtr(mWnd, this);
  } else {
    if (IsWindow(mWnd)) {
      if (mUnicodeWidget) {
        SetWindowLongPtrW(mWnd,
                          GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(mPrevWndProc));
      } else {
        SetWindowLongPtrA(mWnd,
                          GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(mPrevWndProc));
      }
    }
    WinUtils::SetNSWindowPtr(mWnd, NULL);
    mPrevWndProc = NULL;
  }
}











NS_IMETHODIMP nsWindow::SetParent(nsIWidget *aNewParent)
{
  mParent = aNewParent;

  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  nsIWidget* parent = GetParent();
  if (parent) {
    parent->RemoveChild(this);
  }
  if (aNewParent) {
    ReparentNativeWidget(aNewParent);
    aNewParent->AddChild(this);
    return NS_OK;
  }
  if (mWnd) {
    
    VERIFY(::SetParent(mWnd, nullptr));
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::ReparentNativeWidget(nsIWidget* aNewParent)
{
  NS_PRECONDITION(aNewParent, "");

  mParent = aNewParent;
  if (mWindowType == eWindowType_popup) {
    return NS_OK;
  }
  HWND newParent = (HWND)aNewParent->GetNativeData(NS_NATIVE_WINDOW);
  NS_ASSERTION(newParent, "Parent widget has a null native window handle");
  if (newParent && mWnd) {
    ::SetParent(mWnd, newParent);
  }
  return NS_OK;
}

nsIWidget* nsWindow::GetParent(void)
{
  return GetParentWindow(false);
}

float nsWindow::GetDPI()
{
  HDC dc = ::GetDC(mWnd);
  if (!dc)
    return 96.0f;

  double heightInches = ::GetDeviceCaps(dc, VERTSIZE)/MM_PER_INCH_FLOAT;
  int heightPx = ::GetDeviceCaps(dc, VERTRES);
  ::ReleaseDC(mWnd, dc);
  if (heightInches < 0.25) {
    
    return 96.0f;
  }
  return float(heightPx/heightInches);
}

double nsWindow::GetDefaultScaleInternal()
{
  HDC dc = ::GetDC(mWnd);
  if (!dc)
    return 1.0;

  
  
  double pixelsPerInch = ::GetDeviceCaps(dc, LOGPIXELSY);
  ::ReleaseDC(mWnd, dc);
  return pixelsPerInch/96.0;
}

nsWindow* nsWindow::GetParentWindow(bool aIncludeOwner)
{
  if (mIsTopWidgetWindow) {
    
    
    
    return nullptr;
  }

  
  
  
  if (mInDtor || mOnDestroyCalled)
    return nullptr;


  
  
  
  nsWindow* widget = nullptr;
  if (mWnd) {
    HWND parent = nullptr;
    if (aIncludeOwner)
      parent = ::GetParent(mWnd);
    else
      parent = ::GetAncestor(mWnd, GA_PARENT);

    if (parent) {
      widget = WinUtils::GetNSWindowPtr(parent);
      if (widget) {
        
        
        if (widget->mInDtor) {
          widget = nullptr;
        }
      }
    }
  }

  return widget;
}
 
BOOL CALLBACK
nsWindow::EnumAllChildWindProc(HWND aWnd, LPARAM aParam)
{
  nsWindow *wnd = WinUtils::GetNSWindowPtr(aWnd);
  if (wnd) {
    ((nsWindow::WindowEnumCallback*)aParam)(wnd);
  }
  return TRUE;
}

BOOL CALLBACK
nsWindow::EnumAllThreadWindowProc(HWND aWnd, LPARAM aParam)
{
  nsWindow *wnd = WinUtils::GetNSWindowPtr(aWnd);
  if (wnd) {
    ((nsWindow::WindowEnumCallback*)aParam)(wnd);
  }
  EnumChildWindows(aWnd, EnumAllChildWindProc, aParam);
  return TRUE;
}

void
nsWindow::EnumAllWindows(WindowEnumCallback aCallback)
{
  EnumThreadWindows(GetCurrentThreadId(),
                    EnumAllThreadWindowProc,
                    (LPARAM)aCallback);
}









NS_METHOD nsWindow::Show(bool bState)
{
  if (mWindowType == eWindowType_popup) {
    
    
    
    
    
    if (HasBogusPopupsDropShadowOnMultiMonitor() &&
        WinUtils::GetMonitorCount() > 1 &&
        !nsUXThemeData::CheckForCompositor())
    {
      if (sDropShadowEnabled) {
        ::SetClassLongA(mWnd, GCL_STYLE, 0);
        sDropShadowEnabled = false;
      }
    } else {
      if (!sDropShadowEnabled) {
        ::SetClassLongA(mWnd, GCL_STYLE, CS_DROPSHADOW);
        sDropShadowEnabled = true;
      }
    }

    
    
    LONG_PTR exStyle = ::GetWindowLongPtrW(mWnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_LAYERED) {
      ::SetWindowLongPtrW(mWnd, GWL_EXSTYLE, exStyle & ~WS_EX_COMPOSITED);
    }
  }

  bool syncInvalidate = false;

  bool wasVisible = mIsVisible;
  
  
  mIsVisible = bState;

  
  
  if (mIsVisible)
    mOldStyle |= WS_VISIBLE;
  else
    mOldStyle &= ~WS_VISIBLE;

  if (!mIsVisible && wasVisible) {
      ClearCachedResources();
  }

  if (mWnd) {
    if (bState) {
      if (!wasVisible && mWindowType == eWindowType_toplevel) {
        
        
        syncInvalidate = true;
        switch (mSizeMode) {
          case nsSizeMode_Fullscreen:
            ::ShowWindow(mWnd, SW_SHOW);
            break;
          case nsSizeMode_Maximized :
            ::ShowWindow(mWnd, SW_SHOWMAXIMIZED);
            break;
          case nsSizeMode_Minimized :
            ::ShowWindow(mWnd, SW_SHOWMINIMIZED);
            break;
          default:
            if (CanTakeFocus()) {
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
          
          
          
          
          flags |= SWP_NOACTIVATE;
          HWND owner = ::GetWindow(mWnd, GW_OWNER);
          ::SetWindowPos(mWnd, owner ? 0 : HWND_TOPMOST, 0, 0, 0, 0, flags);
        } else {
          if (mWindowType == eWindowType_dialog && !CanTakeFocus())
            flags |= SWP_NOACTIVATE;

          ::SetWindowPos(mWnd, HWND_TOP, 0, 0, 0, 0, flags);
        }
      }

      if (!wasVisible && (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog)) {
        
        ::SendMessageW(mWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
      }
    } else {
      
      
      if (wasVisible && mTransparencyMode == eTransparencyTransparent) {
        ClearTranslucentWindow();
      }
      if (mWindowType != eWindowType_dialog) {
        ::ShowWindow(mWnd, SW_HIDE);
      } else {
        ::SetWindowPos(mWnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE |
                       SWP_NOZORDER | SWP_NOACTIVATE);
      }
    }
  }
  
#ifdef MOZ_XUL
  if (!wasVisible && bState) {
    Invalidate();
    if (syncInvalidate && !mInDtor && !mOnDestroyCalled) {
      ::UpdateWindow(mWnd);
    }
  }
#endif

  return NS_OK;
}










bool nsWindow::IsVisible() const
{
  return mIsVisible;
}












void nsWindow::ClearThemeRegion()
{
  if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
      !HasGlass() &&
      (mWindowType == eWindowType_popup && !IsPopupWithTitleBar() &&
       (mPopupType == ePopupTypeTooltip || mPopupType == ePopupTypePanel))) {
    SetWindowRgn(mWnd, NULL, false);
  }
}

void nsWindow::SetThemeRegion()
{
  
  
  
  
  
  if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
      !HasGlass() &&
      (mWindowType == eWindowType_popup && !IsPopupWithTitleBar() &&
       (mPopupType == ePopupTypeTooltip || mPopupType == ePopupTypePanel))) {
    HRGN hRgn = nullptr;
    RECT rect = {0,0,mBounds.width,mBounds.height};
    
    HDC dc = ::GetDC(mWnd);
    GetThemeBackgroundRegion(nsUXThemeData::GetTheme(eUXTooltip), dc, TTP_STANDARD, TS_NORMAL, &rect, &hRgn);
    if (hRgn) {
      if (!SetWindowRgn(mWnd, hRgn, false)) 
        DeleteObject(hRgn);
    }
    ::ReleaseDC(mWnd, dc);
  }
}










NS_METHOD nsWindow::RegisterTouchWindow() {
  mTouchWindow = true;
  mGesture.RegisterTouchWindow(mWnd);
  ::EnumChildWindows(mWnd, nsWindow::RegisterTouchForDescendants, 0);
  return NS_OK;
}

NS_METHOD nsWindow::UnregisterTouchWindow() {
  mTouchWindow = false;
  mGesture.UnregisterTouchWindow(mWnd);
  ::EnumChildWindows(mWnd, nsWindow::UnregisterTouchForDescendants, 0);
  return NS_OK;
}

BOOL CALLBACK nsWindow::RegisterTouchForDescendants(HWND aWnd, LPARAM aMsg) {
  nsWindow* win = WinUtils::GetNSWindowPtr(aWnd);
  if (win)
    win->mGesture.RegisterTouchWindow(aWnd);
  return TRUE;
}

BOOL CALLBACK nsWindow::UnregisterTouchForDescendants(HWND aWnd, LPARAM aMsg) {
  nsWindow* win = WinUtils::GetNSWindowPtr(aWnd);
  if (win)
    win->mGesture.UnregisterTouchWindow(aWnd);
  return TRUE;
}










void
nsWindow::SetSizeConstraints(const SizeConstraints& aConstraints)
{
  SizeConstraints c = aConstraints;
  if (mWindowType != eWindowType_popup) {
    c.mMinSize.width = std::max(int32_t(::GetSystemMetrics(SM_CXMINTRACK)), c.mMinSize.width);
    c.mMinSize.height = std::max(int32_t(::GetSystemMetrics(SM_CYMINTRACK)), c.mMinSize.height);
  }

  nsBaseWidget::SetSizeConstraints(c);
}


NS_METHOD nsWindow::Move(double aX, double aY)
{
  if (mWindowType == eWindowType_toplevel ||
      mWindowType == eWindowType_dialog) {
    SetSizeMode(nsSizeMode_Normal);
  }
  
  
  
  

  
  
  
  if (mWindowType != eWindowType_popup && (mBounds.x == aX) && (mBounds.y == aY))
  {
    
    return NS_OK;
  }

  
  
  double scale =
    (mWindowType <= eWindowType_popup) ? GetDefaultScale() : 1.0;
  int32_t x = NSToIntRound(aX * scale);
  int32_t y = NSToIntRound(aY * scale);

  mBounds.x = x;
  mBounds.y = y;

  if (mWnd) {
#ifdef DEBUG
    
    if (mIsTopWidgetWindow) { 
      
      
      HDC dc = ::GetDC(mWnd);
      if (dc) {
        if (::GetDeviceCaps(dc, TECHNOLOGY) == DT_RASDISPLAY) {
          RECT workArea;
          ::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
          
          if (x < 0 || x >= workArea.right || y < 0 || y >= workArea.bottom) {
            PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
                   ("window moved to offscreen position\n"));
          }
        }
      ::ReleaseDC(mWnd, dc);
      }
    }
#endif
    ClearThemeRegion();

    UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE;
    
    
    
    if (mWindowType == eWindowType_plugin &&
        (!mLayerManager || mLayerManager->GetBackendType() == LAYERS_D3D9) &&
        mClipRects &&
        (mClipRectCount != 1 || !mClipRects[0].IsEqualInterior(nsIntRect(0, 0, mBounds.width, mBounds.height)))) {
      flags |= SWP_NOCOPYBITS;
    }
    VERIFY(::SetWindowPos(mWnd, NULL, x, y, 0, 0, flags));

    SetThemeRegion();
  }
  NotifyRollupGeometryChange();
  return NS_OK;
}


NS_METHOD nsWindow::Resize(double aWidth, double aHeight, bool aRepaint)
{
  
  
  double scale =
    (mWindowType <= eWindowType_popup) ? GetDefaultScale() : 1.0;
  int32_t width = NSToIntRound(aWidth * scale);
  int32_t height = NSToIntRound(aHeight * scale);

  NS_ASSERTION((width >= 0) , "Negative width passed to nsWindow::Resize");
  NS_ASSERTION((height >= 0), "Negative height passed to nsWindow::Resize");

  ConstrainSize(&width, &height);

  
  if (mBounds.width == width && mBounds.height == height) {
    if (aRepaint) {
      Invalidate();
    }
    return NS_OK;
  }

#ifdef MOZ_XUL
  if (eTransparencyTransparent == mTransparencyMode)
    ResizeTranslucentWindow(width, height);
#endif

  
  mBounds.width  = width;
  mBounds.height = height;

  if (mWnd) {
    UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;

    if (!aRepaint) {
      flags |= SWP_NOREDRAW;
    }

    ClearThemeRegion();
    VERIFY(::SetWindowPos(mWnd, NULL, 0, 0, width, GetHeight(height), flags));
    SetThemeRegion();
  }

  if (aRepaint)
    Invalidate();

  NotifyRollupGeometryChange();
  return NS_OK;
}


NS_METHOD nsWindow::Resize(double aX, double aY, double aWidth, double aHeight, bool aRepaint)
{
  
  
  double scale =
    (mWindowType <= eWindowType_popup) ? GetDefaultScale() : 1.0;
  int32_t x = NSToIntRound(aX * scale);
  int32_t y = NSToIntRound(aY * scale);
  int32_t width = NSToIntRound(aWidth * scale);
  int32_t height = NSToIntRound(aHeight * scale);

  NS_ASSERTION((width >= 0),  "Negative width passed to nsWindow::Resize");
  NS_ASSERTION((height >= 0), "Negative height passed to nsWindow::Resize");

  ConstrainSize(&width, &height);

  
  if (mBounds.x == x && mBounds.y == y &&
      mBounds.width == width && mBounds.height == height) {
    if (aRepaint) {
      Invalidate();
    }
    return NS_OK;
  }

#ifdef MOZ_XUL
  if (eTransparencyTransparent == mTransparencyMode)
    ResizeTranslucentWindow(width, height);
#endif

  
  mBounds.x      = x;
  mBounds.y      = y;
  mBounds.width  = width;
  mBounds.height = height;

  if (mWnd) {
    UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!aRepaint) {
      flags |= SWP_NOREDRAW;
    }

    ClearThemeRegion();
    VERIFY(::SetWindowPos(mWnd, NULL, x, y, width, GetHeight(height), flags));
    SetThemeRegion();
  }

  if (aRepaint)
    Invalidate();

  NotifyRollupGeometryChange();
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::BeginResizeDrag(nsGUIEvent* aEvent, int32_t aHorizontal, int32_t aVertical)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  if (aEvent->eventStructType != NS_MOUSE_EVENT) {
    
    return NS_ERROR_INVALID_ARG;
  }

  nsMouseEvent* mouseEvent = static_cast<nsMouseEvent*>(aEvent);
  if (mouseEvent->button != nsMouseEvent::eLeftButton) {
    
    return NS_ERROR_INVALID_ARG;
  }

  
  WPARAM syscommand;
  if (aVertical < 0) {
    if (aHorizontal < 0) {
      syscommand = SC_SIZE | WMSZ_TOPLEFT;
    } else if (aHorizontal == 0) {
      syscommand = SC_SIZE | WMSZ_TOP;
    } else {
      syscommand = SC_SIZE | WMSZ_TOPRIGHT;
    }
  } else if (aVertical == 0) {
    if (aHorizontal < 0) {
      syscommand = SC_SIZE | WMSZ_LEFT;
    } else if (aHorizontal == 0) {
      return NS_ERROR_INVALID_ARG;
    } else {
      syscommand = SC_SIZE | WMSZ_RIGHT;
    }
  } else {
    if (aHorizontal < 0) {
      syscommand = SC_SIZE | WMSZ_BOTTOMLEFT;
    } else if (aHorizontal == 0) {
      syscommand = SC_SIZE | WMSZ_BOTTOM;
    } else {
      syscommand = SC_SIZE | WMSZ_BOTTOMRIGHT;
    }
  }

  
  CaptureMouse(false);

  
  HWND toplevelWnd = WinUtils::GetTopLevelHWND(mWnd, true);

  
  ::PostMessage(toplevelWnd, WM_SYSCOMMAND, syscommand,
                POINTTOPOINTS(aEvent->refPoint));

  return NS_OK;
}













NS_METHOD nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                nsIWidget *aWidget, bool aActivate)
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


NS_IMETHODIMP nsWindow::SetSizeMode(int32_t aMode) {

  nsresult rv;

  
  
  
  if (aMode == mSizeMode)
    return NS_OK;

  
  mLastSizeMode = mSizeMode;
  rv = nsBaseWidget::SetSizeMode(aMode);
  if (NS_SUCCEEDED(rv) && mIsVisible) {
    int mode;

    switch (aMode) {
      case nsSizeMode_Fullscreen :
        mode = SW_SHOW;
        break;

      case nsSizeMode_Maximized :
        mode = SW_MAXIMIZE;
        break;

      case nsSizeMode_Minimized :
        
        
        
        
        
        
        mode = sTrimOnMinimize ? SW_MINIMIZE : SW_SHOWMINIMIZED;
        break;

      default :
        mode = SW_RESTORE;
    }

    WINDOWPLACEMENT pl;
    pl.length = sizeof(pl);
    ::GetWindowPlacement(mWnd, &pl);
    
    
    
    
    if( !(pl.showCmd == SW_SHOWNORMAL && mode == SW_RESTORE) ) {
      ::ShowWindow(mWnd, mode);
    }
    
    if (mode == SW_MAXIMIZE || mode == SW_SHOW)
      DispatchFocusToTopLevelWindow(true);
  }
  return rv;
}


NS_METHOD nsWindow::ConstrainPosition(bool aAllowSlop,
                                      int32_t *aX, int32_t *aY)
{
  if (!mIsTopWidgetWindow) 
    return NS_OK;

  bool doConstrain = false; 

  

  RECT screenRect;

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    int32_t left, top, width, height;

    
    width = mBounds.width > 0 ? mBounds.width : 1;
    height = mBounds.height > 0 ? mBounds.height : 1;
    screenmgr->ScreenForRect(*aX, *aY, width, height,
                             getter_AddRefs(screen));
    if (screen) {
      if (mSizeMode != nsSizeMode_Fullscreen) {
        
        screen->GetAvailRect(&left, &top, &width, &height);
      } else {
        
        screen->GetRect(&left, &top, &width, &height);
      }
      screenRect.left = left;
      screenRect.right = left+width;
      screenRect.top = top;
      screenRect.bottom = top+height;
      doConstrain = true;
    }
  } else {
    if (mWnd) {
      HDC dc = ::GetDC(mWnd);
      if (dc) {
        if (::GetDeviceCaps(dc, TECHNOLOGY) == DT_RASDISPLAY) {
          if (mSizeMode != nsSizeMode_Fullscreen) {
            ::SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
          } else {
            screenRect.left = screenRect.top = 0;
            screenRect.right = GetSystemMetrics(SM_CXFULLSCREEN);
            screenRect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
          }
          doConstrain = true;
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










NS_METHOD nsWindow::Enable(bool bState)
{
  if (mWnd) {
    ::EnableWindow(mWnd, bState);
  }
  return NS_OK;
}


bool nsWindow::IsEnabled() const
{
  return !mWnd ||
         (::IsWindowEnabled(mWnd) &&
          ::IsWindowEnabled(::GetAncestor(mWnd, GA_ROOT)));
}










NS_METHOD nsWindow::SetFocus(bool aRaise)
{
  if (mWnd) {
#ifdef WINSTATE_DEBUG_OUTPUT
    if (mWnd == WinUtils::GetTopLevelHWND(mWnd)) {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
             ("*** SetFocus: [  top] raise=%d\n", aRaise));
    } else {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
             ("*** SetFocus: [child] raise=%d\n", aRaise));
    }
#endif
    
    HWND toplevelWnd = WinUtils::GetTopLevelHWND(mWnd);
    if (aRaise && ::IsIconic(toplevelWnd)) {
      ::ShowWindow(toplevelWnd, SW_RESTORE);
    }
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

    
    
    if (mWindowType == eWindowType_popup) {
      aRect.x = r.left;
      aRect.y = r.top;
      return NS_OK;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    HWND parent = ::GetParent(mWnd);
    if (parent) {
      RECT pr;
      VERIFY(::GetWindowRect(parent, &pr));
      r.left -= pr.left;
      r.top  -= pr.top;
      
      nsWindow* pWidget = static_cast<nsWindow*>(GetParent());
      if (pWidget && pWidget->IsTopLevelWidget()) {
        nsIntPoint clientOffset = pWidget->GetClientOffset();
        r.left -= clientOffset.x;
        r.top  -= clientOffset.y;
      }
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

    nsIntRect bounds;
    GetBounds(bounds);
    aRect.MoveTo(bounds.TopLeft() + GetClientOffset());
    aRect.width  = r.right - r.left;
    aRect.height = r.bottom - r.top;

  } else {
    aRect.SetRect(0,0,0,0);
  }
  return NS_OK;
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



nsIntPoint nsWindow::GetClientOffset()
{
  if (!mWnd) {
    return nsIntPoint(0, 0);
  }

  RECT r1;
  GetWindowRect(mWnd, &r1);
  nsIntPoint pt = WidgetToScreenOffset();
  return nsIntPoint(pt.x - r1.left, pt.y - r1.top);
}

void
nsWindow::SetDrawsInTitlebar(bool aState)
{
  nsWindow * window = GetTopLevelWindow(true);
  if (window && window != this) {
    return window->SetDrawsInTitlebar(aState);
  }

  if (aState) {
    
    nsIntMargin margins(0, -1, -1, -1);
    SetNonClientMargins(margins);
  }
  else {
    nsIntMargin margins(-1, -1, -1, -1);
    SetNonClientMargins(margins);
  }
}

NS_IMETHODIMP
nsWindow::GetNonClientMargins(nsIntMargin &margins)
{
  nsWindow * window = GetTopLevelWindow(true);
  if (window && window != this) {
    return window->GetNonClientMargins(margins);
  }

  if (mCustomNonClient) {
    margins = mNonClientMargins;
    return NS_OK;
  }

  margins.top = GetSystemMetrics(SM_CYCAPTION);
  margins.bottom = GetSystemMetrics(SM_CYFRAME);
  margins.top += margins.bottom;
  margins.left = margins.right = GetSystemMetrics(SM_CXFRAME);

  return NS_OK;
}

void
nsWindow::ResetLayout()
{
  
  
  SetWindowPos(mWnd, 0, 0, 0, 0, 0,
               SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|
               SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

  
  if (!mIsVisible)
    return;

  
  RECT clientRc = {0};
  GetClientRect(mWnd, &clientRc);
  nsIntRect evRect(WinUtils::ToIntRect(clientRc));
  OnResize(evRect);

  
  Invalidate();
}




static const PRUnichar kManageWindowInfoProperty[] = L"ManageWindowInfoProperty";
typedef BOOL (WINAPI *GetWindowInfoPtr)(HWND hwnd, PWINDOWINFO pwi);
static GetWindowInfoPtr sGetWindowInfoPtrStub = NULL;

BOOL WINAPI
GetWindowInfoHook(HWND hWnd, PWINDOWINFO pwi)
{
  if (!sGetWindowInfoPtrStub) {
    NS_ASSERTION(FALSE, "Something is horribly wrong in GetWindowInfoHook!");
    return FALSE;
  }
  int windowStatus = 
    reinterpret_cast<LONG_PTR>(GetPropW(hWnd, kManageWindowInfoProperty));
  
  if (!windowStatus)
    return sGetWindowInfoPtrStub(hWnd, pwi);
  
  
  BOOL result = sGetWindowInfoPtrStub(hWnd, pwi);
  if (result && pwi)
    pwi->dwWindowStatus = (windowStatus == 1 ? 0 : WS_ACTIVECAPTION);
  return result;
}

void
nsWindow::UpdateGetWindowInfoCaptionStatus(bool aActiveCaption)
{
  if (!mWnd)
    return;

  if (!sGetWindowInfoPtrStub) {
    sUser32Intercept.Init("user32.dll");
    if (!sUser32Intercept.AddHook("GetWindowInfo", reinterpret_cast<intptr_t>(GetWindowInfoHook),
                                  (void**) &sGetWindowInfoPtrStub))
      return;
  }
  
  SetPropW(mWnd, kManageWindowInfoProperty, 
    reinterpret_cast<HANDLE>(static_cast<int>(aActiveCaption) + 1));
}


























bool
nsWindow::UpdateNonClientMargins(int32_t aSizeMode, bool aReflowWindow)
{
  if (!mCustomNonClient)
    return false;

  if (aSizeMode == -1) {
    aSizeMode = mSizeMode;
  }

  bool hasCaption = (mBorderStyle
                    & (eBorderStyle_all
                     | eBorderStyle_title
                     | eBorderStyle_menu
                     | eBorderStyle_default));

  
  
  
  
  
  
  
  
  
  
  
  mCaptionHeight = GetSystemMetrics(SM_CYFRAME)
                 + (hasCaption ? GetSystemMetrics(SM_CYCAPTION)
                                 + GetSystemMetrics(SM_CXPADDEDBORDER)
                               : 0);

  
  
  
  
  
  
  
  
  
  mHorResizeMargin = GetSystemMetrics(SM_CXFRAME)
                   + (hasCaption ? GetSystemMetrics(SM_CXPADDEDBORDER) : 0);

  
  
  
  
  
  
  
  
  mVertResizeMargin = GetSystemMetrics(SM_CYFRAME)
                    + (hasCaption ? GetSystemMetrics(SM_CXPADDEDBORDER) : 0);

  if (aSizeMode == nsSizeMode_Minimized) {
    
    mNonClientOffset.top = 0;
    mNonClientOffset.left = 0;
    mNonClientOffset.right = 0;
    mNonClientOffset.bottom = 0;
  } else if (aSizeMode == nsSizeMode_Fullscreen) {
    
    
    
    
    mNonClientOffset.top = mCaptionHeight;
    mNonClientOffset.bottom = mVertResizeMargin;
    mNonClientOffset.left = mHorResizeMargin;
    mNonClientOffset.right = mHorResizeMargin;
  } else if (aSizeMode == nsSizeMode_Maximized) {
    
    
    
    
    
    
    
    
    mNonClientOffset.top = mCaptionHeight;
    mNonClientOffset.bottom = 0;
    mNonClientOffset.left = 0;
    mNonClientOffset.right = 0;

    APPBARDATA appBarData;
    appBarData.cbSize = sizeof(appBarData);
    UINT taskbarState = SHAppBarMessage(ABM_GETSTATE, &appBarData);
    if (ABS_AUTOHIDE & taskbarState) {
      UINT edge = -1;
      appBarData.hWnd = FindWindow(L"Shell_TrayWnd", NULL);
      if (appBarData.hWnd) {
        HMONITOR taskbarMonitor = ::MonitorFromWindow(appBarData.hWnd,
                                                      MONITOR_DEFAULTTOPRIMARY);
        HMONITOR windowMonitor = ::MonitorFromWindow(mWnd,
                                                     MONITOR_DEFAULTTONEAREST);
        if (taskbarMonitor == windowMonitor) {
          SHAppBarMessage(ABM_GETTASKBARPOS, &appBarData);
          edge = appBarData.uEdge;
        }
      }

      if (ABE_LEFT == edge) {
        mNonClientOffset.left -= 1;
      } else if (ABE_RIGHT == edge) {
        mNonClientOffset.right -= 1;
      } else if (ABE_BOTTOM == edge || ABE_TOP == edge) {
        mNonClientOffset.bottom -= 1;
      }
    }
  } else {
    bool glass = nsUXThemeData::CheckForCompositor();

    
    
    
    
    
    
    

    if (mNonClientMargins.top > 0 && glass) {
      mNonClientOffset.top = std::min(mCaptionHeight, mNonClientMargins.top);
    } else if (mNonClientMargins.top == 0) {
      mNonClientOffset.top = mCaptionHeight;
    } else {
      mNonClientOffset.top = 0;
    }

    if (mNonClientMargins.bottom > 0 && glass) {
      mNonClientOffset.bottom = std::min(mVertResizeMargin, mNonClientMargins.bottom);
    } else if (mNonClientMargins.bottom == 0) {
      mNonClientOffset.bottom = mVertResizeMargin;
    } else {
      mNonClientOffset.bottom = 0;
    }

    if (mNonClientMargins.left > 0 && glass) {
      mNonClientOffset.left = std::min(mHorResizeMargin, mNonClientMargins.left);
    } else if (mNonClientMargins.left == 0) {
      mNonClientOffset.left = mHorResizeMargin;
    } else {
      mNonClientOffset.left = 0;
    }

    if (mNonClientMargins.right > 0 && glass) {
      mNonClientOffset.right = std::min(mHorResizeMargin, mNonClientMargins.right);
    } else if (mNonClientMargins.right == 0) {
      mNonClientOffset.right = mHorResizeMargin;
    } else {
      mNonClientOffset.right = 0;
    }
  }

  if (aReflowWindow) {
    
    
    ResetLayout();
  }

  return true;
}

NS_IMETHODIMP
nsWindow::SetNonClientMargins(nsIntMargin &margins)
{
  if (!mIsTopWidgetWindow ||
      mBorderStyle & eBorderStyle_none ||
      mHideChrome)
    return NS_ERROR_INVALID_ARG;

  
  if (margins.top == -1 && margins.left == -1 &&
      margins.right == -1 && margins.bottom == -1) {
    mCustomNonClient = false;
    mNonClientMargins = margins;
    RemovePropW(mWnd, kManageWindowInfoProperty);
    
    
    ResetLayout();
    return NS_OK;
  }

  if (margins.top < -1 || margins.bottom < -1 ||
      margins.left < -1 || margins.right < -1)
    return NS_ERROR_INVALID_ARG;

  mNonClientMargins = margins;
  mCustomNonClient = true;
  if (!UpdateNonClientMargins()) {
    NS_WARNING("UpdateNonClientMargins failed!");
    return NS_OK;
  }

  return NS_OK;
}

void
nsWindow::InvalidateNonClientRegion()
{
  
  
  
  
  
  
  
  
  
  
  
  
  RECT rect;
  GetWindowRect(mWnd, &rect);
  MapWindowPoints(NULL, mWnd, (LPPOINT)&rect, 2);
  HRGN winRgn = CreateRectRgnIndirect(&rect);

  
  
  
  GetWindowRect(mWnd, &rect);
  rect.top += mCaptionHeight;
  rect.right -= mHorResizeMargin;
  rect.bottom -= mHorResizeMargin;
  rect.left += mVertResizeMargin;
  MapWindowPoints(NULL, mWnd, (LPPOINT)&rect, 2);
  HRGN clientRgn = CreateRectRgnIndirect(&rect);
  CombineRgn(winRgn, winRgn, clientRgn, RGN_DIFF);
  DeleteObject(clientRgn);

  
  RedrawWindow(mWnd, NULL, winRgn, RDW_FRAME|RDW_INVALIDATE);
  DeleteObject(winRgn);
}

HRGN
nsWindow::ExcludeNonClientFromPaintRegion(HRGN aRegion)
{
  RECT rect;
  HRGN rgn = NULL;
  if (aRegion == (HRGN)1) { 
    GetWindowRect(mWnd, &rect);
    rgn = CreateRectRgnIndirect(&rect);
  } else {
    rgn = aRegion;
  }
  GetClientRect(mWnd, &rect);
  MapWindowPoints(mWnd, NULL, (LPPOINT)&rect, 2);
  HRGN nonClientRgn = CreateRectRgnIndirect(&rect);
  CombineRgn(rgn, rgn, nonClientRgn, RGN_DIFF);
  DeleteObject(nonClientRgn);
  return rgn;
}









NS_METHOD nsWindow::SetBackgroundColor(const nscolor &aColor)
{
  nsBaseWidget::SetBackgroundColor(aColor);

  if (mBrush)
    ::DeleteObject(mBrush);

  mBrush = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
  if (mWnd != NULL) {
    ::SetClassLongPtrW(mWnd, GCLP_HBRBACKGROUND, (LONG_PTR)mBrush);
  }
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
    
    if (sHCursor == oldCursor) {
      NS_IF_RELEASE(sCursorImgContainer);
      if (sHCursor != NULL)
        ::DestroyIcon(sHCursor);
      sHCursor = NULL;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsWindow::SetCursor(imgIContainer* aCursor,
                                  uint32_t aHotspotX, uint32_t aHotspotY)
{
  if (sCursorImgContainer == aCursor && sHCursor) {
    ::SetCursor(sHCursor);
    return NS_OK;
  }

  int32_t width;
  int32_t height;

  nsresult rv;
  rv = aCursor->GetWidth(&width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCursor->GetHeight(&height);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (width > 128 || height > 128)
    return NS_ERROR_NOT_AVAILABLE;

  HCURSOR cursor;
  
  gfxIntSize size(0, 0);
  rv = nsWindowGfx::CreateIcon(aCursor, true, aHotspotX, aHotspotY, size, &cursor);
  NS_ENSURE_SUCCESS(rv, rv);

  mCursor = nsCursor(-1);
  ::SetCursor(cursor);

  NS_IF_RELEASE(sCursorImgContainer);
  sCursorImgContainer = aCursor;
  NS_ADDREF(sCursorImgContainer);

  if (sHCursor != NULL)
    ::DestroyIcon(sHCursor);
  sHCursor = cursor;

  return NS_OK;
}










#ifdef MOZ_XUL
nsTransparencyMode nsWindow::GetTransparencyMode()
{
  return GetTopLevelWindow(true)->GetWindowTranslucencyInner();
}

void nsWindow::SetTransparencyMode(nsTransparencyMode aMode)
{
  GetTopLevelWindow(true)->SetWindowTranslucencyInner(aMode);
}

static const nsIntRegion
RegionFromArray(const nsTArray<nsIntRect>& aRects)
{
  nsIntRegion region;
  for (uint32_t i = 0; i < aRects.Length(); ++i) {
    region.Or(region, aRects[i]);
  }
  return region;
}

void nsWindow::UpdateOpaqueRegion(const nsIntRegion &aOpaqueRegion)
{
  if (!HasGlass() || GetParent())
    return;

  
  
  
  MARGINS margins = { -1, -1, -1, -1 };
  if (!aOpaqueRegion.IsEmpty()) {
    nsIntRect pluginBounds;
    for (nsIWidget* child = GetFirstChild(); child; child = child->GetNextSibling()) {
      nsWindowType type;
      child->GetWindowType(type);
      if (type == eWindowType_plugin) {
        
        nsIntRect childBounds;
        child->GetBounds(childBounds);
        pluginBounds.UnionRect(pluginBounds, childBounds);
      }
    }

    nsIntRect clientBounds;
    GetClientBounds(clientBounds);

    
    
    nsIntRect largest = aOpaqueRegion.GetLargestRectangle(pluginBounds);
    margins.cxLeftWidth = largest.x;
    margins.cxRightWidth = clientBounds.width - largest.XMost();
    margins.cyBottomHeight = clientBounds.height - largest.YMost();
    if (mCustomNonClient) {
      
      
      largest.y = std::max<uint32_t>(largest.y,
                         nsUXThemeData::sCommandButtons[CMDBUTTONIDX_BUTTONBOX].cy);
    }
    margins.cyTopHeight = largest.y;
  }

  
  if (memcmp(&mGlassMargins, &margins, sizeof mGlassMargins)) {
    mGlassMargins = margins;
    UpdateGlass();
  }
}

void nsWindow::UpdateGlass()
{
  MARGINS margins = mGlassMargins;

  
  
  
  
  DWMNCRENDERINGPOLICY policy = DWMNCRP_USEWINDOWSTYLE;
  switch (mTransparencyMode) {
  case eTransparencyBorderlessGlass:
    
    if (margins.cxLeftWidth >= 0) {
      margins.cxLeftWidth += kGlassMarginAdjustment;
      margins.cyTopHeight += kGlassMarginAdjustment;
      margins.cxRightWidth += kGlassMarginAdjustment;
      margins.cyBottomHeight += kGlassMarginAdjustment;
    }
    
  case eTransparencyGlass:
    policy = DWMNCRP_ENABLED;
    break;
  }

  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("glass margins: left:%d top:%d right:%d bottom:%d\n",
          margins.cxLeftWidth, margins.cyTopHeight,
          margins.cxRightWidth, margins.cyBottomHeight));

  
  if(nsUXThemeData::CheckForCompositor()) {
    nsUXThemeData::dwmExtendFrameIntoClientAreaPtr(mWnd, &margins);
    nsUXThemeData::dwmSetWindowAttributePtr(mWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof policy);
  }
}
#endif









NS_IMETHODIMP nsWindow::HideWindowChrome(bool aShouldHide)
{
  HWND hwnd = WinUtils::GetTopLevelHWND(mWnd, true);
  if (!WinUtils::GetNSWindowPtr(hwnd))
  {
    NS_WARNING("Trying to hide window decorations in an embedded context");
    return NS_ERROR_FAILURE;
  }

  if (mHideChrome == aShouldHide)
    return NS_OK;

  DWORD_PTR style, exStyle;
  mHideChrome = aShouldHide;
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










NS_METHOD nsWindow::Invalidate(bool aEraseBackground, 
                               bool aUpdateNCArea,
                               bool aIncludeChildren)
{
  if (!mWnd) {
    return NS_OK;
  }

#ifdef WIDGET_DEBUG_OUTPUT
  debug_DumpInvalidate(stdout,
                       this,
                       nullptr,
                       nsAutoCString("noname"),
                       (int32_t) mWnd);
#endif 

  DWORD flags = RDW_INVALIDATE;
  if (aEraseBackground) {
    flags |= RDW_ERASE;
  }
  if (aUpdateNCArea) {
    flags |= RDW_FRAME;
  }
  if (aIncludeChildren) {
    flags |= RDW_ALLCHILDREN;
  }

  VERIFY(::RedrawWindow(mWnd, NULL, NULL, flags));
  return NS_OK;
}


NS_METHOD nsWindow::Invalidate(const nsIntRect & aRect)
{
  if (mWnd)
  {
#ifdef WIDGET_DEBUG_OUTPUT
    debug_DumpInvalidate(stdout,
                         this,
                         &aRect,
                         nsAutoCString("noname"),
                         (int32_t) mWnd);
#endif 

    RECT rect;

    rect.left   = aRect.x;
    rect.top    = aRect.y;
    rect.right  = aRect.x + aRect.width;
    rect.bottom = aRect.y + aRect.height;

    VERIFY(::InvalidateRect(mWnd, &rect, FALSE));
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(bool aFullScreen)
{
  
  nsCOMPtr<nsIWinTaskbar> taskbarInfo =
    do_GetService(NS_TASKBAR_CONTRACTID);

  mFullscreenMode = aFullScreen;
  if (aFullScreen) {
    if (mSizeMode == nsSizeMode_Fullscreen)
      return NS_OK;
    mOldSizeMode = mSizeMode;
    SetSizeMode(nsSizeMode_Fullscreen);

    
    if (taskbarInfo) {
      taskbarInfo->PrepareFullScreenHWND(mWnd, TRUE);
    }
  } else {
    SetSizeMode(mOldSizeMode);
  }

  UpdateNonClientMargins();

  bool visible = mIsVisible;
  if (mOldSizeMode == nsSizeMode_Normal)
    Show(false);
  
  
  
  
  nsresult rv = nsBaseWidget::MakeFullScreen(aFullScreen);

  if (visible) {
    Show(true);
    Invalidate();
  }

  
  if (!aFullScreen && taskbarInfo) {
    taskbarInfo->PrepareFullScreenHWND(mWnd, FALSE);
  }

  if (mWidgetListener)
    mWidgetListener->SizeModeChanged(mSizeMode);

  return rv;
}













void* nsWindow::GetNativeData(uint32_t aDataType)
{
  nsAutoString className;
  switch (aDataType) {
    case NS_NATIVE_TMP_WINDOW:
      GetWindowClass(className);
      return (void*)::CreateWindowExW(mIsRTL ? WS_EX_LAYOUTRTL : 0,
                                      className.get(),
                                      L"",
                                      WS_CHILD,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      mWnd,
                                      NULL,
                                      nsToolkit::mDllInstance,
                                      NULL);
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_SHAREABLE_WINDOW:
      return (void*)mWnd;
    case NS_NATIVE_GRAPHIC:
      
#ifdef MOZ_XUL
      return (void*)(eTransparencyTransparent == mTransparencyMode) ?
        mMemoryDC : ::GetDC(mWnd);
#else
      return (void*)::GetDC(mWnd);
#endif

    case NS_NATIVE_TSF_THREAD_MGR:
    case NS_NATIVE_TSF_CATEGORY_MGR:
    case NS_NATIVE_TSF_DISPLAY_ATTR_MGR:
      return IMEHandler::GetNativeData(aDataType);

    default:
      break;
  }

  return NULL;
}


void nsWindow::FreeNativeData(void * data, uint32_t aDataType)
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









NS_METHOD nsWindow::SetTitle(const nsAString& aTitle)
{
  const nsString& strTitle = PromiseFlatString(aTitle);
  ::SendMessageW(mWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)(LPCWSTR)strTitle.get());
  return NS_OK;
}









NS_METHOD nsWindow::SetIcon(const nsAString& aIconSpec) 
{
  

  nsCOMPtr<nsIFile> iconFile;
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
    mIconBig = bigIcon;
  }
#ifdef DEBUG_SetIcon
  else {
    NS_LossyConvertUTF16toASCII cPath(iconPath);
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
           ("\nIcon load error; icon=%s, rc=0x%08X\n\n", 
            cPath.get(), ::GetLastError()));
  }
#endif
  if (smallIcon) {
    HICON icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)smallIcon);
    if (icon)
      ::DestroyIcon(icon);
    mIconSmall = smallIcon;
  }
#ifdef DEBUG_SetIcon
  else {
    NS_LossyConvertUTF16toASCII cPath(iconPath);
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
           ("\nSmall icon load error; icon=%s, rc=0x%08X\n\n", 
            cPath.get(), ::GetLastError()));
  }
#endif
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

nsIntSize nsWindow::ClientToWindowSize(const nsIntSize& aClientSize)
{
  if (mWindowType == eWindowType_popup && !IsPopupWithTitleBar())
    return aClientSize;

  
  RECT r;
  r.left = 200;
  r.top = 200;
  r.right = 200 + aClientSize.width;
  r.bottom = 200 + aClientSize.height;
  ::AdjustWindowRectEx(&r, WindowStyle(), false, WindowExStyle());

  return nsIntSize(r.right - r.left, r.bottom - r.top);
}









NS_METHOD nsWindow::EnableDragDrop(bool aEnable)
{
  NS_ASSERTION(mWnd, "nsWindow::EnableDragDrop() called after Destroy()");

  nsresult rv = NS_ERROR_FAILURE;
  if (aEnable) {
    if (nullptr == mNativeDragTarget) {
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
    if (nullptr != mWnd && NULL != mNativeDragTarget) {
      ::RevokeDragDrop(mWnd);
      if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget, FALSE, TRUE)) {
        rv = NS_OK;
      }
      mNativeDragTarget->DragCancel();
      NS_RELEASE(mNativeDragTarget);
    }
  }
  return rv;
}









NS_METHOD nsWindow::CaptureMouse(bool aCapture)
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
  sIsInMouseCapture = aCapture;
  return NS_OK;
}











NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener,
                                            bool aDoCapture)
{
  if (aDoCapture) {
    gRollupListener = aListener;
    if (!sMsgFilterHook && !sCallProcHook && !sCallMouseHook) {
      RegisterSpecialDropdownHooks();
    }
    sProcessHook = true;
  } else {
    gRollupListener = nullptr;
    sProcessHook = false;
    UnregisterSpecialDropdownHooks();
  }

  return NS_OK;
}










NS_IMETHODIMP
nsWindow::GetAttention(int32_t aCycleCount)
{
  
  if (!mWnd)
    return NS_ERROR_NOT_INITIALIZED;

  HWND flashWnd = WinUtils::GetTopLevelHWND(mWnd, false, false);
  HWND fgWnd = ::GetForegroundWindow();
  
  
  if (aCycleCount == 0 || 
      flashWnd == fgWnd ||
      flashWnd == WinUtils::GetTopLevelHWND(fgWnd, false, false)) {
    return NS_OK;
  }

  DWORD defaultCycleCount = 0;
  ::SystemParametersInfo(SPI_GETFOREGROUNDFLASHCOUNT, 0, &defaultCycleCount, 0);

  FLASHWINFO flashInfo = { sizeof(FLASHWINFO), flashWnd,
    FLASHW_ALL, aCycleCount > 0 ? aCycleCount : defaultCycleCount, 0 };
  ::FlashWindowEx(&flashInfo);

  return NS_OK;
}

void nsWindow::StopFlashing()
{
  HWND flashWnd = mWnd;
  while (HWND ownerWnd = ::GetWindow(flashWnd, GW_OWNER)) {
    flashWnd = ownerWnd;
  }

  FLASHWINFO flashInfo = { sizeof(FLASHWINFO), flashWnd,
    FLASHW_STOP, 0, 0 };
  ::FlashWindowEx(&flashInfo);
}










bool
nsWindow::HasPendingInputEvent()
{
  
  
  
  
  
  if (HIWORD(GetQueueStatus(QS_INPUT)))
    return true;
  GUITHREADINFO guiInfo;
  guiInfo.cbSize = sizeof(GUITHREADINFO);
  if (!GetGUIThreadInfo(GetCurrentThreadId(), &guiInfo))
    return false;
  return GUI_INMOVESIZE == (guiInfo.flags & GUI_INMOVESIZE);
}









struct LayerManagerPrefs {
  LayerManagerPrefs()
    : mAccelerateByDefault(true)
    , mDisableAcceleration(false)
    , mPreferOpenGL(false)
    , mPreferD3D9(false)
  {}
  bool mAccelerateByDefault;
  bool mDisableAcceleration;
  bool mForceAcceleration;
  bool mPreferOpenGL;
  bool mPreferD3D9;
};

static void
GetLayerManagerPrefs(LayerManagerPrefs* aManagerPrefs)
{
  Preferences::GetBool("layers.acceleration.disabled",
                       &aManagerPrefs->mDisableAcceleration);
  Preferences::GetBool("layers.acceleration.force-enabled",
                       &aManagerPrefs->mForceAcceleration);
  Preferences::GetBool("layers.prefer-opengl",
                       &aManagerPrefs->mPreferOpenGL);
  Preferences::GetBool("layers.prefer-d3d9",
                       &aManagerPrefs->mPreferD3D9);

  const char *acceleratedEnv = PR_GetEnv("MOZ_ACCELERATED");
  aManagerPrefs->mAccelerateByDefault =
    aManagerPrefs->mAccelerateByDefault ||
    (acceleratedEnv && (*acceleratedEnv != '0'));

  bool safeMode = false;
  nsCOMPtr<nsIXULRuntime> xr = do_GetService("@mozilla.org/xre/runtime;1");
  if (xr)
    xr->GetInSafeMode(&safeMode);
  aManagerPrefs->mDisableAcceleration =
    aManagerPrefs->mDisableAcceleration || safeMode;
}

bool
nsWindow::ShouldUseOffMainThreadCompositing()
{
  
  return false;
}

LayerManager*
nsWindow::GetLayerManager(PLayersChild* aShadowManager,
                          LayersBackend aBackendHint,
                          LayerManagerPersistence aPersistence,
                          bool* aAllowRetaining)
{
  if (aAllowRetaining) {
    *aAllowRetaining = true;
  }

#ifdef MOZ_ENABLE_D3D10_LAYER
  if (mLayerManager) {
    if (mLayerManager->GetBackendType() == LAYERS_D3D10)
    {
      LayerManagerD3D10 *layerManagerD3D10 =
        static_cast<LayerManagerD3D10*>(mLayerManager.get());
      if (layerManagerD3D10->device() !=
          gfxWindowsPlatform::GetPlatform()->GetD3D10Device())
      {
        MOZ_ASSERT(!mLayerManager->IsInTransaction());

        mLayerManager->Destroy();
        mLayerManager = nullptr;
      }
    }
  }
#endif

  RECT windowRect;
  ::GetClientRect(mWnd, &windowRect);

  if (!mLayerManager ||
      (!sAllowD3D9 && aPersistence == LAYER_MANAGER_PERSISTENT &&
        mLayerManager->GetBackendType() == LAYERS_BASIC)) {
    
    
    LayerManagerPrefs prefs;
    GetLayerManagerPrefs(&prefs);

    


    if (eTransparencyTransparent == mTransparencyMode ||
        prefs.mDisableAcceleration ||
        windowRect.right - windowRect.left > MAX_ACCELERATED_DIMENSION ||
        windowRect.bottom - windowRect.top > MAX_ACCELERATED_DIMENSION)
      mUseLayersAcceleration = false;
    else if (prefs.mAccelerateByDefault)
      mUseLayersAcceleration = true;

    if (mUseLayersAcceleration) {
      if (aPersistence == LAYER_MANAGER_PERSISTENT && !sAllowD3D9) {
        MOZ_ASSERT(!mLayerManager || !mLayerManager->IsInTransaction());

        
        
        nsToolkit::StartAllowingD3D9();
      }

#ifdef MOZ_ENABLE_D3D10_LAYER
      if (!prefs.mPreferD3D9 && !prefs.mPreferOpenGL) {
        nsRefPtr<LayerManagerD3D10> layerManager =
          new LayerManagerD3D10(this);
        if (layerManager->Initialize(prefs.mForceAcceleration)) {
          mLayerManager = layerManager;
        }
      }
#endif
#ifdef MOZ_ENABLE_D3D9_LAYER
      if (!prefs.mPreferOpenGL && !mLayerManager && sAllowD3D9) {
        nsRefPtr<LayerManagerD3D9> layerManager =
          new LayerManagerD3D9(this);
        if (layerManager->Initialize(prefs.mForceAcceleration)) {
          mLayerManager = layerManager;
        }
      }
#endif
      if (!mLayerManager && prefs.mPreferOpenGL) {
        nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
        int32_t status = nsIGfxInfo::FEATURE_NO_INFO;

        if (gfxInfo && !prefs.mForceAcceleration) {
          gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_OPENGL_LAYERS, &status);
        }

        if (status == nsIGfxInfo::FEATURE_NO_INFO) {
          nsRefPtr<LayerManagerOGL> layerManager =
            new LayerManagerOGL(this);
          if (layerManager->Initialize()) {
            mLayerManager = layerManager;
          }

        } else {
          NS_WARNING("OpenGL accelerated layers are not supported on this system.");
        }
      }
    }

    
    if (!mLayerManager) {
      
      if (ShouldUseOffMainThreadCompositing()) {
        
        
        NS_ASSERTION(aShadowManager == nullptr, "Async Compositor not supported with e10s");
        CreateCompositor();
      }

      if (!mLayerManager)
        mLayerManager = CreateBasicLayerManager();
    }
  }

  NS_ASSERTION(mLayerManager, "Couldn't provide a valid layer manager.");

  return mLayerManager;
}









gfxASurface *nsWindow::GetThebesSurface()
{
#ifdef CAIRO_HAS_D2D_SURFACE
  if (mD2DWindowSurface) {
    return mD2DWindowSurface;
  }
#endif
  if (mPaintDC)
    return (new gfxWindowsSurface(mPaintDC));

#ifdef CAIRO_HAS_D2D_SURFACE
  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
    gfxASurface::gfxContentType content = gfxASurface::CONTENT_COLOR;
#if defined(MOZ_XUL)
    if (mTransparencyMode != eTransparencyOpaque) {
      content = gfxASurface::CONTENT_COLOR_ALPHA;
    }
#endif
    return (new gfxD2DSurface(mWnd, content));
  } else {
#endif
    uint32_t flags = gfxWindowsSurface::FLAG_TAKE_DC;
    if (mTransparencyMode != eTransparencyOpaque) {
        flags |= gfxWindowsSurface::FLAG_IS_TRANSPARENT;
    }
    return (new gfxWindowsSurface(mWnd, flags));
#ifdef CAIRO_HAS_D2D_SURFACE
  }
#endif
}








 
NS_IMETHODIMP
nsWindow::OnDefaultButtonLoaded(const nsIntRect &aButtonRect)
{
  if (aButtonRect.IsEmpty())
    return NS_OK;

  
  HWND activeWnd = ::GetActiveWindow();
  if (activeWnd != ::GetForegroundWindow() ||
      WinUtils::GetTopLevelHWND(mWnd, true) !=
        WinUtils::GetTopLevelHWND(activeWnd, true)) {
    return NS_OK;
  }

  bool isAlwaysSnapCursor =
    Preferences::GetBool("ui.cursor_snapping.always_enabled", false);

  if (!isAlwaysSnapCursor) {
    BOOL snapDefaultButton;
    if (!::SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0,
                                &snapDefaultButton, 0) || !snapDefaultButton)
      return NS_OK;
  }

  nsIntRect widgetRect;
  nsresult rv = GetScreenBounds(widgetRect);
  NS_ENSURE_SUCCESS(rv, rv);
  nsIntRect buttonRect(aButtonRect + widgetRect.TopLeft());

  nsIntPoint centerOfButton(buttonRect.x + buttonRect.width / 2,
                            buttonRect.y + buttonRect.height / 2);
  
  
  if (!widgetRect.Contains(centerOfButton)) {
    return NS_OK;
  }

  if (!::SetCursorPos(centerOfButton.x, centerOfButton.y)) {
    NS_ERROR("SetCursorPos failed");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::OverrideSystemMouseScrollSpeed(double aOriginalDeltaX,
                                         double aOriginalDeltaY,
                                         double& aOverriddenDeltaX,
                                         double& aOverriddenDeltaY)
{
  
  
  const uint32_t kSystemDefaultScrollingSpeed = 3;

  double absOriginDeltaX = Abs(aOriginalDeltaX);
  double absOriginDeltaY = Abs(aOriginalDeltaY);

  
  double absComputedOverriddenDeltaX, absComputedOverriddenDeltaY;
  nsresult rv =
    nsBaseWidget::OverrideSystemMouseScrollSpeed(absOriginDeltaX,
                                                 absOriginDeltaY,
                                                 absComputedOverriddenDeltaX,
                                                 absComputedOverriddenDeltaY);
  NS_ENSURE_SUCCESS(rv, rv);

  aOverriddenDeltaX = aOriginalDeltaX;
  aOverriddenDeltaY = aOriginalDeltaY;

  if (absComputedOverriddenDeltaX == absOriginDeltaX &&
      absComputedOverriddenDeltaY == absOriginDeltaY) {
    
    return NS_OK;
  }

  
  
  UINT systemSpeed;
  if (!::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &systemSpeed, 0)) {
    return NS_ERROR_FAILURE;
  }
  
  
  if (systemSpeed != kSystemDefaultScrollingSpeed) {
    return NS_OK;
  }

  
  
  if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
    if (!::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &systemSpeed, 0)) {
      return NS_ERROR_FAILURE;
    }
    
    
    if (systemSpeed != kSystemDefaultScrollingSpeed) {
      return NS_OK;
    }
  }

  
  
  
  
  double absDeltaLimitX, absDeltaLimitY;
  rv =
    nsBaseWidget::OverrideSystemMouseScrollSpeed(kSystemDefaultScrollingSpeed,
                                                 kSystemDefaultScrollingSpeed,
                                                 absDeltaLimitX,
                                                 absDeltaLimitY);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (absDeltaLimitX <= absOriginDeltaX || absDeltaLimitY <= absOriginDeltaY) {
    return NS_OK;
  }

  aOverriddenDeltaX = std::min(absComputedOverriddenDeltaX, absDeltaLimitX);
  aOverriddenDeltaY = std::min(absComputedOverriddenDeltaY, absDeltaLimitY);

  if (aOriginalDeltaX < 0) {
    aOverriddenDeltaX *= -1;
  }
  if (aOriginalDeltaY < 0) {
    aOverriddenDeltaY *= -1;
  }
  return NS_OK;
}




















void nsWindow::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
  if (nullptr == aPoint) {     
    
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











NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
#ifdef WIDGET_DEBUG_OUTPUT
  debug_DumpEvent(stdout,
                  event->widget,
                  event,
                  nsAutoCString("something"),
                  (int32_t) mWnd);
#endif 

  aStatus = nsEventStatus_eIgnore;

  
  
  
  
  if (mAttachedWidgetListener) {
    aStatus = mAttachedWidgetListener->HandleEvent(event, mUseAttachedEvents);
  }
  else if (mWidgetListener) {
    aStatus = mWidgetListener->HandleEvent(event, mUseAttachedEvents);
  }

  
  
  
  if (mOnDestroyCalled)
    aStatus = nsEventStatus_eConsumeNoDefault;
  return NS_OK;
}

bool nsWindow::DispatchStandardEvent(uint32_t aMsg)
{
  nsGUIEvent event(true, aMsg, this);
  InitEvent(event);

  bool result = DispatchWindowEvent(&event);
  return result;
}

bool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
{
  nsEventStatus status;
  DispatchEvent(event, status);
  return ConvertStatus(status);
}

bool nsWindow::DispatchWindowEvent(nsGUIEvent* event, nsEventStatus &aStatus) {
  DispatchEvent(event, aStatus);
  return ConvertStatus(aStatus);
}

void nsWindow::InitKeyEvent(nsKeyEvent& aKeyEvent,
                            const NativeKey& aNativeKey,
                            const ModifierKeyState &aModKeyState)
{
  nsIntPoint point(0, 0);
  InitEvent(aKeyEvent, &point);
  aKeyEvent.location = aNativeKey.GetKeyLocation();
  aModKeyState.InitInputEvent(aKeyEvent);
}

bool nsWindow::DispatchKeyEvent(nsKeyEvent& aKeyEvent,
                                const MSG *aMsgSentToPlugin)
{
  UserActivity();

  NPEvent pluginEvent;
  if (aMsgSentToPlugin && PluginHasFocus()) {
    pluginEvent.event = aMsgSentToPlugin->message;
    pluginEvent.wParam = aMsgSentToPlugin->wParam;
    pluginEvent.lParam = aMsgSentToPlugin->lParam;
    aKeyEvent.pluginEvent = (void *)&pluginEvent;
  }

  return DispatchWindowEvent(&aKeyEvent);
}

bool nsWindow::DispatchCommandEvent(uint32_t aEventCommand)
{
  nsCOMPtr<nsIAtom> command;
  switch (aEventCommand) {
    case APPCOMMAND_BROWSER_BACKWARD:
      command = nsGkAtoms::Back;
      break;
    case APPCOMMAND_BROWSER_FORWARD:
      command = nsGkAtoms::Forward;
      break;
    case APPCOMMAND_BROWSER_REFRESH:
      command = nsGkAtoms::Reload;
      break;
    case APPCOMMAND_BROWSER_STOP:
      command = nsGkAtoms::Stop;
      break;
    case APPCOMMAND_BROWSER_SEARCH:
      command = nsGkAtoms::Search;
      break;
    case APPCOMMAND_BROWSER_FAVORITES:
      command = nsGkAtoms::Bookmarks;
      break;
    case APPCOMMAND_BROWSER_HOME:
      command = nsGkAtoms::Home;
      break;
    case APPCOMMAND_CLOSE:
      command = nsGkAtoms::Close;
      break;
    case APPCOMMAND_FIND:
      command = nsGkAtoms::Find;
      break;
    case APPCOMMAND_HELP:
      command = nsGkAtoms::Help;
      break;
    case APPCOMMAND_NEW:
      command = nsGkAtoms::New;
      break;
    case APPCOMMAND_OPEN:
      command = nsGkAtoms::Open;
      break;
    case APPCOMMAND_PRINT:
      command = nsGkAtoms::Print;
      break;
    case APPCOMMAND_SAVE:
      command = nsGkAtoms::Save;
      break;
    case APPCOMMAND_FORWARD_MAIL:
      command = nsGkAtoms::ForwardMail;
      break;
    case APPCOMMAND_REPLY_TO_MAIL:
      command = nsGkAtoms::ReplyToMail;
      break;
    case APPCOMMAND_SEND_MAIL:
      command = nsGkAtoms::SendMail;
      break;
    default:
      return false;
  }
  nsCommandEvent event(true, nsGkAtoms::onAppCommand, command, this);

  InitEvent(event);
  return DispatchWindowEvent(&event);
}



BOOL CALLBACK nsWindow::DispatchStarvedPaints(HWND aWnd, LPARAM aMsg)
{
  LONG_PTR proc = ::GetWindowLongPtrW(aWnd, GWLP_WNDPROC);
  if (proc == (LONG_PTR)&nsWindow::WindowProc) {
    
    
    
    if (GetUpdateRect(aWnd, NULL, FALSE))
      VERIFY(::UpdateWindow(aWnd));
  }
  return TRUE;
}







void nsWindow::DispatchPendingEvents()
{
  if (mPainting) {
    NS_WARNING("We were asked to dispatch pending events during painting, "
               "denying since that's unsafe.");
    return;
  }

  
  
  
  static int recursionBlocker = 0;
  if (recursionBlocker++ == 0) {
    NS_ProcessPendingEvents(nullptr, PR_MillisecondsToInterval(100));
    --recursionBlocker;
  }

  
  
  
  if (::GetQueueStatus(QS_PAINT) &&
      ((TimeStamp::Now() - mLastPaintEndTime).ToMilliseconds() >= 50)) {
    
    HWND topWnd = WinUtils::GetTopLevelHWND(mWnd);

    
    
    
    nsWindow::DispatchStarvedPaints(topWnd, 0);
    ::EnumChildWindows(topWnd, nsWindow::DispatchStarvedPaints, 0);
  }
}


bool nsWindow::DispatchPluginEvent(const MSG &aMsg)
{
  if (!PluginHasFocus())
    return false;

  nsPluginEvent event(true, NS_PLUGIN_INPUT_EVENT, this);
  nsIntPoint point(0, 0);
  InitEvent(event, &point);
  NPEvent pluginEvent;
  pluginEvent.event = aMsg.message;
  pluginEvent.wParam = aMsg.wParam;
  pluginEvent.lParam = aMsg.lParam;
  event.pluginEvent = (void *)&pluginEvent;
  event.retargetToFocusedDocument = true;
  return DispatchWindowEvent(&event);
}

bool nsWindow::DispatchPluginEvent(UINT aMessage,
                                     WPARAM aWParam,
                                     LPARAM aLParam,
                                     bool aDispatchPendingEvents)
{
  bool ret = DispatchPluginEvent(WinUtils::InitMSG(aMessage, aWParam, aLParam));
  if (aDispatchPendingEvents) {
    DispatchPendingEvents();
  }
  return ret;
}

void nsWindow::RemoveMessageAndDispatchPluginEvent(UINT aFirstMsg,
                 UINT aLastMsg, nsFakeCharMessage* aFakeCharMessage)
{
  MSG msg;
  if (aFakeCharMessage) {
    if (aFirstMsg > WM_CHAR || aLastMsg < WM_CHAR) {
      return;
    }
    msg = aFakeCharMessage->GetCharMessage(mWnd);
  } else {
    WinUtils::GetMessage(&msg, mWnd, aFirstMsg, aLastMsg);
  }
  DispatchPluginEvent(msg);
}


bool nsWindow::DispatchMouseEvent(uint32_t aEventType, WPARAM wParam,
                                    LPARAM lParam, bool aIsContextMenuKey,
                                    int16_t aButton, uint16_t aInputSource)
{
  bool result = false;

  UserActivity();

  if (!mWidgetListener) {
    return result;
  }

  switch (aEventType) {
    case NS_MOUSE_BUTTON_DOWN:
      CaptureMouse(true);
      break;

    
    
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_MOVE:
    case NS_MOUSE_EXIT:
      if (!(wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)) && sIsInMouseCapture)
        CaptureMouse(false);
      break;

    default:
      break;

  } 

  nsIntPoint eventPoint;
  eventPoint.x = GET_X_LPARAM(lParam);
  eventPoint.y = GET_Y_LPARAM(lParam);

  nsMouseEvent event(true, aEventType, this, nsMouseEvent::eReal,
                     aIsContextMenuKey
                     ? nsMouseEvent::eContextMenuKey
                     : nsMouseEvent::eNormal);
  if (aEventType == NS_CONTEXTMENU && aIsContextMenuKey) {
    nsIntPoint zero(0, 0);
    InitEvent(event, &zero);
  } else {
    InitEvent(event, &eventPoint);
  }

  ModifierKeyState modifierKeyState;
  modifierKeyState.InitInputEvent(event);
  event.button    = aButton;
  event.inputSource = aInputSource;

  nsIntPoint mpScreen = eventPoint + WidgetToScreenOffset();

  
  if (aEventType == NS_MOUSE_MOVE) 
  {
    if ((sLastMouseMovePoint.x == mpScreen.x) && (sLastMouseMovePoint.y == mpScreen.y))
      return result;
    sLastMouseMovePoint.x = mpScreen.x;
    sLastMouseMovePoint.y = mpScreen.y;
  }

  bool insideMovementThreshold = (DeprecatedAbs(sLastMousePoint.x - eventPoint.x) < (short)::GetSystemMetrics(SM_CXDOUBLECLK)) &&
                                   (DeprecatedAbs(sLastMousePoint.y - eventPoint.y) < (short)::GetSystemMetrics(SM_CYDOUBLECLK));

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
    sLastClickCount = 2;
  }
  else if (aEventType == NS_MOUSE_BUTTON_UP) {
    
    sLastMousePoint.x = eventPoint.x;
    sLastMousePoint.y = eventPoint.y;
    sLastMouseButton = eventButton;
  }
  else if (aEventType == NS_MOUSE_BUTTON_DOWN) {
    
    if (((curMsgTime - sLastMouseDownTime) < (LONG)::GetDoubleClickTime()) && insideMovementThreshold &&
        eventButton == sLastMouseButton) {
      sLastClickCount ++;
    } else {
      
      sLastClickCount = 1;
    }
    
    sLastMouseDownTime = curMsgTime;
  }
  else if (aEventType == NS_MOUSE_MOVE && !insideMovementThreshold) {
    sLastClickCount = 0;
  }
  else if (aEventType == NS_MOUSE_EXIT) {
    event.exit = IsTopLevelMouseExit(mWnd) ? nsMouseEvent::eTopLevel : nsMouseEvent::eChild;
  }
  else if (aEventType == NS_MOUSE_MOZHITTEST)
  {
    event.mFlags.mOnlyChromeDispatch = true;
  }
  event.clickCount = sLastClickCount;

#ifdef NS_DEBUG_XX
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("Msg Time: %d Click Count: %d\n", curMsgTime, event.clickCount));
#endif

  NPEvent pluginEvent;

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
    case NS_MOUSE_EXIT:
      pluginEvent.event = WM_MOUSELEAVE;
      break;
    default:
      pluginEvent.event = WM_NULL;
      break;
  }

  pluginEvent.wParam = wParam;     
  pluginEvent.lParam = lParam;

  event.pluginEvent = (void *)&pluginEvent;

  
  if (mWidgetListener) {
    if (nsToolkit::gMouseTrailer)
      nsToolkit::gMouseTrailer->Disable();
    if (aEventType == NS_MOUSE_MOVE) {
      if (nsToolkit::gMouseTrailer && !sIsInMouseCapture) {
        nsToolkit::gMouseTrailer->SetMouseTrailerWindow(mWnd);
      }
      nsIntRect rect;
      GetBounds(rect);
      rect.x = 0;
      rect.y = 0;

      if (rect.Contains(event.refPoint)) {
        if (sCurrentWindow == NULL || sCurrentWindow != this) {
          if ((nullptr != sCurrentWindow) && (!sCurrentWindow->mInDtor)) {
            LPARAM pos = sCurrentWindow->lParamToClient(lParamToScreen(lParam));
            sCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT, wParam, pos, false, 
                                               nsMouseEvent::eLeftButton, aInputSource);
          }
          sCurrentWindow = this;
          if (!mInDtor) {
            LPARAM pos = sCurrentWindow->lParamToClient(lParamToScreen(lParam));
            sCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER, wParam, pos, false,
                                               nsMouseEvent::eLeftButton, aInputSource);
          }
        }
      }
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (sCurrentWindow == this) {
        sCurrentWindow = nullptr;
      }
    }

    result = DispatchWindowEvent(&event);

    if (nsToolkit::gMouseTrailer)
      nsToolkit::gMouseTrailer->Enable();

    
    
    
    return result;
  }

  return result;
}

void nsWindow::DispatchFocusToTopLevelWindow(bool aIsActivate)
{
  if (aIsActivate)
    sJustGotActivate = false;
  sJustGotDeactivate = false;

  
  HWND curWnd = mWnd;
  HWND toplevelWnd = NULL;
  while (curWnd) {
    toplevelWnd = curWnd;

    nsWindow *win = WinUtils::GetNSWindowPtr(curWnd);
    if (win) {
      nsWindowType wintype;
      win->GetWindowType(wintype);
      if (wintype == eWindowType_toplevel || wintype == eWindowType_dialog)
        break;
    }

    curWnd = ::GetParent(curWnd); 
  }

  if (toplevelWnd) {
    nsWindow *win = WinUtils::GetNSWindowPtr(toplevelWnd);
    if (win && win->mWidgetListener) {
      if (aIsActivate) {
        win->mWidgetListener->WindowActivated();
      } else {
        if (!win->BlurEventsSuppressed()) {
          win->mWidgetListener->WindowDeactivated();
        }
      }
    }
  }
}

bool nsWindow::IsTopLevelMouseExit(HWND aWnd)
{
  DWORD pos = ::GetMessagePos();
  POINT mp;
  mp.x = GET_X_LPARAM(pos);
  mp.y = GET_Y_LPARAM(pos);
  HWND mouseWnd = ::WindowFromPoint(mp);

  
  
  
  HWND mouseTopLevel = WinUtils::GetTopLevelHWND(mouseWnd);
  if (mouseWnd == mouseTopLevel)
    return true;

  return WinUtils::GetTopLevelHWND(aWnd) != mouseTopLevel;
}

bool nsWindow::BlurEventsSuppressed()
{
  
  if (mBlurSuppressLevel > 0)
    return true;

  
  HWND parentWnd = ::GetParent(mWnd);
  if (parentWnd) {
    nsWindow *parent = WinUtils::GetNSWindowPtr(parentWnd);
    if (parent)
      return parent->BlurEventsSuppressed();
  }
  return false;
}




void nsWindow::SuppressBlurEvents(bool aSuppress)
{
  if (aSuppress)
    ++mBlurSuppressLevel; 
  else {
    NS_ASSERTION(mBlurSuppressLevel > 0, "unbalanced blur event suppression");
    if (mBlurSuppressLevel > 0)
      --mBlurSuppressLevel;
  }
}

bool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  return aStatus == nsEventStatus_eConsumeNoDefault;
}










bool
nsWindow::IsAsyncResponseEvent(UINT aMsg, LRESULT& aResult)
{
  switch(aMsg) {
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_PARENTNOTIFY:
    case WM_ACTIVATEAPP:
    case WM_NCACTIVATE:
    case WM_ACTIVATE:
    case WM_CHILDACTIVATE:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_SHOWWINDOW:
    case WM_CANCELMODE:
    case WM_MOUSEACTIVATE:
    case WM_CONTEXTMENU:
      aResult = 0;
    return true;

    case WM_SETTINGCHANGE:
    case WM_SETCURSOR:
    return false;
  }

#ifdef DEBUG
  char szBuf[200];
  sprintf(szBuf,
    "An unhandled ISMEX_SEND message was received during spin loop! (%X)", aMsg);
  NS_WARNING(szBuf);
#endif

  return false;
}

void
nsWindow::IPCWindowProcHandler(UINT& msg, WPARAM& wParam, LPARAM& lParam)
{
  NS_ASSERTION(!mozilla::ipc::SyncChannel::IsPumpingMessages(),
               "Failed to prevent a nonqueued message from running!");

  
  if (mozilla::ipc::RPCChannel::IsSpinLoopActive() &&
      (InSendMessageEx(NULL)&(ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
    LRESULT res;
    if (IsAsyncResponseEvent(msg, res)) {
      ReplyMessage(res);
    }
    return;
  }

  
  

  DWORD dwResult = 0;
  bool handled = false;

  switch(msg) {
    
    
    case WM_ACTIVATE:
      if (lParam != 0 && LOWORD(wParam) == WA_ACTIVE &&
          IsWindow((HWND)lParam)) {
        
        
        if ((InSendMessageEx(NULL)&(ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
          PRUnichar szClass[10];
          HWND focusWnd = (HWND)lParam;
          if (IsWindowVisible(focusWnd) &&
              GetClassNameW(focusWnd, szClass,
                            sizeof(szClass)/sizeof(PRUnichar)) &&
              !wcscmp(szClass, L"Edit") &&
              !WinUtils::IsOurProcessWindow(focusWnd)) {
            break;
          }
        }
        handled = true;
      }
    break;
    
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    
    
    case WM_SYSCOMMAND:
    
    
    case WM_CONTEXTMENU:
    
    case WM_IME_SETCONTEXT:
      handled = true;
    break;
  }

  if (handled &&
      (InSendMessageEx(NULL)&(ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
    ReplyMessage(dwResult);
  }
}





















static bool
DisplaySystemMenu(HWND hWnd, nsSizeMode sizeMode, bool isRtl, int32_t x, int32_t y)
{
  HMENU hMenu = GetSystemMenu(hWnd, FALSE);
  if (hMenu) {
    MENUITEMINFO mii;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fType = 0;

    
    mii.fState = MF_ENABLED;
    SetMenuItemInfo(hMenu, SC_RESTORE, FALSE, &mii);
    SetMenuItemInfo(hMenu, SC_SIZE, FALSE, &mii);
    SetMenuItemInfo(hMenu, SC_MOVE, FALSE, &mii);
    SetMenuItemInfo(hMenu, SC_MAXIMIZE, FALSE, &mii);
    SetMenuItemInfo(hMenu, SC_MINIMIZE, FALSE, &mii);

    mii.fState = MF_GRAYED;
    switch(sizeMode) {
      case nsSizeMode_Fullscreen:
        SetMenuItemInfo(hMenu, SC_RESTORE, FALSE, &mii);
        
      case nsSizeMode_Maximized:
        SetMenuItemInfo(hMenu, SC_SIZE, FALSE, &mii);
        SetMenuItemInfo(hMenu, SC_MOVE, FALSE, &mii);
        SetMenuItemInfo(hMenu, SC_MAXIMIZE, FALSE, &mii);
        break;
      case nsSizeMode_Minimized:
        SetMenuItemInfo(hMenu, SC_MINIMIZE, FALSE, &mii);
        break;
      case nsSizeMode_Normal:
        SetMenuItemInfo(hMenu, SC_RESTORE, FALSE, &mii);
        break;
    }
    LPARAM cmd =
      TrackPopupMenu(hMenu,
                     (TPM_LEFTBUTTON|TPM_RIGHTBUTTON|
                      TPM_RETURNCMD|TPM_TOPALIGN|
                      (isRtl ? TPM_RIGHTALIGN : TPM_LEFTALIGN)),
                     x, y, 0, hWnd, NULL);
    if (cmd) {
      PostMessage(hWnd, WM_SYSCOMMAND, cmd, 0);
      return true;
    }
  }
  return false;
}

inline static mozilla::HangMonitor::ActivityType ActivityTypeForMessage(UINT msg)
{
  if ((msg >= WM_KEYFIRST && msg <= WM_IME_KEYLAST) ||
      (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ||
      (msg >= MOZ_WM_MOUSEWHEEL_FIRST && msg <= MOZ_WM_MOUSEWHEEL_LAST) ||
      (msg >= NS_WM_IMEFIRST && msg <= NS_WM_IMELAST)) {
    return mozilla::HangMonitor::kUIActivity;
  }

  
  
  return mozilla::HangMonitor::kActivityUIAVail;
}




LRESULT CALLBACK nsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HangMonitor::NotifyActivity(ActivityTypeForMessage(msg));

  return mozilla::CallWindowProcCrashProtected(WindowProcInternal, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK nsWindow::WindowProcInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (::GetWindowLongPtrW(hWnd, GWLP_ID) == eFakeTrackPointScrollableID) {
    
    if (msg == WM_HSCROLL) {
      
      hWnd = ::GetParent(::GetParent(hWnd));
    } else {
      
      WNDPROC prevWindowProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
      return ::CallWindowProcW(prevWindowProc, hWnd, msg, wParam, lParam);
    }
  }

  if (msg == MOZ_WM_TRACE) {
    
    
    mozilla::SignalTracerThread();
    return 0;
  }

  
  nsWindow *targetWindow = WinUtils::GetNSWindowPtr(hWnd);
  NS_ASSERTION(targetWindow, "nsWindow* is null!");
  if (!targetWindow)
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);

  
  
  nsCOMPtr<nsISupports> kungFuDeathGrip;
  if (!targetWindow->mInDtor)
    kungFuDeathGrip = do_QueryInterface((nsBaseWidget*)targetWindow);

  targetWindow->IPCWindowProcHandler(msg, wParam, lParam);

  
  
  nsAutoRollup autoRollup;

  LRESULT popupHandlingResult;
  if (DealWithPopups(hWnd, msg, wParam, lParam, &popupHandlingResult))
    return popupHandlingResult;

  
  LRESULT retValue;
  if (targetWindow->ProcessMessage(msg, wParam, lParam, &retValue)) {
    return retValue;
  }

  LRESULT res = ::CallWindowProcW(targetWindow->GetPrevWindowProc(),
                                  hWnd, msg, wParam, lParam);

  return res;
}





bool
nsWindow::ProcessMessageForPlugin(const MSG &aMsg,
                                  LRESULT *aResult,
                                  bool &aCallDefWndProc)
{
  NS_PRECONDITION(aResult, "aResult must be non-null.");
  *aResult = 0;

  aCallDefWndProc = false;
  bool eventDispatched = false;
  switch (aMsg.message) {
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

    case WM_CUT:
    case WM_COPY:
    case WM_PASTE:
    case WM_CLEAR:
    case WM_UNDO:
      break;

    default:
      return false;
  }

  if (!eventDispatched)
    aCallDefWndProc = !DispatchPluginEvent(aMsg);
  DispatchPendingEvents();
  return true;
}

static void ForceFontUpdate()
{
  
  
  
  
  
  static const char kPrefName[] = "font.internaluseonly.changed";
  bool fontInternalChange =
    Preferences::GetBool(kPrefName, false);
  Preferences::SetBool(kPrefName, !fontInternalChange);
}

static bool CleartypeSettingChanged()
{
  static int currentQuality = -1;
  BYTE quality = cairo_win32_get_system_text_quality();

  if (currentQuality == quality)
    return false;

  if (currentQuality < 0) {
    currentQuality = quality;
    return false;
  }
  currentQuality = quality;
  return true;
}


bool nsWindow::ProcessMessage(UINT msg, WPARAM &wParam, LPARAM &lParam,
                                LRESULT *aRetValue)
{
  
  if (mWindowHook.Notify(mWnd, msg, wParam, lParam, aRetValue))
    return true;

#if defined(EVENT_DEBUG_OUTPUT)
  
  
  PrintEvent(msg, SHOW_REPEAT_EVENTS, SHOW_MOUSEMOVE_EVENTS);
#endif

  bool eatMessage;
  if (IMEHandler::ProcessMessage(this, msg, wParam, lParam, aRetValue,
                                 eatMessage)) {
    return mWnd ? eatMessage : true;
  }

  if (MouseScrollHandler::ProcessMessage(this, msg, wParam, lParam, aRetValue,
                                         eatMessage)) {
    return mWnd ? eatMessage : true;
  }

  if (PluginHasFocus()) {
    bool callDefaultWndProc;
    MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam);
    if (ProcessMessageForPlugin(nativeMsg, aRetValue, callDefaultWndProc)) {
      return mWnd ? !callDefaultWndProc : true;
    }
  }

  bool result = false;    
  *aRetValue = 0;

  
  LRESULT dwmHitResult;
  if (mCustomNonClient &&
      nsUXThemeData::CheckForCompositor() &&
      nsUXThemeData::dwmDwmDefWindowProcPtr(mWnd, msg, wParam, lParam, &dwmHitResult)) {
    *aRetValue = dwmHitResult;
    return true;
  }

  switch (msg) {
    
    
    case WM_QUERYENDSESSION:
      if (sCanQuit == TRI_UNKNOWN)
      {
        
        
        nsCOMPtr<nsIObserverService> obsServ =
          mozilla::services::GetObserverService();
        nsCOMPtr<nsISupportsPRBool> cancelQuit =
          do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
        cancelQuit->SetData(false);
        obsServ->NotifyObservers(cancelQuit, "quit-application-requested", nullptr);

        bool abortQuit;
        cancelQuit->GetData(&abortQuit);
        sCanQuit = abortQuit ? TRI_FALSE : TRI_TRUE;
      }
      *aRetValue = sCanQuit ? TRUE : FALSE;
      result = true;
      break;

    case WM_ENDSESSION:
    case MOZ_WM_APP_QUIT:
      if (msg == MOZ_WM_APP_QUIT || (wParam == TRUE && sCanQuit == TRI_TRUE))
      {
        
        
        
        
        nsCOMPtr<nsIObserverService> obsServ =
          mozilla::services::GetObserverService();
        NS_NAMED_LITERAL_STRING(context, "shutdown-persist");
        obsServ->NotifyObservers(nullptr, "quit-application-granted", nullptr);
        obsServ->NotifyObservers(nullptr, "quit-application-forced", nullptr);
        obsServ->NotifyObservers(nullptr, "quit-application", nullptr);
        obsServ->NotifyObservers(nullptr, "profile-change-net-teardown", context.get());
        obsServ->NotifyObservers(nullptr, "profile-change-teardown", context.get());
        obsServ->NotifyObservers(nullptr, "profile-before-change", context.get());
        obsServ->NotifyObservers(nullptr, "profile-before-change2", context.get());
        
        _exit(0);
      }
      sCanQuit = TRI_UNKNOWN;
      result = true;
      break;

    case WM_SYSCOLORCHANGE:
      OnSysColorChanged();
      break;

    case WM_THEMECHANGED:
    {
      
      UpdateNonClientMargins();
      nsUXThemeData::InitTitlebarInfo();
      nsUXThemeData::UpdateNativeThemeInfo();

      NotifyThemeChanged();

      
      
      Invalidate(true, true, true);
    }
    break;

    case WM_FONTCHANGE:
    {
      
      
      
      if (mWindowType != eWindowType_invisible) {
        break;
      }

      nsresult rv;
      bool didChange = false;

      
      nsCOMPtr<nsIFontEnumerator> fontEnum = do_GetService("@mozilla.org/gfx/fontenumerator;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        fontEnum->UpdateFontList(&didChange);
        ForceFontUpdate();
      } 
    }
    break;

    case WM_NCCALCSIZE:
    {
      if (mCustomNonClient) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        RECT *clientRect = wParam
                         ? &(reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam))->rgrc[0]
                         : (reinterpret_cast<RECT*>(lParam));
        clientRect->top      += (mCaptionHeight - mNonClientOffset.top);
        clientRect->left     += (mHorResizeMargin - mNonClientOffset.left);
        clientRect->right    -= (mHorResizeMargin - mNonClientOffset.right);
        clientRect->bottom   -= (mVertResizeMargin - mNonClientOffset.bottom);

        result = true;
        *aRetValue = 0;
      }
      break;
    }

    case WM_NCHITTEST:
    {
      







      if (!mCustomNonClient)
        break;

      *aRetValue =
        ClientMarginHitTestPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      result = true;
      break;
    }

    case WM_SETTEXT:
      




      if (!mCustomNonClient || mNonClientMargins.top == -1)
        break;

      {
        
        
        
        DWORD style = GetWindowLong(mWnd, GWL_STYLE);
        SetWindowLong(mWnd, GWL_STYLE, style & ~WS_VISIBLE);
        *aRetValue = CallWindowProcW(GetPrevWindowProc(), mWnd,
                                     msg, wParam, lParam);
        SetWindowLong(mWnd, GWL_STYLE, style);
        return true;
      }

    case WM_NCACTIVATE:
    {
      




      if (!mCustomNonClient)
        break;

      
      if(nsUXThemeData::CheckForCompositor())
        break;

      if (wParam == TRUE) {
        
        *aRetValue = FALSE; 
        result = true;
        UpdateGetWindowInfoCaptionStatus(true);
        
        InvalidateNonClientRegion();
        break;
      } else {
        
        *aRetValue = TRUE; 
        result = true;
        UpdateGetWindowInfoCaptionStatus(false);
        
        InvalidateNonClientRegion();
        break;
      }
    }

    case WM_NCPAINT:
    {
      





      if (!mCustomNonClient)
        break;

      
      if(nsUXThemeData::CheckForCompositor())
        break;

      HRGN paintRgn = ExcludeNonClientFromPaintRegion((HRGN)wParam);
      LRESULT res = CallWindowProcW(GetPrevWindowProc(), mWnd,
                                    msg, (WPARAM)paintRgn, lParam);
      if (paintRgn != (HRGN)wParam)
        DeleteObject(paintRgn);
      *aRetValue = res;
      result = true;
    }
    break;

    case WM_POWERBROADCAST:
      switch (wParam)
      {
        case PBT_APMSUSPEND:
          PostSleepWakeNotification(true);
          break;
        case PBT_APMRESUMEAUTOMATIC:
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
          PostSleepWakeNotification(false);
          break;
      }
      break;

    case WM_MOVE: 
    {
      RECT rect;
      ::GetWindowRect(mWnd, &rect);
      result = OnMove(rect.left, rect.top);
    }
    break;

    case WM_CLOSE: 
      if (mWidgetListener)
        mWidgetListener->RequestWindowClose(this);
      result = true; 
      break;

    case WM_DESTROY:
      
      OnDestroy();
      result = true;
      break;

    case WM_PAINT:
      if (CleartypeSettingChanged()) {
        ForceFontUpdate();
        gfxFontCache *fc = gfxFontCache::GetCache();
        if (fc) {
          fc->Flush();
        }
      }
      *aRetValue = (int) OnPaint(NULL, 0);
      result = true;
      break;

    case WM_PRINTCLIENT:
      result = OnPaint((HDC) wParam, 0);
      break;

    case WM_HOTKEY:
      result = OnHotKey(wParam, lParam);
      break;

    case WM_SYSCHAR:
    case WM_CHAR:
    {
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam);
      result = ProcessCharMessage(nativeMsg, nullptr);
      DispatchPendingEvents();
    }
    break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam);
      nativeMsg.time = ::GetMessageTime();
      result = ProcessKeyUpMessage(nativeMsg, nullptr);
      DispatchPendingEvents();
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam);
      result = ProcessKeyDownMessage(nativeMsg, nullptr);
      DispatchPendingEvents();
    }
    break;

    
    
    case WM_ERASEBKGND:
      if (!AutoErase((HDC)wParam)) {
        *aRetValue = 1;
        result = true;
      }
      break;

    case WM_MOUSEMOVE:
    {
      mMousePresent = true;

      
      
      
      LPARAM lParamScreen = lParamToScreen(lParam);
      POINT mp;
      mp.x      = GET_X_LPARAM(lParamScreen);
      mp.y      = GET_Y_LPARAM(lParamScreen);
      bool userMovedMouse = false;
      if ((sLastMouseMovePoint.x != mp.x) || (sLastMouseMovePoint.y != mp.y)) {
        userMovedMouse = true;
      }

      result = DispatchMouseEvent(NS_MOUSE_MOVE, wParam, lParam,
                                  false, nsMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
      if (userMovedMouse) {
        DispatchPendingEvents();
      }
    }
    break;

    case WM_NCMOUSEMOVE:
      
      
      if (mMousePresent && !sIsInMouseCapture)
        SendMessage(mWnd, WM_MOUSELEAVE, 0, 0);
    break;

    case WM_LBUTTONDOWN:
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam,
                                  false, nsMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
    }
    break;

    case WM_LBUTTONUP:
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam,
                                  false, nsMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
    }
    break;

    case WM_MOUSELEAVE:
    {
      if (!mMousePresent)
        break;
      mMousePresent = false;

      
      
      WPARAM mouseState = (GetKeyState(VK_LBUTTON) ? MK_LBUTTON : 0)
        | (GetKeyState(VK_MBUTTON) ? MK_MBUTTON : 0)
        | (GetKeyState(VK_RBUTTON) ? MK_RBUTTON : 0);
      
      
      LPARAM pos = lParamToClient(::GetMessagePos());
      DispatchMouseEvent(NS_MOUSE_EXIT, mouseState, pos, false,
                         nsMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
    }
    break;

    case WM_CONTEXTMENU:
    {
      
      
      LPARAM pos;
      bool contextMenukey = false;
      if (lParam == -1)
      {
        contextMenukey = true;
        pos = lParamToClient(GetMessagePos());
      }
      else
      {
        pos = lParamToClient(lParam);
      }

      result = DispatchMouseEvent(NS_CONTEXTMENU, wParam, pos, contextMenukey,
                                  contextMenukey ?
                                    nsMouseEvent::eLeftButton :
                                    nsMouseEvent::eRightButton, MOUSE_INPUT_SOURCE());
      if (lParam != -1 && !result && mCustomNonClient &&
          DispatchMouseEvent(NS_MOUSE_MOZHITTEST, wParam, pos,
                             false, nsMouseEvent::eLeftButton,
                             MOUSE_INPUT_SOURCE())) {
        
        DisplaySystemMenu(mWnd, mSizeMode, mIsRTL, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        result = true;
      }
    }
    break;

    case WM_LBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam, lParam, false,
                                  nsMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_MBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam, false,
                                  nsMouseEvent::eMiddleButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_MBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam, false,
                                  nsMouseEvent::eMiddleButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_MBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam, lParam, false,
                                  nsMouseEvent::eMiddleButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCMBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, 0, lParamToClient(lParam), false,
                                  nsMouseEvent::eMiddleButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCMBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, 0, lParamToClient(lParam), false,
                                  nsMouseEvent::eMiddleButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCMBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, 0, lParamToClient(lParam), false,
                                  nsMouseEvent::eMiddleButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_RBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam, false,
                                  nsMouseEvent::eRightButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_RBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam, false,
                                  nsMouseEvent::eRightButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_RBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam, lParam, false,
                                  nsMouseEvent::eRightButton, MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCRBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, 0, lParamToClient(lParam), 
                                  false, nsMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCRBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, 0, lParamToClient(lParam),
                                  false, nsMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCRBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, 0, lParamToClient(lParam),
                                  false, nsMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_EXITSIZEMOVE:
      if (!sIsInMouseCapture) {
        NotifySizeMoveDone();
      }
      break;

    case WM_NCLBUTTONDBLCLK:
      DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, 0, lParamToClient(lParam),
                         false, nsMouseEvent::eLeftButton,
                         MOUSE_INPUT_SOURCE());
      result = 
        DispatchMouseEvent(NS_MOUSE_BUTTON_UP, 0, lParamToClient(lParam),
                           false, nsMouseEvent::eLeftButton,
                           MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_APPCOMMAND:
    {
      uint32_t appCommand = GET_APPCOMMAND_LPARAM(lParam);
      uint32_t contentCommandMessage = NS_EVENT_NULL;
      
      
      switch (appCommand)
      {
        case APPCOMMAND_BROWSER_BACKWARD:
        case APPCOMMAND_BROWSER_FORWARD:
        case APPCOMMAND_BROWSER_REFRESH:
        case APPCOMMAND_BROWSER_STOP:
        case APPCOMMAND_BROWSER_SEARCH:
        case APPCOMMAND_BROWSER_FAVORITES:
        case APPCOMMAND_BROWSER_HOME:
        case APPCOMMAND_CLOSE:
        case APPCOMMAND_FIND:
        case APPCOMMAND_HELP:
        case APPCOMMAND_NEW:
        case APPCOMMAND_OPEN:
        case APPCOMMAND_PRINT:
        case APPCOMMAND_SAVE:
        case APPCOMMAND_FORWARD_MAIL:
        case APPCOMMAND_REPLY_TO_MAIL:
        case APPCOMMAND_SEND_MAIL:
          
          
          
          if (DispatchCommandEvent(appCommand)) {
            
            *aRetValue = 1;
            result = true;
          }
          break;

        
        case APPCOMMAND_COPY:
          contentCommandMessage = NS_CONTENT_COMMAND_COPY;
          break;
        case APPCOMMAND_CUT:
          contentCommandMessage = NS_CONTENT_COMMAND_CUT;
          break;
        case APPCOMMAND_PASTE:
          contentCommandMessage = NS_CONTENT_COMMAND_PASTE;
          break;
        case APPCOMMAND_REDO:
          contentCommandMessage = NS_CONTENT_COMMAND_REDO;
          break;
        case APPCOMMAND_UNDO:
          contentCommandMessage = NS_CONTENT_COMMAND_UNDO;
          break;
      }

      if (contentCommandMessage) {
        nsContentCommandEvent contentCommand(true, contentCommandMessage, this);
        DispatchWindowEvent(&contentCommand);
        
        *aRetValue = 1;
        result = true;
      }
      
    }
    break;

    
    
    
    
    
    
    case WM_ACTIVATE:
      if (mWidgetListener) {
        int32_t fActive = LOWORD(wParam);

        if (WA_INACTIVE == fActive) {
          
          
          if (HIWORD(wParam))
            DispatchFocusToTopLevelWindow(false);
          else
            sJustGotDeactivate = true;

          if (mIsTopWidgetWindow)
            mLastKeyboardLayout = gKbdLayout.GetLayout();

        } else {
          StopFlashing();

          sJustGotActivate = true;
          nsMouseEvent event(true, NS_MOUSE_ACTIVATE, this,
                             nsMouseEvent::eReal);
          InitEvent(event);
          ModifierKeyState modifierKeyState;
          modifierKeyState.InitInputEvent(event);
          DispatchWindowEvent(&event);
          if (sSwitchKeyboardLayout && mLastKeyboardLayout)
            ActivateKeyboardLayout(mLastKeyboardLayout, 0);
        }
      }
      break;
      
    case WM_MOUSEACTIVATE:
      if (mWindowType == eWindowType_popup) {
        
        
        
        
        HWND owner = ::GetWindow(mWnd, GW_OWNER);
        if (owner && owner == ::GetForegroundWindow()) {
          *aRetValue = MA_NOACTIVATE;
          result = true;
        }
      }
      break;

    case WM_WINDOWPOSCHANGING:
    {
      LPWINDOWPOS info = (LPWINDOWPOS) lParam;
      OnWindowPosChanging(info);
    }
    break;

    case WM_GETMINMAXINFO:
    {
      MINMAXINFO* mmi = (MINMAXINFO*)lParam;
      
      
      mmi->ptMinTrackSize.x =
        std::min((int32_t)mmi->ptMaxTrackSize.x,
               std::max((int32_t)mmi->ptMinTrackSize.x, mSizeConstraints.mMinSize.width));
      mmi->ptMinTrackSize.y =
        std::min((int32_t)mmi->ptMaxTrackSize.y,
        std::max((int32_t)mmi->ptMinTrackSize.y, mSizeConstraints.mMinSize.height));
      mmi->ptMaxTrackSize.x = std::min((int32_t)mmi->ptMaxTrackSize.x, mSizeConstraints.mMaxSize.width);
      mmi->ptMaxTrackSize.y = std::min((int32_t)mmi->ptMaxTrackSize.y, mSizeConstraints.mMaxSize.height);
    }
    break;

    case WM_SETFOCUS:
      
      
      if (!WinUtils::IsOurProcessWindow(HWND(wParam))) {
        ForgetRedirectedKeyDownMessage();
      }
      if (sJustGotActivate) {
        DispatchFocusToTopLevelWindow(true);
      }
      break;

    case WM_KILLFOCUS:
      if (sJustGotDeactivate) {
        DispatchFocusToTopLevelWindow(false);
      }
      break;

    case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS *wp = (LPWINDOWPOS)lParam;
      OnWindowPosChanged(wp, result);
    }
    break;

    case WM_INPUTLANGCHANGEREQUEST:
      *aRetValue = TRUE;
      result = false;
      break;

    case WM_INPUTLANGCHANGE:
      result = OnInputLangChange((HKL)lParam);
      break;

    case WM_DESTROYCLIPBOARD:
    {
      nsIClipboard* clipboard;
      nsresult rv = CallGetService(kCClipboardCID, &clipboard);
      if(NS_SUCCEEDED(rv)) {
        clipboard->EmptyClipboard(nsIClipboard::kGlobalClipboard);
        NS_RELEASE(clipboard);
      }
    }
    break;

#ifdef ACCESSIBILITY
    case WM_GETOBJECT:
    {
      *aRetValue = 0;
      
      
      DWORD objId = static_cast<DWORD>(lParam);
      if (objId == OBJID_CLIENT) { 
        a11y::Accessible* rootAccessible = GetRootAccessible(); 
        if (rootAccessible) {
          IAccessible *msaaAccessible = NULL;
          rootAccessible->GetNativeInterface((void**)&msaaAccessible); 
          if (msaaAccessible) {
            *aRetValue = LresultFromObject(IID_IAccessible, wParam, msaaAccessible); 
            msaaAccessible->Release(); 
            result = true;  
          }
        }
      }
    }
#endif

    case WM_SYSCOMMAND:
    {
      WPARAM filteredWParam = (wParam &0xFFF0);
      
      if (!sTrimOnMinimize && filteredWParam == SC_MINIMIZE) {
        ::ShowWindow(mWnd, SW_SHOWMINIMIZED);
        result = true;
      }

      
      
      if (filteredWParam == SC_KEYMENU && lParam == VK_SPACE &&
          mSizeMode == nsSizeMode_Fullscreen) {
        DisplaySystemMenu(mWnd, mSizeMode, mIsRTL,
                          MOZ_SYSCONTEXT_X_POS,
                          MOZ_SYSCONTEXT_Y_POS);
        result = true;
      }
    }
    break;

  case WM_DWMCOMPOSITIONCHANGED:
    
    
    nsUXThemeData::CheckForCompositor(true);

    UpdateNonClientMargins();
    RemovePropW(mWnd, kManageWindowInfoProperty);
    BroadcastMsg(mWnd, WM_DWMCOMPOSITIONCHANGED);
    NotifyThemeChanged();
    UpdateGlass();
    Invalidate(true, true, true);
    break;

  case WM_UPDATEUISTATE:
  {
    
    
    
    
    
    int32_t action = LOWORD(wParam);
    if (action == UIS_SET || action == UIS_CLEAR) {
      int32_t flags = HIWORD(wParam);
      UIStateChangeType showAccelerators = UIStateChangeType_NoChange;
      UIStateChangeType showFocusRings = UIStateChangeType_NoChange;
      if (flags & UISF_HIDEACCEL)
        showAccelerators = (action == UIS_SET) ? UIStateChangeType_Clear : UIStateChangeType_Set;
      if (flags & UISF_HIDEFOCUS)
        showFocusRings = (action == UIS_SET) ? UIStateChangeType_Clear : UIStateChangeType_Set;
      NotifyUIStateChanged(showAccelerators, showFocusRings);
    }

    break;
  }

  
  case WM_TABLET_QUERYSYSTEMGESTURESTATUS:
    
    
    result = true;
    *aRetValue = TABLET_ROTATE_GESTURE_ENABLE;
    break;

  case WM_TOUCH:
    result = OnTouch(wParam, lParam);
    if (result) {
      *aRetValue = 0;
    }
    break;

  case WM_GESTURE:
    result = OnGesture(wParam, lParam);
    break;

  case WM_GESTURENOTIFY:
    {
      if (mWindowType != eWindowType_invisible &&
          mWindowType != eWindowType_plugin) {
        
        
        
        GESTURENOTIFYSTRUCT * gestureinfo = (GESTURENOTIFYSTRUCT*)lParam;
        nsPointWin touchPoint;
        touchPoint = gestureinfo->ptsLocation;
        touchPoint.ScreenToClient(mWnd);
        nsGestureNotifyEvent gestureNotifyEvent(true, NS_GESTURENOTIFY_EVENT_START, this);
        gestureNotifyEvent.refPoint = touchPoint;
        nsEventStatus status;
        DispatchEvent(&gestureNotifyEvent, status);
        mDisplayPanFeedback = gestureNotifyEvent.displayPanFeedback;
        if (!mTouchWindow)
          mGesture.SetWinGestureSupport(mWnd, gestureNotifyEvent.panDirection);
      }
      result = false; 
    }
    break;

    case WM_CLEAR:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_DELETE, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case WM_CUT:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_CUT, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case WM_COPY:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_COPY, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case WM_PASTE:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_PASTE, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case EM_UNDO:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_UNDO, this);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case EM_REDO:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_REDO, this);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case EM_CANPASTE:
    {
      
      
      if (wParam == 0 || wParam == CF_TEXT || wParam == CF_UNICODETEXT) {
        nsContentCommandEvent command(true, NS_CONTENT_COMMAND_PASTE,
                                      this, true);
        DispatchWindowEvent(&command);
        *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
        result = true;
      }
    }
    break;

    case EM_CANUNDO:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_UNDO,
                                    this, true);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case EM_CANREDO:
    {
      nsContentCommandEvent command(true, NS_CONTENT_COMMAND_REDO,
                                    this, true);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    default:
    {
      if (msg == nsAppShell::GetTaskbarButtonCreatedMessage())
        SetHasTaskbarIconBeenCreated();
      if (msg == sOOPPPluginFocusEvent) {
        if (wParam == 1) {
          
          
          
          
          ::SendMessage(mWnd, WM_MOUSEACTIVATE, 0, 0); 
        } else {
          
          if (sJustGotDeactivate) {
            DispatchFocusToTopLevelWindow(false);
          }
        }
      }
    }
    break;
  }

  
  if (mWnd) {
    return result;
  }
  else {
    
    
    return true;
  }
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
  
  
  ::EnumChildWindows(aTopWindow, nsWindow::BroadcastMsgToChildren, aMsg);
  return TRUE;
}










int32_t
nsWindow::ClientMarginHitTestPoint(int32_t mx, int32_t my)
{
  if (mSizeMode == nsSizeMode_Minimized ||
      mSizeMode == nsSizeMode_Fullscreen) {
    return HTCLIENT;
  }

  
  RECT winRect;
  GetWindowRect(mWnd, &winRect);

  
  
  
  
  
  
  
  
  
  

  int32_t testResult = HTCLIENT;

  bool isResizable = (mBorderStyle & (eBorderStyle_all |
                                      eBorderStyle_resizeh |
                                      eBorderStyle_default)) > 0 ? true : false;
  if (mSizeMode == nsSizeMode_Maximized)
    isResizable = false;

  
  
  nsIntMargin nonClientSize(std::max(mCaptionHeight - mNonClientOffset.top,
                                     kResizableBorderMinSize),
                            std::max(mHorResizeMargin - mNonClientOffset.right,
                                     kResizableBorderMinSize),
                            std::max(mVertResizeMargin - mNonClientOffset.bottom,
                                     kResizableBorderMinSize),
                            std::max(mHorResizeMargin - mNonClientOffset.left,
                                     kResizableBorderMinSize));

  bool allowContentOverride = mSizeMode == nsSizeMode_Maximized ||
                              (mx >= winRect.left + nonClientSize.left &&
                               mx <= winRect.right - nonClientSize.right &&
                               my >= winRect.top + nonClientSize.top &&
                               my <= winRect.bottom - nonClientSize.bottom);

  
  
  
  
  
  nsIntMargin borderSize(std::max(nonClientSize.top,    mVertResizeMargin),
                         std::max(nonClientSize.right,  mHorResizeMargin),
                         std::max(nonClientSize.bottom, mVertResizeMargin),
                         std::max(nonClientSize.left,   mHorResizeMargin));

  bool top    = false;
  bool bottom = false;
  bool left   = false;
  bool right  = false;

  if (my >= winRect.top && my < winRect.top + borderSize.top) {
    top = true;
  } else if (my <= winRect.bottom && my > winRect.bottom - borderSize.bottom) {
    bottom = true;
  }

  
  int multiplier = (top || bottom) ? 2 : 1;
  if (mx >= winRect.left &&
      mx < winRect.left + (multiplier * borderSize.left)) {
    left = true;
  } else if (mx <= winRect.right &&
             mx > winRect.right - (multiplier * borderSize.right)) {
    right = true;
  }

  if (isResizable) {
    if (top) {
      testResult = HTTOP;
      if (left)
        testResult = HTTOPLEFT;
      else if (right)
        testResult = HTTOPRIGHT;
    } else if (bottom) {
      testResult = HTBOTTOM;
      if (left)
        testResult = HTBOTTOMLEFT;
      else if (right)
        testResult = HTBOTTOMRIGHT;
    } else {
      if (left)
        testResult = HTLEFT;
      if (right)
        testResult = HTRIGHT;
    }
  } else {
    if (top)
      testResult = HTCAPTION;
    else if (bottom || left || right)
      testResult = HTBORDER;
  }

  if (!sIsInMouseCapture && allowContentOverride) {
    LPARAM lParam = MAKELPARAM(mx, my);
    LPARAM lParamClient = lParamToClient(lParam);
    bool result = DispatchMouseEvent(NS_MOUSE_MOZHITTEST, 0, lParamClient,
                                     false, nsMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
    if (result) {
      
      testResult = testResult == HTCLIENT ? HTCAPTION : testResult;

    } else {
      
      
      testResult = HTCLIENT;
    }
  }

  return testResult;
}

void nsWindow::PostSleepWakeNotification(const bool aIsSleepMode)
{
  if (aIsSleepMode == gIsSleepMode)
    return;

  gIsSleepMode = aIsSleepMode;

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService)
    observerService->NotifyObservers(nullptr,
      aIsSleepMode ? NS_WIDGET_SLEEP_OBSERVER_TOPIC :
                     NS_WIDGET_WAKE_OBSERVER_TOPIC, nullptr);
}










void nsWindow::RemoveNextCharMessage(HWND aWnd)
{
  MSG msg;
  if (WinUtils::PeekMessage(&msg, aWnd, WM_KEYFIRST, WM_KEYLAST,
                            PM_NOREMOVE | PM_NOYIELD) &&
      (msg.message == WM_CHAR || msg.message == WM_SYSCHAR)) {
    WinUtils::GetMessage(&msg, aWnd, msg.message, msg.message);
  }
}

LRESULT nsWindow::ProcessCharMessage(const MSG &aMsg, bool *aEventDispatched)
{
  NS_PRECONDITION(aMsg.message == WM_CHAR || aMsg.message == WM_SYSCHAR,
                  "message is not keydown event");
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("%s charCode=%d scanCode=%d\n",
          aMsg.message == WM_SYSCHAR ? "WM_SYSCHAR" : "WM_CHAR",
          aMsg.wParam, HIWORD(aMsg.lParam) & 0xFF));

  
  
  ModifierKeyState modKeyState;
  NativeKey nativeKey(gKbdLayout, this, aMsg);
  return OnChar(aMsg, nativeKey, modKeyState, aEventDispatched);
}

LRESULT nsWindow::ProcessKeyUpMessage(const MSG &aMsg, bool *aEventDispatched)
{
  NS_PRECONDITION(aMsg.message == WM_KEYUP || aMsg.message == WM_SYSKEYUP,
                  "message is not keydown event");
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("%s VK=%d\n", aMsg.message == WM_SYSKEYDOWN ?
                        "WM_SYSKEYUP" : "WM_KEYUP", aMsg.wParam));

  ModifierKeyState modKeyState;

  
  
  
  
  
  
  
  

  
  if (modKeyState.IsAlt() && !modKeyState.IsControl() &&
      IS_VK_DOWN(NS_VK_SPACE)) {
    return FALSE;
  }

  if (!IMEHandler::IsComposingOn(this)) {
    return OnKeyUp(aMsg, modKeyState, aEventDispatched);
  }

  return 0;
}

LRESULT nsWindow::ProcessKeyDownMessage(const MSG &aMsg,
                                        bool *aEventDispatched)
{
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("%s VK=%d\n", aMsg.message == WM_SYSKEYDOWN ?
                        "WM_SYSKEYDOWN" : "WM_KEYDOWN", aMsg.wParam));
  NS_PRECONDITION(aMsg.message == WM_KEYDOWN || aMsg.message == WM_SYSKEYDOWN,
                  "message is not keydown event");

  
  
  
  
  AutoForgetRedirectedKeyDownMessage forgetRedirectedMessage(this, aMsg);

  ModifierKeyState modKeyState;

  
  
  
  
  
  
  
  

  
  if (modKeyState.IsAlt() && !modKeyState.IsControl() &&
      IS_VK_DOWN(NS_VK_SPACE))
    return FALSE;

  LRESULT result = 0;
  if (!IMEHandler::IsComposingOn(this)) {
    result = OnKeyDown(aMsg, modKeyState, aEventDispatched, nullptr);
    
    
    forgetRedirectedMessage.mCancel = true;
  }

  if (aMsg.wParam == VK_MENU ||
      (aMsg.wParam == VK_F10 && !modKeyState.IsShift())) {
    
    
    
    
    
    bool hasNativeMenu = false;
    HWND hWnd = mWnd;
    while (hWnd) {
      if (::GetMenu(hWnd)) {
        hasNativeMenu = true;
        break;
      }
      hWnd = ::GetParent(hWnd);
    }
    result = !hasNativeMenu;
  }

  return result;
}

nsresult
nsWindow::SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                   int32_t aNativeKeyCode,
                                   uint32_t aModifierFlags,
                                   const nsAString& aCharacters,
                                   const nsAString& aUnmodifiedCharacters)
{
  UINT keyboardLayoutListCount = ::GetKeyboardLayoutList(0, NULL);
  NS_ASSERTION(keyboardLayoutListCount > 0,
               "One keyboard layout must be installed at least");
  HKL keyboardLayoutListBuff[50];
  HKL* keyboardLayoutList =
    keyboardLayoutListCount < 50 ? keyboardLayoutListBuff :
                                   new HKL[keyboardLayoutListCount];
  keyboardLayoutListCount =
    ::GetKeyboardLayoutList(keyboardLayoutListCount, keyboardLayoutList);
  NS_ASSERTION(keyboardLayoutListCount > 0,
               "Failed to get all keyboard layouts installed on the system");

  nsPrintfCString layoutName("%08x", aNativeKeyboardLayout);
  HKL loadedLayout = LoadKeyboardLayoutA(layoutName.get(), KLF_NOTELLSHELL);
  if (loadedLayout == NULL) {
    if (keyboardLayoutListBuff != keyboardLayoutList) {
      delete [] keyboardLayoutList;
    }
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  BYTE originalKbdState[256];
  ::GetKeyboardState(originalKbdState);
  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));
  
  
  ::SetKeyboardState(kbdState);
  HKL oldLayout = gKbdLayout.GetLayout();
  gKbdLayout.LoadLayout(loadedLayout);

  uint8_t argumentKeySpecific = 0;
  switch (aNativeKeyCode) {
    case VK_SHIFT:
      aModifierFlags &= ~(nsIWidget::SHIFT_L | nsIWidget::SHIFT_R);
      argumentKeySpecific = VK_LSHIFT;
      break;
    case VK_LSHIFT:
      aModifierFlags &= ~nsIWidget::SHIFT_L;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_SHIFT;
      break;
    case VK_RSHIFT:
      aModifierFlags &= ~nsIWidget::SHIFT_R;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_SHIFT;
      break;
    case VK_CONTROL:
      aModifierFlags &= ~(nsIWidget::CTRL_L | nsIWidget::CTRL_R);
      argumentKeySpecific = VK_LCONTROL;
      break;
    case VK_LCONTROL:
      aModifierFlags &= ~nsIWidget::CTRL_L;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_CONTROL;
      break;
    case VK_RCONTROL:
      aModifierFlags &= ~nsIWidget::CTRL_R;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_CONTROL;
      break;
    case VK_MENU:
      aModifierFlags &= ~(nsIWidget::ALT_L | nsIWidget::ALT_R);
      argumentKeySpecific = VK_LMENU;
      break;
    case VK_LMENU:
      aModifierFlags &= ~nsIWidget::ALT_L;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_MENU;
      break;
    case VK_RMENU:
      aModifierFlags &= ~nsIWidget::ALT_R;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_MENU;
      break;
    case VK_CAPITAL:
      aModifierFlags &= ~nsIWidget::CAPS_LOCK;
      argumentKeySpecific = VK_CAPITAL;
      break;
    case VK_NUMLOCK:
      aModifierFlags &= ~nsIWidget::NUM_LOCK;
      argumentKeySpecific = VK_NUMLOCK;
      break;
  }

  nsAutoTArray<KeyPair,10> keySequence;
  SetupKeyModifiersSequence(&keySequence, aModifierFlags);
  NS_ASSERTION(aNativeKeyCode >= 0 && aNativeKeyCode < 256,
               "Native VK key code out of range");
  keySequence.AppendElement(KeyPair(aNativeKeyCode, argumentKeySpecific));

  
  for (uint32_t i = 0; i < keySequence.Length(); ++i) {
    uint8_t key = keySequence[i].mGeneral;
    uint8_t keySpecific = keySequence[i].mSpecific;
    kbdState[key] = 0x81; 
    if (keySpecific) {
      kbdState[keySpecific] = 0x81;
    }
    ::SetKeyboardState(kbdState);
    ModifierKeyState modKeyState;
    UINT scanCode = ::MapVirtualKeyEx(argumentKeySpecific ?
                                        argumentKeySpecific : aNativeKeyCode,
                                      MAPVK_VK_TO_VSC, gKbdLayout.GetLayout());
    LPARAM lParam = static_cast<LPARAM>(scanCode << 16);
    
    
    if (keySpecific == VK_RCONTROL || keySpecific == VK_RMENU) {
      lParam |= 0x1000000;
    }
    MSG msg = WinUtils::InitMSG(WM_KEYDOWN, key, lParam);
    if (i == keySequence.Length() - 1) {
      bool makeDeadCharMessage =
        gKbdLayout.IsDeadKey(key, modKeyState) && aCharacters.IsEmpty();
      nsAutoString chars(aCharacters);
      if (makeDeadCharMessage) {
        UniCharsAndModifiers deadChars =
          gKbdLayout.GetUniCharsAndModifiers(key, modKeyState);
        chars = deadChars.ToString();
        NS_ASSERTION(chars.Length() == 1,
                     "Dead char must be only one character");
      }
      if (chars.IsEmpty()) {
        OnKeyDown(msg, modKeyState, nullptr, nullptr);
      } else {
        nsFakeCharMessage fakeMsg = { chars.CharAt(0), scanCode,
                                      makeDeadCharMessage };
        OnKeyDown(msg, modKeyState, nullptr, &fakeMsg);
        for (uint32_t j = 1; j < chars.Length(); j++) {
          nsFakeCharMessage fakeMsg = { chars.CharAt(j), scanCode, false };
          MSG msg = fakeMsg.GetCharMessage(mWnd);
          NativeKey nativeKey(gKbdLayout, this, msg);
          OnChar(msg, nativeKey, modKeyState, nullptr);
        }
      }
    } else {
      OnKeyDown(msg, modKeyState, nullptr, nullptr);
    }
  }
  for (uint32_t i = keySequence.Length(); i > 0; --i) {
    uint8_t key = keySequence[i - 1].mGeneral;
    uint8_t keySpecific = keySequence[i - 1].mSpecific;
    kbdState[key] = 0; 
    if (keySpecific) {
      kbdState[keySpecific] = 0;
    }
    ::SetKeyboardState(kbdState);
    ModifierKeyState modKeyState;
    UINT scanCode = ::MapVirtualKeyEx(argumentKeySpecific ?
                                        argumentKeySpecific : aNativeKeyCode,
                                      MAPVK_VK_TO_VSC, gKbdLayout.GetLayout());
    LPARAM lParam = static_cast<LPARAM>(scanCode << 16);
    
    
    if (keySpecific == VK_RCONTROL || keySpecific == VK_RMENU) {
      lParam |= 0x1000000;
    }
    MSG msg = WinUtils::InitMSG(WM_KEYUP, key, lParam);
    OnKeyUp(msg, modKeyState, nullptr);
  }

  
  ::SetKeyboardState(originalKbdState);
  gKbdLayout.LoadLayout(oldLayout, true);

  
  for (uint32_t i = 0; i < keyboardLayoutListCount; i++) {
    if (keyboardLayoutList[i] == loadedLayout) {
      loadedLayout = 0;
      break;
    }
  }
  if (keyboardLayoutListBuff != keyboardLayoutList) {
    delete [] keyboardLayoutList;
  }
  if (loadedLayout) {
    ::UnloadKeyboardLayout(loadedLayout);
  }
  return NS_OK;
}

nsresult
nsWindow::SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                     uint32_t aNativeMessage,
                                     uint32_t aModifierFlags)
{
  ::SetCursorPos(aPoint.x, aPoint.y);

  INPUT input;
  memset(&input, 0, sizeof(input));

  input.type = INPUT_MOUSE;
  input.mi.dwFlags = aNativeMessage;
  ::SendInput(1, &input, sizeof(INPUT));

  return NS_OK;
}

nsresult
nsWindow::SynthesizeNativeMouseScrollEvent(nsIntPoint aPoint,
                                           uint32_t aNativeMessage,
                                           double aDeltaX,
                                           double aDeltaY,
                                           double aDeltaZ,
                                           uint32_t aModifierFlags,
                                           uint32_t aAdditionalFlags)
{
  return MouseScrollHandler::SynthesizeNativeMouseScrollEvent(
           this, aPoint, aNativeMessage,
           (aNativeMessage == WM_MOUSEWHEEL || aNativeMessage == WM_VSCROLL) ?
             static_cast<int32_t>(aDeltaY) : static_cast<int32_t>(aDeltaX),
           aModifierFlags, aAdditionalFlags);
}










BOOL nsWindow::OnInputLangChange(HKL aHKL)
{
#ifdef KE_DEBUG
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("OnInputLanguageChange\n"));
#endif
  gKbdLayout.LoadLayout(aHKL);
  return false;   
}

void nsWindow::OnWindowPosChanged(WINDOWPOS *wp, bool& result)
{
  if (wp == nullptr)
    return;

#ifdef WINSTATE_DEBUG_OUTPUT
  if (mWnd == WinUtils::GetTopLevelHWND(mWnd)) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("*** OnWindowPosChanged: [  top] "));
  } else {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("*** OnWindowPosChanged: [child] "));
  }
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("WINDOWPOS flags:"));
  if (wp->flags & SWP_FRAMECHANGED) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("SWP_FRAMECHANGED "));
  }
  if (wp->flags & SWP_SHOWWINDOW) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("SWP_SHOWWINDOW "));
  }
  if (wp->flags & SWP_NOSIZE) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("SWP_NOSIZE "));
  }
  if (wp->flags & SWP_HIDEWINDOW) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("SWP_HIDEWINDOW "));
  }
  if (wp->flags & SWP_NOZORDER) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("SWP_NOZORDER "));
  }
  if (wp->flags & SWP_NOACTIVATE) {
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("SWP_NOACTIVATE "));
  }
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("\n"));
#endif

  
  if (wp->flags & SWP_FRAMECHANGED && mSizeMode != nsSizeMode_Fullscreen) {

    
    
    
    
    if (mSizeMode == nsSizeMode_Minimized && (wp->flags & SWP_NOACTIVATE))
      return;

    WINDOWPLACEMENT pl;
    pl.length = sizeof(pl);
    ::GetWindowPlacement(mWnd, &pl);

    
    
    
    
    
    
    
    if (pl.showCmd == SW_SHOWMAXIMIZED)
      mSizeMode = (mFullscreenMode ? nsSizeMode_Fullscreen : nsSizeMode_Maximized);
    else if (pl.showCmd == SW_SHOWMINIMIZED)
      mSizeMode = nsSizeMode_Minimized;
    else if (mFullscreenMode)
      mSizeMode = nsSizeMode_Fullscreen;
    else
      mSizeMode = nsSizeMode_Normal;

    
    
    
    
    
    if (!sTrimOnMinimize && nsSizeMode_Minimized == mSizeMode)
      ActivateOtherWindowHelper(mWnd);

#ifdef WINSTATE_DEBUG_OUTPUT
    switch (mSizeMode) {
      case nsSizeMode_Normal:
          PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
                 ("*** mSizeMode: nsSizeMode_Normal\n"));
        break;
      case nsSizeMode_Minimized:
        PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
               ("*** mSizeMode: nsSizeMode_Minimized\n"));
        break;
      case nsSizeMode_Maximized:
          PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
                 ("*** mSizeMode: nsSizeMode_Maximized\n");
        break;
      default:
          PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("*** mSizeMode: ??????\n");
        break;
    };
#endif

    if (mWidgetListener)
      mWidgetListener->SizeModeChanged(mSizeMode);

    
    
    
    if (mLastSizeMode != nsSizeMode_Normal && mSizeMode == nsSizeMode_Normal)
      DispatchFocusToTopLevelWindow(true);

    
    if (mSizeMode == nsSizeMode_Minimized)
      return;
  }

  
  if (!(wp->flags & SWP_NOSIZE)) {
    RECT r;
    int32_t newWidth, newHeight;

    ::GetWindowRect(mWnd, &r);

    newWidth  = r.right - r.left;
    newHeight = r.bottom - r.top;
    nsIntRect rect(wp->x, wp->y, newWidth, newHeight);

#ifdef MOZ_XUL
    if (eTransparencyTransparent == mTransparencyMode)
      ResizeTranslucentWindow(newWidth, newHeight);
#endif

    if (newWidth > mLastSize.width)
    {
      RECT drect;

      
      drect.left   = wp->x + mLastSize.width;
      drect.top    = wp->y;
      drect.right  = drect.left + (newWidth - mLastSize.width);
      drect.bottom = drect.top + newHeight;

      ::RedrawWindow(mWnd, &drect, NULL,
                     RDW_INVALIDATE |
                     RDW_NOERASE |
                     RDW_NOINTERNALPAINT |
                     RDW_ERASENOW |
                     RDW_ALLCHILDREN);
    }
    if (newHeight > mLastSize.height)
    {
      RECT drect;

      
      drect.left   = wp->x;
      drect.top    = wp->y + mLastSize.height;
      drect.right  = drect.left + newWidth;
      drect.bottom = drect.top + (newHeight - mLastSize.height);

      ::RedrawWindow(mWnd, &drect, NULL,
                     RDW_INVALIDATE |
                     RDW_NOERASE |
                     RDW_NOINTERNALPAINT |
                     RDW_ERASENOW |
                     RDW_ALLCHILDREN);
    }

    mBounds.width    = newWidth;
    mBounds.height   = newHeight;
    mLastSize.width  = newWidth;
    mLastSize.height = newHeight;

#ifdef WINSTATE_DEBUG_OUTPUT
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
           ("*** Resize window: %d x %d x %d x %d\n", wp->x, wp->y, 
            newWidth, newHeight));
#endif
    
    
    if (mSizeMode == nsSizeMode_Maximized) {
      if (UpdateNonClientMargins(nsSizeMode_Maximized, true)) {
        
        result = true;
        return;
      }
    }

    
    if (::GetClientRect(mWnd, &r)) {
      rect.width  = r.right - r.left;
      rect.height = r.bottom - r.top;
    }
    
    
    result = OnResize(rect);
  }
}


void nsWindow::ActivateOtherWindowHelper(HWND aWnd)
{
  
  HWND hwndBelow = ::GetNextWindow(aWnd, GW_HWNDNEXT);
  while (hwndBelow && (!::IsWindowEnabled(hwndBelow) || !::IsWindowVisible(hwndBelow) ||
                       ::IsIconic(hwndBelow))) {
    hwndBelow = ::GetNextWindow(hwndBelow, GW_HWNDNEXT);
  }

  
  
  ::SetWindowPos(aWnd, HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
  if (hwndBelow)
    ::SetForegroundWindow(hwndBelow);

  
  
  nsCOMPtr<nsISound> sound(do_CreateInstance("@mozilla.org/sound;1"));
  if (sound) {
    sound->PlaySystemSound(NS_LITERAL_STRING("Minimize"));
  }
}

void nsWindow::OnWindowPosChanging(LPWINDOWPOS& info)
{
  
  
  
  
  if ((info->flags & SWP_FRAMECHANGED && !(info->flags & SWP_NOSIZE)) &&
      mSizeMode != nsSizeMode_Fullscreen) {
    WINDOWPLACEMENT pl;
    pl.length = sizeof(pl);
    ::GetWindowPlacement(mWnd, &pl);
    nsSizeMode sizeMode;
    if (pl.showCmd == SW_SHOWMAXIMIZED)
      sizeMode = (mFullscreenMode ? nsSizeMode_Fullscreen : nsSizeMode_Maximized);
    else if (pl.showCmd == SW_SHOWMINIMIZED)
      sizeMode = nsSizeMode_Minimized;
    else if (mFullscreenMode)
      sizeMode = nsSizeMode_Fullscreen;
    else
      sizeMode = nsSizeMode_Normal;

    if (mWidgetListener)
      mWidgetListener->SizeModeChanged(sizeMode);

    UpdateNonClientMargins(sizeMode, false);
  }

  
  if (!(info->flags & SWP_NOZORDER)) {
    HWND hwndAfter = info->hwndInsertAfter;

    nsWindow *aboveWindow = 0;
    nsWindowZ placement;

    if (hwndAfter == HWND_BOTTOM)
      placement = nsWindowZBottom;
    else if (hwndAfter == HWND_TOP || hwndAfter == HWND_TOPMOST || hwndAfter == HWND_NOTOPMOST)
      placement = nsWindowZTop;
    else {
      placement = nsWindowZRelative;
      aboveWindow = WinUtils::GetNSWindowPtr(hwndAfter);
    }

    if (mWidgetListener) {
      nsCOMPtr<nsIWidget> actualBelow = nullptr;
      if (mWidgetListener->ZLevelChanged(false, &placement,
                                         aboveWindow, getter_AddRefs(actualBelow))) {
        if (placement == nsWindowZBottom)
          info->hwndInsertAfter = HWND_BOTTOM;
        else if (placement == nsWindowZTop)
          info->hwndInsertAfter = HWND_TOP;
        else {
          info->hwndInsertAfter = (HWND)actualBelow->GetNativeData(NS_NATIVE_WINDOW);
        }
      }
    }
  }
  
  if (mWindowType == eWindowType_invisible)
    info->flags &= ~SWP_SHOWWINDOW;
}

void nsWindow::UserActivity()
{
  
  if (!mIdleService) {
    mIdleService = do_GetService("@mozilla.org/widget/idleservice;1");
  }

  
  if (mIdleService) {
    mIdleService->ResetIdleTimeOut(0);
  }
}

bool nsWindow::OnTouch(WPARAM wParam, LPARAM lParam)
{
  uint32_t cInputs = LOWORD(wParam);
  PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];

  if (mGesture.GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs)) {
    nsTouchEvent* touchEventToSend = nullptr;
    nsTouchEvent* touchEndEventToSend = nullptr;
    nsEventStatus status;

    
    for (uint32_t i = 0; i < cInputs; i++) {
      uint32_t msg;

      if (pInputs[i].dwFlags & (TOUCHEVENTF_DOWN | TOUCHEVENTF_MOVE)) {
        
        if (!touchEventToSend) {
          touchEventToSend = new nsTouchEvent(true, NS_TOUCH_MOVE, this);
          touchEventToSend->time = ::GetMessageTime();
          ModifierKeyState modifierKeyState;
          modifierKeyState.InitInputEvent(*touchEventToSend);
        }

        
        
        if (pInputs[i].dwFlags & TOUCHEVENTF_DOWN) {
          touchEventToSend->message = msg = NS_TOUCH_START;
        } else {
          msg = NS_TOUCH_MOVE;
        }
      } else if (pInputs[i].dwFlags & TOUCHEVENTF_UP) {
        
        
        
        if (!touchEndEventToSend) {
          touchEndEventToSend = new nsTouchEvent(true, NS_TOUCH_END, this);
          touchEndEventToSend->time = ::GetMessageTime();
          ModifierKeyState modifierKeyState;
          modifierKeyState.InitInputEvent(*touchEndEventToSend);
        }
        msg = NS_TOUCH_END;
      } else {
        
        
        continue;
      }

      
      nsPointWin touchPoint;
      touchPoint.x = TOUCH_COORD_TO_PIXEL(pInputs[i].x);
      touchPoint.y = TOUCH_COORD_TO_PIXEL(pInputs[i].y);
      touchPoint.ScreenToClient(mWnd);
      nsCOMPtr<nsIDOMTouch> touch =
        new nsDOMTouch(pInputs[i].dwID,
                       touchPoint,
                       
                       pInputs[i].dwFlags & TOUCHINPUTMASKF_CONTACTAREA ?
                         nsIntPoint(
                           TOUCH_COORD_TO_PIXEL(pInputs[i].cxContact) / 2,
                           TOUCH_COORD_TO_PIXEL(pInputs[i].cyContact) / 2) :
                         nsIntPoint(1,1),
                       
                       0.0f, 0.0f);

      
      if (msg == NS_TOUCH_START || msg == NS_TOUCH_MOVE) {
        touchEventToSend->touches.AppendElement(touch);
      } else {
        touchEndEventToSend->touches.AppendElement(touch);
      }
    }

    
    if (touchEventToSend) {
      DispatchEvent(touchEventToSend, status);
      delete touchEventToSend;
    }

    
    if (touchEndEventToSend) {
      DispatchEvent(touchEndEventToSend, status);
      delete touchEndEventToSend;
    }
  }

  delete [] pInputs;
  mGesture.CloseTouchInputHandle((HTOUCHINPUT)lParam);
  return true;
}

static int32_t RoundDown(double aDouble)
{
  return aDouble > 0 ? static_cast<int32_t>(floor(aDouble)) :
                       static_cast<int32_t>(ceil(aDouble));
}


bool nsWindow::OnGesture(WPARAM wParam, LPARAM lParam)
{
  
  if (mGesture.IsPanEvent(lParam)) {
    if ( !mGesture.ProcessPanMessage(mWnd, wParam, lParam) )
      return false; 

    nsEventStatus status;

    WheelEvent wheelEvent(true, NS_WHEEL_WHEEL, this);

    ModifierKeyState modifierKeyState;
    modifierKeyState.InitInputEvent(wheelEvent);

    wheelEvent.button      = 0;
    wheelEvent.time        = ::GetMessageTime();
    wheelEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

    bool endFeedback = true;

    if (mGesture.PanDeltaToPixelScroll(wheelEvent)) {
      DispatchEvent(&wheelEvent, status);
    }

    if (mDisplayPanFeedback) {
      mGesture.UpdatePanFeedbackX(mWnd,
                                  DeprecatedAbs(RoundDown(wheelEvent.overflowDeltaX)),
                                  endFeedback);
      mGesture.UpdatePanFeedbackY(mWnd,
                                  DeprecatedAbs(RoundDown(wheelEvent.overflowDeltaY)),
                                  endFeedback);
      mGesture.PanFeedbackFinalize(mWnd, endFeedback);
    }

    mGesture.CloseGestureInfoHandle((HGESTUREINFO)lParam);

    return true;
  }

  
  nsSimpleGestureEvent event(true, 0, this, 0, 0.0);
  if ( !mGesture.ProcessGestureMessage(mWnd, wParam, lParam, event) ) {
    return false; 
  }
  
  
  ModifierKeyState modifierKeyState;
  modifierKeyState.InitInputEvent(event);
  event.button    = 0;
  event.time      = ::GetMessageTime();
  event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

  nsEventStatus status;
  DispatchEvent(&event, status);
  if (status == nsEventStatus_eIgnore) {
    return false; 
  }

  
  mGesture.CloseGestureInfoHandle((HGESTUREINFO)lParam);

  return true; 
}


bool nsWindow::IsRedirectedKeyDownMessage(const MSG &aMsg)
{
  return (aMsg.message == WM_KEYDOWN || aMsg.message == WM_SYSKEYDOWN) &&
         (sRedirectedKeyDown.message == aMsg.message &&
          WinUtils::GetScanCode(sRedirectedKeyDown.lParam) ==
            WinUtils::GetScanCode(aMsg.lParam));
}









LRESULT nsWindow::OnKeyDown(const MSG &aMsg,
                            const ModifierKeyState &aModKeyState,
                            bool *aEventDispatched,
                            nsFakeCharMessage* aFakeCharMessage)
{
  NativeKey nativeKey(gKbdLayout, this, aMsg);
  UINT virtualKeyCode = nativeKey.GetOriginalVirtualKeyCode();
  UniCharsAndModifiers inputtingChars =
    gKbdLayout.OnKeyDown(virtualKeyCode, aModKeyState);

  
  
  uint32_t DOMKeyCode = nativeKey.GetDOMKeyCode();

#ifdef DEBUG
  
#endif

  static bool sRedirectedKeyDownEventPreventedDefault = false;
  bool noDefault;
  if (aFakeCharMessage || !IsRedirectedKeyDownMessage(aMsg)) {
    bool isIMEEnabled = IMEHandler::IsIMEEnabled(mInputContext);
    nsKeyEvent keydownEvent(true, NS_KEY_DOWN, this);
    keydownEvent.keyCode = DOMKeyCode;
    InitKeyEvent(keydownEvent, nativeKey, aModKeyState);
    noDefault = DispatchKeyEvent(keydownEvent, &aMsg);
    if (aEventDispatched) {
      *aEventDispatched = true;
    }

    
    
    
    
    
    
    
    
    HWND focusedWnd = ::GetFocus();
    if (!noDefault && !aFakeCharMessage && focusedWnd && !PluginHasFocus() &&
        !isIMEEnabled && IMEHandler::IsIMEEnabled(mInputContext)) {
      RemoveNextCharMessage(focusedWnd);

      INPUT keyinput;
      keyinput.type = INPUT_KEYBOARD;
      keyinput.ki.wVk = aMsg.wParam;
      keyinput.ki.wScan = WinUtils::GetScanCode(aMsg.lParam);
      keyinput.ki.dwFlags = KEYEVENTF_SCANCODE;
      if (WinUtils::IsExtendedScanCode(aMsg.lParam)) {
        keyinput.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
      }
      keyinput.ki.time = 0;
      keyinput.ki.dwExtraInfo = 0;

      sRedirectedKeyDownEventPreventedDefault = noDefault;
      sRedirectedKeyDown = aMsg;

      ::SendInput(1, &keyinput, sizeof(keyinput));

      
      
      
      return true;
    }

    if (mOnDestroyCalled) {
      
      
      return true;
    }
  } else {
    noDefault = sRedirectedKeyDownEventPreventedDefault;
    
    
    if (aEventDispatched) {
      *aEventDispatched = true;
    }
  }

  ForgetRedirectedKeyDownMessage();

  
  if (aMsg.wParam == VK_PROCESSKEY) {
    return noDefault;
  }

  
  
  switch (DOMKeyCode) {
    case NS_VK_SHIFT:
    case NS_VK_CONTROL:
    case NS_VK_ALT:
    case NS_VK_CAPS_LOCK:
    case NS_VK_NUM_LOCK:
    case NS_VK_SCROLL_LOCK:
    case NS_VK_WIN:
      return noDefault;
  }

  bool isDeadKey = gKbdLayout.IsDeadKey(virtualKeyCode, aModKeyState);
  EventFlags extraFlags;
  extraFlags.mDefaultPrevented = noDefault;
  MSG msg;
  BOOL gotMsg = aFakeCharMessage ||
    WinUtils::PeekMessage(&msg, mWnd, WM_KEYFIRST, WM_KEYLAST,
                          PM_NOREMOVE | PM_NOYIELD);
  
  
  if (DOMKeyCode == NS_VK_RETURN || DOMKeyCode == NS_VK_BACK ||
      ((aModKeyState.IsControl() || aModKeyState.IsAlt() || aModKeyState.IsWin())
       && !isDeadKey && KeyboardLayout::IsPrintableCharKey(virtualKeyCode)))
  {
    
    
    
    
    bool anyCharMessagesRemoved = false;

    if (aFakeCharMessage) {
      RemoveMessageAndDispatchPluginEvent(WM_KEYFIRST, WM_KEYLAST,
                                          aFakeCharMessage);
      anyCharMessagesRemoved = true;
    } else {
      while (gotMsg && (msg.message == WM_CHAR || msg.message == WM_SYSCHAR))
      {
        PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
               ("%s charCode=%d scanCode=%d\n", msg.message == WM_SYSCHAR ? 
                                                "WM_SYSCHAR" : "WM_CHAR",
                msg.wParam, HIWORD(msg.lParam) & 0xFF));
        RemoveMessageAndDispatchPluginEvent(WM_KEYFIRST, WM_KEYLAST);
        anyCharMessagesRemoved = true;

        gotMsg = WinUtils::PeekMessage(&msg, mWnd, WM_KEYFIRST, WM_KEYLAST,
                                       PM_NOREMOVE | PM_NOYIELD);
      }
    }

    if (!anyCharMessagesRemoved && DOMKeyCode == NS_VK_BACK &&
        IMEHandler::IsDoingKakuteiUndo(mWnd)) {
      NS_ASSERTION(!aFakeCharMessage,
                   "We shouldn't be touching the real msg queue");
      RemoveMessageAndDispatchPluginEvent(WM_CHAR, WM_CHAR);
    }
  }
  else if (gotMsg &&
           (aFakeCharMessage ||
            msg.message == WM_CHAR || msg.message == WM_SYSCHAR || msg.message == WM_DEADCHAR)) {
    if (aFakeCharMessage) {
      MSG msg = aFakeCharMessage->GetCharMessage(mWnd);
      if (msg.message == WM_DEADCHAR) {
        return false;
      }
#ifdef DEBUG
      if (KeyboardLayout::IsPrintableCharKey(virtualKeyCode)) {
        nsPrintfCString log(
          "virtualKeyCode=0x%02X, inputtingChar={ mChars=[ 0x%04X, 0x%04X, "
          "0x%04X, 0x%04X, 0x%04X ], mLength=%d }, wParam=0x%04X",
          virtualKeyCode, inputtingChars.mChars[0], inputtingChars.mChars[1],
          inputtingChars.mChars[2], inputtingChars.mChars[3],
          inputtingChars.mChars[4], inputtingChars.mLength, msg.wParam);
        if (!inputtingChars.mLength) {
          log.Insert("length is zero: ", 0);
          NS_ERROR(log.get());
          NS_ABORT();
        } else if (inputtingChars.mChars[0] != msg.wParam) {
          log.Insert("character mismatch: ", 0);
          NS_ERROR(log.get());
          NS_ABORT();
        }
      }
#endif 
      return OnChar(msg, nativeKey, aModKeyState, nullptr, &extraFlags);
    }

    
    WinUtils::GetMessage(&msg, mWnd, msg.message, msg.message);

    if (msg.message == WM_DEADCHAR) {
      if (!PluginHasFocus())
        return false;

      
      DispatchPluginEvent(msg);
      return noDefault;
    }

    PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
           ("%s charCode=%d scanCode=%d\n",
            msg.message == WM_SYSCHAR ? "WM_SYSCHAR" : "WM_CHAR",
            msg.wParam, HIWORD(msg.lParam) & 0xFF));

    BOOL result = OnChar(msg, nativeKey, aModKeyState, nullptr, &extraFlags);
    
    
    if (!result && msg.message == WM_SYSCHAR)
      ::DefWindowProcW(mWnd, msg.message, msg.wParam, msg.lParam);
    return result;
  }
  else if (!aModKeyState.IsControl() && !aModKeyState.IsAlt() &&
            !aModKeyState.IsWin() &&
            KeyboardLayout::IsPrintableCharKey(virtualKeyCode)) {
    
    
    
    return PluginHasFocus() && noDefault;
  }

  if (isDeadKey) {
    return PluginHasFocus() && noDefault;
  }

  UniCharsAndModifiers shiftedChars;
  UniCharsAndModifiers unshiftedChars;
  uint32_t shiftedLatinChar = 0;
  uint32_t unshiftedLatinChar = 0;

  if (!KeyboardLayout::IsPrintableCharKey(virtualKeyCode)) {
    inputtingChars.Clear();
  }

  if (aModKeyState.IsControl() ^ aModKeyState.IsAlt()) {
    widget::ModifierKeyState capsLockState(
      aModKeyState.GetModifiers() & MODIFIER_CAPSLOCK);
    unshiftedChars =
      gKbdLayout.GetUniCharsAndModifiers(virtualKeyCode, capsLockState);
    capsLockState.Set(MODIFIER_SHIFT);
    shiftedChars =
      gKbdLayout.GetUniCharsAndModifiers(virtualKeyCode, capsLockState);

    
    
    
    capsLockState.Unset(MODIFIER_SHIFT);
    WidgetUtils::GetLatinCharCodeForKeyCode(DOMKeyCode,
                                            capsLockState.GetModifiers(),
                                            &unshiftedLatinChar,
                                            &shiftedLatinChar);

    
    if (shiftedLatinChar) {
      
      
      
      if (unshiftedLatinChar == unshiftedChars.mChars[0] &&
          shiftedLatinChar == shiftedChars.mChars[0]) {
        shiftedLatinChar = unshiftedLatinChar = 0;
      }
    } else if (unshiftedLatinChar) {
      
      
      
      
      
      
      
      if (unshiftedLatinChar == unshiftedChars.mChars[0] ||
          unshiftedLatinChar == shiftedChars.mChars[0]) {
        unshiftedLatinChar = 0;
      }
    }

    
    
    
    
    
    if (aModKeyState.IsControl()) {
      uint32_t ch =
        aModKeyState.IsShift() ? shiftedLatinChar : unshiftedLatinChar;
      if (ch &&
          (!inputtingChars.mLength ||
           inputtingChars.UniCharsCaseInsensitiveEqual(
             aModKeyState.IsShift() ? shiftedChars : unshiftedChars))) {
        inputtingChars.Clear();
        inputtingChars.Append(ch, aModKeyState.GetModifiers());
      }
    }
  }

  if (inputtingChars.mLength ||
      shiftedChars.mLength || unshiftedChars.mLength) {
    uint32_t num = std::max(inputtingChars.mLength,
                          std::max(shiftedChars.mLength, unshiftedChars.mLength));
    uint32_t skipUniChars = num - inputtingChars.mLength;
    uint32_t skipShiftedChars = num - shiftedChars.mLength;
    uint32_t skipUnshiftedChars = num - unshiftedChars.mLength;
    UINT keyCode = !inputtingChars.mLength ? DOMKeyCode : 0;
    for (uint32_t cnt = 0; cnt < num; cnt++) {
      uint16_t uniChar, shiftedChar, unshiftedChar;
      uniChar = shiftedChar = unshiftedChar = 0;
      ModifierKeyState modKeyState(aModKeyState);
      if (skipUniChars <= cnt) {
        if (cnt - skipUniChars  < inputtingChars.mLength) {
          
          
          
          
          
          
          modKeyState.Unset(MODIFIER_SHIFT | MODIFIER_CONTROL | MODIFIER_ALT |
                            MODIFIER_ALTGRAPH | MODIFIER_CAPSLOCK);
          modKeyState.Set(inputtingChars.mModifiers[cnt - skipUniChars]);
        }
        uniChar = inputtingChars.mChars[cnt - skipUniChars];
      }
      if (skipShiftedChars <= cnt)
        shiftedChar = shiftedChars.mChars[cnt - skipShiftedChars];
      if (skipUnshiftedChars <= cnt)
        unshiftedChar = unshiftedChars.mChars[cnt - skipUnshiftedChars];
      nsAutoTArray<nsAlternativeCharCode, 5> altArray;

      if (shiftedChar || unshiftedChar) {
        nsAlternativeCharCode chars(unshiftedChar, shiftedChar);
        altArray.AppendElement(chars);
      }
      if (cnt == num - 1) {
        if (unshiftedLatinChar || shiftedLatinChar) {
          nsAlternativeCharCode chars(unshiftedLatinChar, shiftedLatinChar);
          altArray.AppendElement(chars);
        }

        
        
        
        
        
        
        PRUnichar charForOEMKeyCode = 0;
        switch (virtualKeyCode) {
          case VK_OEM_PLUS:   charForOEMKeyCode = '+'; break;
          case VK_OEM_COMMA:  charForOEMKeyCode = ','; break;
          case VK_OEM_MINUS:  charForOEMKeyCode = '-'; break;
          case VK_OEM_PERIOD: charForOEMKeyCode = '.'; break;
        }
        if (charForOEMKeyCode &&
            charForOEMKeyCode != unshiftedChars.mChars[0] &&
            charForOEMKeyCode != shiftedChars.mChars[0] &&
            charForOEMKeyCode != unshiftedLatinChar &&
            charForOEMKeyCode != shiftedLatinChar) {
          nsAlternativeCharCode OEMChars(charForOEMKeyCode, charForOEMKeyCode);
          altArray.AppendElement(OEMChars);
        }
      }

      nsKeyEvent keypressEvent(true, NS_KEY_PRESS, this);
      keypressEvent.mFlags.Union(extraFlags);
      keypressEvent.charCode = uniChar;
      keypressEvent.alternativeCharCodes.AppendElements(altArray);
      InitKeyEvent(keypressEvent, nativeKey, modKeyState);
      DispatchKeyEvent(keypressEvent, nullptr);
    }
  } else {
    nsKeyEvent keypressEvent(true, NS_KEY_PRESS, this);
    keypressEvent.mFlags.Union(extraFlags);
    keypressEvent.keyCode = DOMKeyCode;
    InitKeyEvent(keypressEvent, nativeKey, aModKeyState);
    DispatchKeyEvent(keypressEvent, nullptr);
  }

  return noDefault;
}


LRESULT nsWindow::OnKeyUp(const MSG &aMsg,
                          const ModifierKeyState &aModKeyState,
                          bool *aEventDispatched)
{
  
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("nsWindow::OnKeyUp wParam(VK)=%d\n", aMsg.wParam));

  if (aEventDispatched)
    *aEventDispatched = true;
  nsKeyEvent keyupEvent(true, NS_KEY_UP, this);
  NativeKey nativeKey(gKbdLayout, this, aMsg);
  keyupEvent.keyCode = nativeKey.GetDOMKeyCode();
  InitKeyEvent(keyupEvent, nativeKey, aModKeyState);
  
  
  
  
  
  keyupEvent.mFlags.mDefaultPrevented =
    (aMsg.wParam == VK_MENU && aMsg.message != WM_SYSKEYUP);
  return DispatchKeyEvent(keyupEvent, &aMsg);
}


LRESULT nsWindow::OnChar(const MSG &aMsg,
                         const NativeKey& aNativeKey,
                         const ModifierKeyState &aModKeyState,
                         bool *aEventDispatched,
                         const EventFlags *aExtraFlags)
{
  
  if (aModKeyState.IsAlt() && !aModKeyState.IsControl() &&
      IS_VK_DOWN(NS_VK_SPACE)) {
    return FALSE;
  }

  uint32_t charCode = aMsg.wParam;
  
  if (aModKeyState.IsControl() && charCode == 0xA) {
    return FALSE;
  }

  
  ModifierKeyState modKeyState(aModKeyState);
  if (modKeyState.IsAlt() && modKeyState.IsControl()) {
    modKeyState.Unset(MODIFIER_ALT | MODIFIER_CONTROL);
  }

  if (IMEHandler::IsComposingOn(this)) {
    IMEHandler::NotifyIME(this, REQUEST_TO_COMMIT_COMPOSITION);
  }

  wchar_t uniChar;
  
  if (modKeyState.IsControl() && charCode <= 0x1A) {
    
    if (modKeyState.IsShift()) {
      uniChar = charCode - 1 + 'A';
    } else {
      uniChar = charCode - 1 + 'a';
    }
  } else if (modKeyState.IsControl() && charCode <= 0x1F) {
    
    
    
    
    uniChar = charCode - 1 + 'A';
  } else { 
    if (charCode < 0x20 || (charCode == 0x3D && modKeyState.IsControl())) {
      uniChar = 0;
    } else {
      uniChar = charCode;
    }
  }

  
  
  if (uniChar && (modKeyState.IsControl() || modKeyState.IsAlt())) {
    UINT virtualKeyCode = ::MapVirtualKeyEx(aNativeKey.GetScanCode(),
                                            MAPVK_VSC_TO_VK,
                                            gKbdLayout.GetLayout());
    UINT unshiftedCharCode =
      virtualKeyCode >= '0' && virtualKeyCode <= '9' ? virtualKeyCode :
        modKeyState.IsShift() ? ::MapVirtualKeyEx(virtualKeyCode,
                                                  MAPVK_VK_TO_CHAR,
                                                  gKbdLayout.GetLayout()) : 0;
    
    if ((INT)unshiftedCharCode > 0)
      uniChar = unshiftedCharCode;
  }

  
  
  
  if (!modKeyState.IsShift() &&
      (aModKeyState.IsAlt() || aModKeyState.IsControl())) {
    uniChar = towlower(uniChar);
  }

  nsKeyEvent keypressEvent(true, NS_KEY_PRESS, this);
  if (aExtraFlags) {
    keypressEvent.mFlags.Union(*aExtraFlags);
  }
  keypressEvent.charCode = uniChar;
  if (!keypressEvent.charCode) {
    keypressEvent.keyCode = aNativeKey.GetDOMKeyCode();
  }
  InitKeyEvent(keypressEvent, aNativeKey, modKeyState);
  bool result = DispatchKeyEvent(keypressEvent, &aMsg);
  if (aEventDispatched)
    *aEventDispatched = true;
  return result;
}

void
nsWindow::SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray, uint32_t aModifiers)
{
  for (uint32_t i = 0; i < ArrayLength(sModifierKeyMap); ++i) {
    const uint32_t* map = sModifierKeyMap[i];
    if (aModifiers & map[0]) {
      aArray->AppendElement(KeyPair(map[1], map[2]));
    }
  }
}

static BOOL WINAPI EnumFirstChild(HWND hwnd, LPARAM lParam)
{
  *((HWND*)lParam) = hwnd;
  return FALSE;
}

static void InvalidatePluginAsWorkaround(nsWindow *aWindow, const nsIntRect &aRect)
{
  aWindow->Invalidate(aRect);

  
  
  
  
  
  
  HWND current = (HWND)aWindow->GetNativeData(NS_NATIVE_WINDOW);

  RECT windowRect;
  RECT parentRect;

  ::GetWindowRect(current, &parentRect);
        
  HWND next = current;

  do {
    current = next;

    ::EnumChildWindows(current, &EnumFirstChild, (LPARAM)&next);

    ::GetWindowRect(next, &windowRect);
    
    
    windowRect.left -= parentRect.left;
    windowRect.top -= parentRect.top;
  } while (next != current && windowRect.top == 0 && windowRect.left == 0);

  if (windowRect.top == 0 && windowRect.left == 0) {
    RECT rect;
    rect.left   = aRect.x;
    rect.top    = aRect.y;
    rect.right  = aRect.XMost();
    rect.bottom = aRect.YMost();

    ::InvalidateRect(next, &rect, FALSE);
  }
}

nsresult
nsWindow::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
  
  
  
  for (uint32_t i = 0; i < aConfigurations.Length(); ++i) {
    const Configuration& configuration = aConfigurations[i];
    nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
    NS_ASSERTION(w->GetParent() == this,
                 "Configured widget is not a child");
    nsresult rv = w->SetWindowClipRegion(configuration.mClipRegion, true);
    NS_ENSURE_SUCCESS(rv, rv);
    nsIntRect bounds;
    w->GetBounds(bounds);
    if (bounds.Size() != configuration.mBounds.Size()) {
      w->Resize(configuration.mBounds.x, configuration.mBounds.y,
                configuration.mBounds.width, configuration.mBounds.height,
                true);
    } else if (bounds.TopLeft() != configuration.mBounds.TopLeft()) {
      w->Move(configuration.mBounds.x, configuration.mBounds.y);


      if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
          gfxWindowsPlatform::RENDER_DIRECT2D ||
          GetLayerManager()->GetBackendType() != LAYERS_BASIC) {
        
        
        
        nsIntRegion r;
        r.Sub(bounds, configuration.mBounds);
        r.MoveBy(-bounds.x,
                 -bounds.y);
        nsIntRect toInvalidate = r.GetBounds();

        InvalidatePluginAsWorkaround(w, toInvalidate);
      }
    }
    rv = w->SetWindowClipRegion(configuration.mClipRegion, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

static HRGN
CreateHRGNFromArray(const nsTArray<nsIntRect>& aRects)
{
  int32_t size = sizeof(RGNDATAHEADER) + sizeof(RECT)*aRects.Length();
  nsAutoTArray<uint8_t,100> buf;
  if (!buf.SetLength(size))
    return NULL;
  RGNDATA* data = reinterpret_cast<RGNDATA*>(buf.Elements());
  RECT* rects = reinterpret_cast<RECT*>(data->Buffer);
  data->rdh.dwSize = sizeof(data->rdh);
  data->rdh.iType = RDH_RECTANGLES;
  data->rdh.nCount = aRects.Length();
  nsIntRect bounds;
  for (uint32_t i = 0; i < aRects.Length(); ++i) {
    const nsIntRect& r = aRects[i];
    bounds.UnionRect(bounds, r);
    ::SetRect(&rects[i], r.x, r.y, r.XMost(), r.YMost());
  }
  ::SetRect(&data->rdh.rcBound, bounds.x, bounds.y, bounds.XMost(), bounds.YMost());
  return ::ExtCreateRegion(NULL, buf.Length(), data);
}

static void
ArrayFromRegion(const nsIntRegion& aRegion, nsTArray<nsIntRect>& aRects)
{
  const nsIntRect* r;
  for (nsIntRegionRectIterator iter(aRegion); (r = iter.Next());) {
    aRects.AppendElement(*r);
  }
}

nsresult
nsWindow::SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                              bool aIntersectWithExisting)
{
  if (!aIntersectWithExisting) {
    if (!StoreWindowClipRegion(aRects))
      return NS_OK;
  } else {
    
    if (mClipRects && mClipRectCount == aRects.Length() &&
        memcmp(mClipRects,
               aRects.Elements(),
               sizeof(nsIntRect)*mClipRectCount) == 0) {
      return NS_OK;
    }

    
    nsTArray<nsIntRect> currentRects;
    GetWindowClipRegion(&currentRects);
    
    nsIntRegion currentRegion = RegionFromArray(currentRects);
    
    nsIntRegion newRegion = RegionFromArray(aRects);
    
    nsIntRegion intersection;
    intersection.And(currentRegion, newRegion);
    
    nsTArray<nsIntRect> rects;
    ArrayFromRegion(intersection, rects);
    
    if (!StoreWindowClipRegion(rects))
      return NS_OK;
  }

  HRGN dest = CreateHRGNFromArray(aRects);
  if (!dest)
    return NS_ERROR_OUT_OF_MEMORY;

  if (aIntersectWithExisting) {
    HRGN current = ::CreateRectRgn(0, 0, 0, 0);
    if (current) {
      if (::GetWindowRgn(mWnd, current) != 0 ) {
        ::CombineRgn(dest, dest, current, RGN_AND);
      }
      ::DeleteObject(current);
    }
  }

  
  
  
  
  if(mWindowType == eWindowType_plugin) {
    if(NULLREGION == ::CombineRgn(dest, dest, dest, RGN_OR)) {
      ::ShowWindow(mWnd, SW_HIDE);
      ::EnableWindow(mWnd, FALSE);
    } else {
      ::EnableWindow(mWnd, TRUE);
      ::ShowWindow(mWnd, SW_SHOW);
    }
  }
  if (!::SetWindowRgn(mWnd, dest, TRUE)) {
    ::DeleteObject(dest);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


void nsWindow::OnDestroy()
{
  mOnDestroyCalled = true;

  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  
  
  if (!mInDtor)
    NotifyWindowDestroyed();

  
  mWidgetListener = nullptr;
  mAttachedWidgetListener = nullptr;

  
  
  SubclassWindow(FALSE);

  
  
  if (sCurrentWindow == this)
    sCurrentWindow = nullptr;

  
  nsBaseWidget::Destroy();

  
  nsBaseWidget::OnDestroy();
  
  
  
  
  
  mParent = nullptr;

  
  EnableDragDrop(false);

  
  
  nsIRollupListener* rollupListener = nsBaseWidget::GetActiveRollupListener();
  nsCOMPtr<nsIWidget> rollupWidget;
  if (rollupListener) {
    rollupWidget = rollupListener->GetRollupWidget();
  }
  if (this == rollupWidget) {
    if ( rollupListener )
      rollupListener->Rollup(0, nullptr);
    CaptureRollupEvents(nullptr, false);
  }

  IMEHandler::OnDestroyWindow(this);

  
  MouseTrailer* mtrailer = nsToolkit::gMouseTrailer;
  if (mtrailer) {
    if (mtrailer->GetMouseTrailerWindow() == mWnd)
      mtrailer->DestroyTimer();

    if (mtrailer->GetCaptureWindow() == mWnd)
      mtrailer->SetCaptureWindow(nullptr);
  }

  
  if (mBrush) {
    VERIFY(::DeleteObject(mBrush));
    mBrush = NULL;
  }


  
  if (mCursor == -1)
    SetCursor(eCursor_standard);

#ifdef MOZ_XUL
  
  if (eTransparencyTransparent == mTransparencyMode)
    SetupTranslucentWindowMemoryBitmap(eTransparencyOpaque);
#endif

  
  mGesture.PanFeedbackFinalize(mWnd, true);

  
  mWnd = NULL;
}


bool nsWindow::OnMove(int32_t aX, int32_t aY)
{
  mBounds.x = aX;
  mBounds.y = aY;

  return mWidgetListener ? mWidgetListener->WindowMoved(this, aX, aY) : false;
}


bool nsWindow::OnResize(nsIntRect &aWindowRect)
{
#ifdef CAIRO_HAS_D2D_SURFACE
  if (mD2DWindowSurface) {
    mD2DWindowSurface = NULL;
    Invalidate();
  }
#endif

  bool result = mWidgetListener ?
                mWidgetListener->WindowResized(this, aWindowRect.width, aWindowRect.height) : false;

  
  if (mAttachedWidgetListener) {
    return mAttachedWidgetListener->WindowResized(this, aWindowRect.width, aWindowRect.height);
  }

  return result;
}

bool nsWindow::OnHotKey(WPARAM wParam, LPARAM lParam)
{
  return true;
}


bool nsWindow::AutoErase(HDC dc)
{
  return false;
}

void
nsWindow::AllowD3D9Callback(nsWindow *aWindow)
{
  if (aWindow->mLayerManager) {
    aWindow->mLayerManager->Destroy();
    aWindow->mLayerManager = NULL;
  }
}

void
nsWindow::AllowD3D9WithReinitializeCallback(nsWindow *aWindow)
{
  if (aWindow->mLayerManager) {
    aWindow->mLayerManager->Destroy();
    aWindow->mLayerManager = NULL;
    (void) aWindow->GetLayerManager();
  }
}

void
nsWindow::StartAllowingD3D9(bool aReinitialize)
{
  sAllowD3D9 = true;

  LayerManagerPrefs prefs;
  GetLayerManagerPrefs(&prefs);
  if (prefs.mDisableAcceleration) {
    
    
    
    
    
    
    
    
    
    
    
    return;
  }

  if (aReinitialize) {
    EnumAllWindows(AllowD3D9WithReinitializeCallback);
  } else {
    EnumAllWindows(AllowD3D9Callback);
  }
}

bool
nsWindow::HasBogusPopupsDropShadowOnMultiMonitor() {
  if (sHasBogusPopupsDropShadowOnMultiMonitor == TRI_UNKNOWN) {
    
    
    
    sHasBogusPopupsDropShadowOnMultiMonitor =
      gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
        gfxWindowsPlatform::RENDER_DIRECT2D ? TRI_TRUE : TRI_FALSE;
    if (!sHasBogusPopupsDropShadowOnMultiMonitor) {
      
      LayerManagerPrefs prefs;
      GetLayerManagerPrefs(&prefs);
      if (!prefs.mDisableAcceleration && !prefs.mPreferOpenGL) {
        nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
        if (gfxInfo) {
          int32_t status;
          if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_DIRECT3D_9_LAYERS, &status))) {
            if (status == nsIGfxInfo::FEATURE_NO_INFO || prefs.mForceAcceleration)
            {
              sHasBogusPopupsDropShadowOnMultiMonitor = TRI_TRUE;
            }
          }
        }
      }
    }
  }
  return !!sHasBogusPopupsDropShadowOnMultiMonitor;
}

void
nsWindow::OnSysColorChanged()
{
  if (mWindowType == eWindowType_invisible) {
    ::EnumThreadWindows(GetCurrentThreadId(), nsWindow::BroadcastMsg, WM_SYSCOLORCHANGE);
  }
  else {
    
    
    
    
    
    NotifySysColorChanged();
  }
}











NS_IMETHODIMP
nsWindow::NotifyIME(NotificationToIME aNotification)
{
  return IMEHandler::NotifyIME(this, aNotification);
}

NS_IMETHODIMP_(void)
nsWindow::SetInputContext(const InputContext& aContext,
                          const InputContextAction& aAction)
{
  InputContext newInputContext = aContext;
  IMEHandler::SetInputContext(this, newInputContext);
  mInputContext = newInputContext;
}

NS_IMETHODIMP_(InputContext)
nsWindow::GetInputContext()
{
  mInputContext.mIMEState.mOpen = IMEState::CLOSED;
  if (IMEHandler::IsIMEEnabled(mInputContext) && IMEHandler::GetOpenState(this)) {
    mInputContext.mIMEState.mOpen = IMEState::OPEN;
  } else {
    mInputContext.mIMEState.mOpen = IMEState::CLOSED;
  }
  return mInputContext;
}

NS_IMETHODIMP
nsWindow::GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState)
{
#ifdef DEBUG_KBSTATE
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("GetToggledKeyState\n"));
#endif 
  NS_ENSURE_ARG_POINTER(aLEDState);
  *aLEDState = (::GetKeyState(aKeyCode) & 1) != 0;
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::NotifyIMEOfTextChange(uint32_t aStart,
                                uint32_t aOldEnd,
                                uint32_t aNewEnd)
{
  return IMEHandler::NotifyIMEOfTextChange(aStart, aOldEnd, aNewEnd);
}

nsIMEUpdatePreference
nsWindow::GetIMEUpdatePreference()
{
  return IMEHandler::GetUpdatePreference();
}

#ifdef ACCESSIBILITY

#ifdef DEBUG_WMGETOBJECT
#define NS_LOG_WMGETOBJECT_WNDACC(aWnd)                                        \
  a11y::Accessible* acc = aWnd ? aWind->GetAccessible() : nullptr;             \
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("     acc: %p", acc));                   \
  if (acc) {                                                                   \
    nsAutoString name;                                                         \
    acc->GetName(name);                                                        \
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS,                                         \
           (", accname: %s", NS_ConvertUTF16toUTF8(name).get()));              \
    nsCOMPtr<nsIAccessibleDocument> doc = do_QueryObject(acc);                 \
    void *hwnd = nullptr;                                                      \
    doc->GetWindowHandle(&hwnd);                                               \
    PR_LOG(gWindowsLog, PR_LOG_ALWAYS, (", acc hwnd: %d", hwnd));              \
  }

#define NS_LOG_WMGETOBJECT_THISWND                                             \
{                                                                              \
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,                                           \
         ("\n*******Get Doc Accessible*******\nOrig Window: "));               \
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,                                           \
         ("\n  {\n     HWND: %d, parent HWND: %d, wndobj: %p,\n",              \
          mWnd, ::GetParent(mWnd), this));                                     \
  NS_LOG_WMGETOBJECT_WNDACC(this)                                              \
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("\n  }\n"));                             \
}

#define NS_LOG_WMGETOBJECT_WND(aMsg, aHwnd)                                    \
{                                                                              \
  nsWindow* wnd = WinUtils::GetNSWindowPtr(aHwnd);                             \
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,                                           \
         ("Get " aMsg ":\n  {\n     HWND: %d, parent HWND: %d, wndobj: %p,\n", \
          aHwnd, ::GetParent(aHwnd), wnd));                                    \
  NS_LOG_WMGETOBJECT_WNDACC(wnd);                                              \
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("\n }\n"));                              \
}
#else
#define NS_LOG_WMGETOBJECT_THISWND
#define NS_LOG_WMGETOBJECT_WND(aMsg, aHwnd)
#endif 

a11y::Accessible*
nsWindow::GetRootAccessible()
{
  
  if (a11y::PlatformDisabledState() == a11y::ePlatformIsDisabled)
    return nullptr;

  if (mInDtor || mOnDestroyCalled || mWindowType == eWindowType_invisible) {
    return nullptr;
  }

  NS_LOG_WMGETOBJECT_THISWND
  NS_LOG_WMGETOBJECT_WND("This Window", mWnd);

  return GetAccessible();
}
#endif











#ifdef MOZ_XUL

void nsWindow::ResizeTranslucentWindow(int32_t aNewWidth, int32_t aNewHeight, bool force)
{
  if (!force && aNewWidth == mBounds.width && aNewHeight == mBounds.height)
    return;

#ifdef CAIRO_HAS_D2D_SURFACE
  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
    nsRefPtr<gfxD2DSurface> newSurface =
      new gfxD2DSurface(gfxIntSize(aNewWidth, aNewHeight), gfxASurface::ImageFormatARGB32);
    mTransparentSurface = newSurface;
    mMemoryDC = nullptr;
  } else
#endif
  {
    nsRefPtr<gfxWindowsSurface> newSurface =
      new gfxWindowsSurface(gfxIntSize(aNewWidth, aNewHeight), gfxASurface::ImageFormatARGB32);
    mTransparentSurface = newSurface;
    mMemoryDC = newSurface->GetDC();
  }
}

void nsWindow::SetWindowTranslucencyInner(nsTransparencyMode aMode)
{
  if (aMode == mTransparencyMode)
    return;

  
  HWND hWnd = WinUtils::GetTopLevelHWND(mWnd, true);
  nsWindow* parent = WinUtils::GetNSWindowPtr(hWnd);

  if (!parent)
  {
    NS_WARNING("Trying to use transparent chrome in an embedded context");
    return;
  }

  if (parent != this) {
    NS_WARNING("Setting SetWindowTranslucencyInner on a parent this is not us!");
  }

  if (aMode == eTransparencyTransparent) {
    
    
    HideWindowChrome(true);
  } else if (mHideChrome && mTransparencyMode == eTransparencyTransparent) {
    
    HideWindowChrome(false);
  }

  LONG_PTR style = ::GetWindowLongPtrW(hWnd, GWL_STYLE),
    exStyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
 
   if (parent->mIsVisible)
     style |= WS_VISIBLE;
   if (parent->mSizeMode == nsSizeMode_Maximized)
     style |= WS_MAXIMIZE;
   else if (parent->mSizeMode == nsSizeMode_Minimized)
     style |= WS_MINIMIZE;

   if (aMode == eTransparencyTransparent)
     exStyle |= WS_EX_LAYERED;
   else
     exStyle &= ~WS_EX_LAYERED;

  VERIFY_WINDOW_STYLE(style);
  ::SetWindowLongPtrW(hWnd, GWL_STYLE, style);
  ::SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle);

  if (HasGlass())
    memset(&mGlassMargins, 0, sizeof mGlassMargins);
  mTransparencyMode = aMode;

  SetupTranslucentWindowMemoryBitmap(aMode);
  UpdateGlass();
}

void nsWindow::SetupTranslucentWindowMemoryBitmap(nsTransparencyMode aMode)
{
  if (eTransparencyTransparent == aMode) {
    ResizeTranslucentWindow(mBounds.width, mBounds.height, true);
  } else {
    mTransparentSurface = nullptr;
    mMemoryDC = NULL;
  }
}

void nsWindow::ClearTranslucentWindow()
{
  if (mTransparentSurface) {
    nsRefPtr<gfxContext> thebesContext = new gfxContext(mTransparentSurface);
    thebesContext->SetOperator(gfxContext::OPERATOR_CLEAR);
    thebesContext->Paint();
    UpdateTranslucentWindow();
 }
}

nsresult nsWindow::UpdateTranslucentWindow()
{
  if (mBounds.IsEmpty())
    return NS_OK;

  ::GdiFlush();

  BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
  SIZE winSize = { mBounds.width, mBounds.height };
  POINT srcPos = { 0, 0 };
  HWND hWnd = WinUtils::GetTopLevelHWND(mWnd, true);
  RECT winRect;
  ::GetWindowRect(hWnd, &winRect);

#ifdef CAIRO_HAS_D2D_SURFACE
  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
    mMemoryDC = static_cast<gfxD2DSurface*>(mTransparentSurface.get())->
      GetDC(true);
  }
#endif
  
  bool updateSuccesful = 
    ::UpdateLayeredWindow(hWnd, NULL, (POINT*)&winRect, &winSize, mMemoryDC, &srcPos, 0, &bf, ULW_ALPHA);

#ifdef CAIRO_HAS_D2D_SURFACE
  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
    nsIntRect r(0, 0, 0, 0);
    static_cast<gfxD2DSurface*>(mTransparentSurface.get())->ReleaseDC(&r);
  }
#endif

  if (!updateSuccesful) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

#endif 












void nsWindow::ScheduleHookTimer(HWND aWnd, UINT aMsgId)
{
  
  
  if (sHookTimerId == 0) {
    
    sRollupMsgId = aMsgId;
    sRollupMsgWnd = aWnd;
    
    
    sHookTimerId = ::SetTimer(NULL, 0, 0, (TIMERPROC)HookTimerForPopups);
    NS_ASSERTION(sHookTimerId, "Timer couldn't be created.");
  }
}

#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
int gLastMsgCode = 0;
extern MSGFEventMsgInfo gMSGFEvents[];
#endif


LRESULT CALLBACK nsWindow::MozSpecialMsgFilter(int code, WPARAM wParam, LPARAM lParam)
{
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
  if (sProcessHook) {
    MSG* pMsg = (MSG*)lParam;

    int inx = 0;
    while (gMSGFEvents[inx].mId != code && gMSGFEvents[inx].mStr != NULL) {
      inx++;
    }
    if (code != gLastMsgCode) {
      if (gMSGFEvents[inx].mId == code) {
#ifdef DEBUG
        PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
               ("MozSpecialMessageProc - code: 0x%X  - %s  hw: %p\n", 
                code, gMSGFEvents[inx].mStr, pMsg->hwnd));
#endif
      } else {
#ifdef DEBUG
        PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
               ("MozSpecialMessageProc - code: 0x%X  - %d  hw: %p\n", 
                code, gMSGFEvents[inx].mId, pMsg->hwnd));
#endif
      }
      gLastMsgCode = code;
    }
    PrintEvent(pMsg->message, FALSE, FALSE);
  }
#endif 

  if (sProcessHook && code == MSGF_MENU) {
    MSG* pMsg = (MSG*)lParam;
    ScheduleHookTimer( pMsg->hwnd, pMsg->message);
  }

  return ::CallNextHookEx(sMsgFilterHook, code, wParam, lParam);
}



LRESULT CALLBACK nsWindow::MozSpecialMouseProc(int code, WPARAM wParam, LPARAM lParam)
{
  if (sProcessHook) {
    switch (WinUtils::GetNativeMessage(wParam)) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL:
      {
        MOUSEHOOKSTRUCT* ms = (MOUSEHOOKSTRUCT*)lParam;
        nsIWidget* mozWin = WinUtils::GetNSWindowPtr(ms->hwnd);
        if (mozWin) {
          
          
          if (static_cast<nsWindow*>(mozWin)->mWindowType == eWindowType_plugin)
            ScheduleHookTimer(ms->hwnd, (UINT)wParam);
        } else {
          ScheduleHookTimer(ms->hwnd, (UINT)wParam);
        }
        break;
      }
    }
  }
  return ::CallNextHookEx(sCallMouseHook, code, wParam, lParam);
}



LRESULT CALLBACK nsWindow::MozSpecialWndProc(int code, WPARAM wParam, LPARAM lParam)
{
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
  if (sProcessHook) {
    CWPSTRUCT* cwpt = (CWPSTRUCT*)lParam;
    PrintEvent(cwpt->message, FALSE, FALSE);
  }
#endif

  if (sProcessHook) {
    CWPSTRUCT* cwpt = (CWPSTRUCT*)lParam;
    if (cwpt->message == WM_MOVING ||
        cwpt->message == WM_SIZING ||
        cwpt->message == WM_GETMINMAXINFO) {
      ScheduleHookTimer(cwpt->hwnd, (UINT)cwpt->message);
    }
  }

  return ::CallNextHookEx(sCallProcHook, code, wParam, lParam);
}


void nsWindow::RegisterSpecialDropdownHooks()
{
  NS_ASSERTION(!sMsgFilterHook, "sMsgFilterHook must be NULL!");
  NS_ASSERTION(!sCallProcHook,  "sCallProcHook must be NULL!");

  DISPLAY_NMM_PRT("***************** Installing Msg Hooks ***************\n");

  
  if (!sMsgFilterHook) {
    DISPLAY_NMM_PRT("***** Hooking sMsgFilterHook!\n");
    sMsgFilterHook = SetWindowsHookEx(WH_MSGFILTER, MozSpecialMsgFilter, NULL, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sMsgFilterHook) {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
             ("***** SetWindowsHookEx is NOT installed for WH_MSGFILTER!\n"));
    }
#endif
  }

  
  if (!sCallProcHook) {
    DISPLAY_NMM_PRT("***** Hooking sCallProcHook!\n");
    sCallProcHook  = SetWindowsHookEx(WH_CALLWNDPROC, MozSpecialWndProc, NULL, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sCallProcHook) {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
             ("***** SetWindowsHookEx is NOT installed for WH_CALLWNDPROC!\n"));
    }
#endif
  }

  
  if (!sCallMouseHook) {
    DISPLAY_NMM_PRT("***** Hooking sCallMouseHook!\n");
    sCallMouseHook  = SetWindowsHookEx(WH_MOUSE, MozSpecialMouseProc, NULL, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sCallMouseHook) {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
             ("***** SetWindowsHookEx is NOT installed for WH_MOUSE!\n"));
    }
#endif
  }
}


void nsWindow::UnregisterSpecialDropdownHooks()
{
  DISPLAY_NMM_PRT("***************** De-installing Msg Hooks ***************\n");

  if (sCallProcHook) {
    DISPLAY_NMM_PRT("***** Unhooking sCallProcHook!\n");
    if (!::UnhookWindowsHookEx(sCallProcHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for sCallProcHook!\n");
    }
    sCallProcHook = NULL;
  }

  if (sMsgFilterHook) {
    DISPLAY_NMM_PRT("***** Unhooking sMsgFilterHook!\n");
    if (!::UnhookWindowsHookEx(sMsgFilterHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for sMsgFilterHook!\n");
    }
    sMsgFilterHook = NULL;
  }

  if (sCallMouseHook) {
    DISPLAY_NMM_PRT("***** Unhooking sCallMouseHook!\n");
    if (!::UnhookWindowsHookEx(sCallMouseHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for sCallMouseHook!\n");
    }
    sCallMouseHook = NULL;
  }
}








VOID CALLBACK nsWindow::HookTimerForPopups(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
  if (sHookTimerId != 0) {
    
    BOOL status = ::KillTimer(NULL, sHookTimerId);
    NS_ASSERTION(status, "Hook Timer was not killed.");
    sHookTimerId = 0;
  }

  if (sRollupMsgId != 0) {
    
    LRESULT popupHandlingResult;
    nsAutoRollup autoRollup;
    DealWithPopups(sRollupMsgWnd, sRollupMsgId, 0, 0, &popupHandlingResult);
    sRollupMsgId = 0;
    sRollupMsgWnd = NULL;
  }
}

BOOL CALLBACK nsWindow::ClearResourcesCallback(HWND aWnd, LPARAM aMsg)
{
    nsWindow *window = WinUtils::GetNSWindowPtr(aWnd);
    if (window) {
        window->ClearCachedResources();
    }  
    return TRUE;
}

void
nsWindow::ClearCachedResources()
{
#ifdef CAIRO_HAS_D2D_SURFACE
    mD2DWindowSurface = nullptr;
#endif
    if (mLayerManager &&
        mLayerManager->GetBackendType() == LAYERS_BASIC) {
      static_cast<BasicLayerManager*>(mLayerManager.get())->
        ClearCachedResources();
    }
    ::EnumChildWindows(mWnd, nsWindow::ClearResourcesCallback, 0);
}

static bool IsDifferentThreadWindow(HWND aWnd)
{
  return ::GetCurrentThreadId() != ::GetWindowThreadProcessId(aWnd, NULL);
}

bool
nsWindow::EventIsInsideWindow(UINT Msg, nsWindow* aWindow)
{
  RECT r;

  if (Msg == WM_ACTIVATEAPP)
    
    return false;

  ::GetWindowRect(aWindow->mWnd, &r);
  DWORD pos = ::GetMessagePos();
  POINT mp;
  mp.x = GET_X_LPARAM(pos);
  mp.y = GET_Y_LPARAM(pos);

  
  return (bool) PtInRect(&r, mp);
}


bool
nsWindow::DealWithPopups(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam, LRESULT* outResult)
{
  NS_ASSERTION(outResult, "Bad outResult");

  *outResult = MA_NOACTIVATE;

  if (!::IsWindowVisible(inWnd))
    return false;
  nsIRollupListener* rollupListener = nsBaseWidget::GetActiveRollupListener();
  NS_ENSURE_TRUE(rollupListener, false);
  nsCOMPtr<nsIWidget> rollupWidget = rollupListener->GetRollupWidget();
  if (!rollupWidget)
    return false;

  inMsg = WinUtils::GetNativeMessage(inMsg);
  if (inMsg == WM_LBUTTONDOWN || inMsg == WM_RBUTTONDOWN || inMsg == WM_MBUTTONDOWN ||
      inMsg == WM_MOUSEWHEEL || inMsg == WM_MOUSEHWHEEL || inMsg == WM_ACTIVATE ||
      (inMsg == WM_KILLFOCUS && IsDifferentThreadWindow((HWND)inWParam)) ||
      inMsg == WM_NCRBUTTONDOWN ||
      inMsg == WM_MOVING ||
      inMsg == WM_SIZING ||
      inMsg == WM_NCLBUTTONDOWN ||
      inMsg == WM_NCMBUTTONDOWN ||
      inMsg == WM_MOUSEACTIVATE ||
      inMsg == WM_ACTIVATEAPP ||
      inMsg == WM_MENUSELECT) {
    
    bool rollup = !nsWindow::EventIsInsideWindow(inMsg, (nsWindow*)(rollupWidget.get()));

    if (rollup && (inMsg == WM_MOUSEWHEEL || inMsg == WM_MOUSEHWHEEL)) {
      rollup = rollupListener->ShouldRollupOnMouseWheelEvent();
      *outResult = MA_ACTIVATE;
    }

    
    
    uint32_t popupsToRollup = UINT32_MAX;
    if (rollup) {
      nsAutoTArray<nsIWidget*, 5> widgetChain;
      uint32_t sameTypeCount = rollupListener->GetSubmenuWidgetChain(&widgetChain);
      for ( uint32_t i = 0; i < widgetChain.Length(); ++i ) {
        nsIWidget* widget = widgetChain[i];
        if ( nsWindow::EventIsInsideWindow(inMsg, (nsWindow*)widget) ) {
          
          
          
          
          if (i < sameTypeCount) {
            rollup = false;
          } else {
            popupsToRollup = sameTypeCount;
          }
          break;
        }
      } 
    }

    if (inMsg == WM_MOUSEACTIVATE) {
      
      
      
      
      if (!rollup) {
        return true;
      } else {
        UINT uMsg = HIWORD(inLParam);
        if (uMsg == WM_MOUSEMOVE) {
          
          
          rollup = rollupListener->ShouldRollupOnMouseActivate();
          if (!rollup) {
            return true;
          }
        }
      }
    }
    
    else if (rollup) {
      
      NS_ASSERTION(!mLastRollup, "mLastRollup is null");
      bool consumeRollupEvent =
        rollupListener->Rollup(popupsToRollup, inMsg == WM_LBUTTONDOWN ? &mLastRollup : nullptr);
      NS_IF_ADDREF(mLastRollup);

      
      sProcessHook = false;
      sRollupMsgId = 0;
      sRollupMsgWnd = NULL;

      
      
      
      
      if (consumeRollupEvent && inMsg != WM_RBUTTONDOWN) {
        *outResult = MA_ACTIVATE;

        
        if (inMsg == WM_MOUSEACTIVATE) {
          nsWindow* activateWindow = WinUtils::GetNSWindowPtr(inWnd);
          if (activateWindow) {
            nsWindowType wintype;
            activateWindow->GetWindowType(wintype);
            if (wintype == eWindowType_popup && activateWindow->PopupType() == ePopupTypePanel) {
              *outResult = popupsToRollup != UINT32_MAX ? MA_NOACTIVATEANDEAT : MA_NOACTIVATE;
            }
          }
        }
        return true;
      }
      
      
      
      if (popupsToRollup != UINT32_MAX && inMsg == WM_MOUSEACTIVATE) {
        *outResult = MA_NOACTIVATEANDEAT;
        return true;
      }
    }
  } 

  return false;
}















nsWindow* nsWindow::GetTopLevelWindow(bool aStopOnDialogOrPopup)
{
  nsWindow* curWindow = this;

  while (true) {
    if (aStopOnDialogOrPopup) {
      switch (curWindow->mWindowType) {
        case eWindowType_dialog:
        case eWindowType_popup:
          return curWindow;
        default:
          break;
      }
    }

    
    nsWindow* parentWindow = curWindow->GetParentWindow(true);

    if (!parentWindow)
      return curWindow;

    curWindow = parentWindow;
  }
}

static BOOL CALLBACK gEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  DWORD pid;
  ::GetWindowThreadProcessId(hwnd, &pid);
  if (pid == GetCurrentProcessId() && ::IsWindowVisible(hwnd))
  {
    gWindowsVisible = true;
    return FALSE;
  }
  return TRUE;
}

bool nsWindow::CanTakeFocus()
{
  gWindowsVisible = false;
  EnumWindows(gEnumWindowsProc, 0);
  if (!gWindowsVisible) {
    return true;
  } else {
    HWND fgWnd = ::GetForegroundWindow();
    if (!fgWnd) {
      return true;
    }
    DWORD pid;
    GetWindowThreadProcessId(fgWnd, &pid);
    if (pid == GetCurrentProcessId()) {
      return true;
    }
  }
  return false;
}

void nsWindow::GetMainWindowClass(nsAString& aClass)
{
  NS_PRECONDITION(aClass.IsEmpty(), "aClass should be empty string");
  nsresult rv = Preferences::GetString("ui.window_class_override", &aClass);
  if (NS_FAILED(rv) || aClass.IsEmpty()) {
    aClass.AssignASCII(sDefaultMainWindowClass);
  }
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

void nsWindow::PickerOpen()
{
  mPickerDisplayCount++;
}

void nsWindow::PickerClosed()
{
  NS_ASSERTION(mPickerDisplayCount > 0, "mPickerDisplayCount out of sync!");
  if (!mPickerDisplayCount)
    return;
  mPickerDisplayCount--;
  if (!mPickerDisplayCount && mDestroyCalled) {
    Destroy();
  }
}












DWORD ChildWindow::WindowStyle()
{
  DWORD style = WS_CLIPCHILDREN | nsWindow::WindowStyle();
  if (!(style & WS_POPUP))
    style |= WS_CHILD; 
  VERIFY_WINDOW_STYLE(style);
  return style;
}

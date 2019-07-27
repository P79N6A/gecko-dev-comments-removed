























































#include "gfx2DGlue.h"
#include "gfxPlatform.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TouchEvents.h"

#include "mozilla/ipc/MessageChannel.h"
#include <algorithm>
#include <limits>

#include "nsWindow.h"

#include <shellapi.h>
#include <windows.h>
#include <wtsapi32.h>
#include <process.h>
#include <commctrl.h>
#include <unknwn.h>
#include <psapi.h>

#include "prlog.h"
#include "prtime.h"
#include "prprf.h"
#include "prmem.h"
#include "prenv.h"

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
#include "nsLayoutUtils.h"
#include "nsView.h"
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
#include "mozilla/dom/Touch.h"
#include "mozilla/gfx/2D.h"
#include "nsToolkitCompsCID.h"
#include "nsIAppStartup.h"
#include "mozilla/WindowsVersion.h"
#include "mozilla/TextEvents.h" 
#include "nsThemeConstants.h"

#include "nsIGfxInfo.h"
#include "nsUXThemeConstants.h"
#include "KeyboardLayout.h"
#include "nsNativeDragTarget.h"
#include <mmsystem.h> 
#include <zmouse.h>
#include <richedit.h>

#if defined(ACCESSIBILITY)

#ifdef DEBUG
#include "mozilla/a11y/Logging.h"
#endif

#include "oleidl.h"
#include <winuser.h>
#include "nsAccessibilityService.h"
#include "mozilla/a11y/DocAccessible.h"
#include "mozilla/a11y/Platform.h"
#if !defined(WINABLEAPI)
#include <winable.h>
#endif 
#endif 

#include "nsIWinTaskbar.h"
#define NS_TASKBAR_CONTRACTID "@mozilla.org/windows-taskbar;1"

#include "nsWindowDefs.h"

#include "nsCrashOnException.h"
#include "nsIXULRuntime.h"

#include "nsIContent.h"

#include "mozilla/HangMonitor.h"
#include "WinIMEHandler.h"

#include "npapi.h"

#include <d3d11.h>

#if !defined(SM_CONVERTIBLESLATEMODE)
#define SM_CONVERTIBLESLATEMODE 0x2003
#endif

#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/InputAPZContext.h"
#include "InputData.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using namespace mozilla::widget;

namespace mozilla {
namespace widget {
  extern int32_t IsTouchDeviceSupportPresent();
}
}

















bool            nsWindow::sDropShadowEnabled      = true;
uint32_t        nsWindow::sInstanceCount          = 0;
bool            nsWindow::sSwitchKeyboardLayout   = false;
BOOL            nsWindow::sIsOleInitialized       = FALSE;
HCURSOR         nsWindow::sHCursor                = nullptr;
imgIContainer*  nsWindow::sCursorImgContainer     = nullptr;
nsWindow*       nsWindow::sCurrentWindow          = nullptr;
bool            nsWindow::sJustGotDeactivate      = false;
bool            nsWindow::sJustGotActivate        = false;
bool            nsWindow::sIsInMouseCapture       = false;


TriStateBool    nsWindow::sCanQuit                = TRI_UNKNOWN;




HHOOK           nsWindow::sMsgFilterHook          = nullptr;
HHOOK           nsWindow::sCallProcHook           = nullptr;
HHOOK           nsWindow::sCallMouseHook          = nullptr;
bool            nsWindow::sProcessHook            = false;
UINT            nsWindow::sRollupMsgId            = 0;
HWND            nsWindow::sRollupMsgWnd           = nullptr;
UINT            nsWindow::sHookTimerId            = 0;



POINT           nsWindow::sLastMousePoint         = {0};
POINT           nsWindow::sLastMouseMovePoint     = {0};
LONG            nsWindow::sLastMouseDownTime      = 0L;
LONG            nsWindow::sLastClickCount         = 0L;
BYTE            nsWindow::sLastMouseButton        = 0;


int             nsWindow::sTrimOnMinimize         = 2;


const char*     nsWindow::sDefaultMainWindowClass = kClassNameGeneral;

TriStateBool nsWindow::sHasBogusPopupsDropShadowOnMultiMonitor = TRI_UNKNOWN;

DWORD           nsWindow::sFirstEventTime = 0;
TimeStamp       nsWindow::sFirstEventTimeStamp = TimeStamp();







static const char *sScreenManagerContractID       = "@mozilla.org/gfx/screenmanager;1";

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif


static bool     gWindowsVisible                   = false;


static bool     gIsSleepMode                      = false;

static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);


static WindowsDllInterceptor sUser32Intercept;




static const int32_t kGlassMarginAdjustment = 2;





static const int32_t kResizableBorderMinSize = 3;


static bool gIsPointerEventsEnabled = false;






#define MAX_ACCELERATED_DIMENSION 8192





#define HITTEST_CACHE_LIFETIME_MS 50


static const DWORD kEventTimeRange = std::numeric_limits<DWORD>::max();
static const DWORD kEventTimeHalfRange = kEventTimeRange / 2;



















nsWindow::nsWindow() : nsWindowBase()
{
  mIconSmall            = nullptr;
  mIconBig              = nullptr;
  mWnd                  = nullptr;
  mPaintDC              = nullptr;
  mCompositeDC          = nullptr;
  mPrevWndProc          = nullptr;
  mNativeDragTarget     = nullptr;
  mInDtor               = false;
  mIsVisible            = false;
  mIsTopWidgetWindow    = false;
  mUnicodeWidget        = true;
  mDisplayPanFeedback   = false;
  mTouchWindow          = false;
  mFutureMarginsToUse   = false;
  mCustomNonClient      = false;
  mHideChrome           = false;
  mFullscreenMode       = false;
  mMousePresent         = false;
  mDestroyCalled        = false;
  mHasTaskbarIconBeenCreated = false;
  mMouseTransparent     = false;
  mPickerDisplayCount   = 0;
  mWindowType           = eWindowType_child;
  mBorderStyle          = eBorderStyle_default;
  mOldSizeMode          = nsSizeMode_Normal;
  mLastSizeMode         = nsSizeMode_Normal;
  mLastSize.width       = 0;
  mLastSize.height      = 0;
  mOldStyle             = 0;
  mOldExStyle           = 0;
  mPainting             = 0;
  mLastKeyboardLayout   = 0;
  mBlurSuppressLevel    = 0;
  mLastPaintEndTime     = TimeStamp::Now();
  mCachedHitTestPoint.x = 0;
  mCachedHitTestPoint.y = 0;
  mCachedHitTestTime    = TimeStamp::Now();
  mCachedHitTestResult  = 0;
#ifdef MOZ_XUL
  mTransparentSurface   = nullptr;
  mMemoryDC             = nullptr;
  mTransparencyMode     = eTransparencyOpaque;
  memset(&mGlassMargins, 0, sizeof mGlassMargins);
#endif
  DWORD background      = ::GetSysColor(COLOR_BTNFACE);
  mBrush                = ::CreateSolidBrush(NSRGB_2_COLOREF(background));

  mTaskbarPreview = nullptr;

  
  if (!sInstanceCount) {
    
    
    mozilla::widget::WinTaskbar::RegisterAppUserModelID();
    KeyboardLayout::GetInstance()->OnLayoutChange(::GetKeyboardLayout(0));
    IMEHandler::Initialize();
    if (SUCCEEDED(::OleInitialize(nullptr))) {
      sIsOleInitialized = TRUE;
    }
    NS_ASSERTION(sIsOleInitialized, "***** OLE is not initialized!\n");
    MouseScrollHandler::Initialize();
    
    nsUXThemeData::InitTitlebarInfo();
    
    nsUXThemeData::UpdateNativeThemeInfo();
    RedirectedKeyDownMessageManager::Forget();

    Preferences::AddBoolVarCache(&gIsPointerEventsEnabled,
                                 "dom.w3c_pointer_events.enabled",
                                 gIsPointerEventsEnabled);
  } 

  mIdleService = nullptr;

  sInstanceCount++;
}

nsWindow::~nsWindow()
{
  mInDtor = true;

  
  
  
  
  if (nullptr != mWnd)
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

  BaseCreate(baseParent, aRect, aInitData);

  HWND parent;
  if (aParent) { 
    parent = aParent ? (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW) : nullptr;
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
      parent = nullptr;
    }

    if (IsVistaOrLater() && !IsWin8OrLater() &&
        HasBogusPopupsDropShadowOnMultiMonitor()) {
      extendedStyle |= WS_EX_COMPOSITED;
    }

    if (aInitData->mMouseTransparent) {
      
      mMouseTransparent = true;
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
  
  
  
  if (aInitData->mWindowType == eWindowType_plugin ||
      aInitData->mWindowType == eWindowType_plugin_ipc_chrome ||
      aInitData->mWindowType == eWindowType_plugin_ipc_content) {
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
                           nullptr,
                           nsToolkit::mDllInstance,
                           nullptr);

  if (!mWnd) {
    NS_WARNING("nsWindow CreateWindowEx failed.");
    return NS_ERROR_FAILURE;
  }

  if (mIsRTL && WinUtils::dwmSetWindowAttributePtr) {
    DWORD dwAttribute = TRUE;    
    WinUtils::dwmSetWindowAttributePtr(mWnd, DWMWA_NONCLIENT_RTL_LAYOUT, &dwAttribute, sizeof dwAttribute);
  }

  if (!IsPlugin() &&
      mWindowType != eWindowType_invisible &&
      MouseScrollHandler::Device::IsFakeScrollableWindowNeeded()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    HWND scrollContainerWnd = ::CreateWindowW
      (className.get(), L"FAKETRACKPOINTSCROLLCONTAINER",
       WS_CHILD | WS_VISIBLE,
       0, 0, 0, 0, mWnd, nullptr, nsToolkit::mDllInstance, nullptr);
    HWND scrollableWnd = ::CreateWindowW
      (className.get(), L"FAKETRACKPOINTSCROLLABLE",
       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | 0x30,
       0, 0, 0, 0, scrollContainerWnd, nullptr, nsToolkit::mDllInstance,
       nullptr);

    
    
    
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

  
  
  
  
  DebugOnly<BOOL> wtsRegistered = ::WTSRegisterSessionNotification(mWnd,
                                                       NOTIFY_FOR_THIS_SESSION);
  NS_ASSERTION(wtsRegistered, "WTSRegisterSessionNotification failed!\n");

  IMEHandler::InitInputContext(this, mInputContext);

  
  
  
  
  if (sTrimOnMinimize == 2 && mWindowType == eWindowType_invisible) {
    
    
    
    
    sTrimOnMinimize =
      Preferences::GetBool("config.trim_on_minimize",
        IsVistaOrLater() ? 1 : 0);
    sSwitchKeyboardLayout =
      Preferences::GetBool("intl.keyboard.per_window_layout", false);
  }

  
  
  if (mWindowType == eWindowType_toplevel &&
      (!nsUXThemeData::sTitlebarInfoPopulatedThemed ||
       !nsUXThemeData::sTitlebarInfoPopulatedAero)) {
    nsUXThemeData::UpdateTitlebarInfo(mWnd);
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

  



  DestroyLayerManager();

  

  ClearCachedResources();

  
  
  
  
  
  
  
  
  
  
  VERIFY(::DestroyWindow(mWnd));
  
  
  
  if (false == mOnDestroyCalled) {
    MSGResult msgResult;
    mWindowHook.Notify(mWnd, WM_DESTROY, 0, 0, msgResult);
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
  wc.hIcon         = aIconID ? ::LoadIconW(::GetModuleHandleW(nullptr), aIconID) : nullptr;
  wc.hCursor       = nullptr;
  wc.hbrBackground = mBrush;
  wc.lpszMenuName  = nullptr;
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
    case eWindowType_plugin_ipc_chrome:
    case eWindowType_plugin_ipc_content:
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
    case eWindowType_plugin_ipc_chrome:
    case eWindowType_plugin_ipc_content:
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
    
    WinUtils::SetNSWindowBasePtr(mWnd, this);
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
    WinUtils::SetNSWindowBasePtr(mWnd, nullptr);
    mPrevWndProc = nullptr;
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
  return WinUtils::LogToPhysFactor();
}

nsWindow*
nsWindow::GetParentWindow(bool aIncludeOwner)
{
  return static_cast<nsWindow*>(GetParentWindowBase(aIncludeOwner));
}

nsWindowBase*
nsWindow::GetParentWindowBase(bool aIncludeOwner)
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

  return static_cast<nsWindowBase*>(widget);
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
  if (IsVistaOrLater() && !HasGlass() &&
      (mWindowType == eWindowType_popup && !IsPopupWithTitleBar() &&
       (mPopupType == ePopupTypeTooltip || mPopupType == ePopupTypePanel))) {
    SetWindowRgn(mWnd, nullptr, false);
  }
}

void nsWindow::SetThemeRegion()
{
  
  
  
  
  
  if (IsVistaOrLater() && !HasGlass() &&
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







void nsWindow::ConfigureAPZCTreeManager()
{
  nsBaseWidget::ConfigureAPZCTreeManager();

  
  
  
  
  
  
  RegisterTouchWindow();
}

void nsWindow::RegisterTouchWindow() {
  if (Preferences::GetInt("dom.w3c_touch_events.enabled", 0) ||
      gIsPointerEventsEnabled) {
    mTouchWindow = true;
    mGesture.RegisterTouchWindow(mWnd);
    ::EnumChildWindows(mWnd, nsWindow::RegisterTouchForDescendants, 0);
  }
}

BOOL CALLBACK nsWindow::RegisterTouchForDescendants(HWND aWnd, LPARAM aMsg) {
  nsWindow* win = WinUtils::GetNSWindowPtr(aWnd);
  if (win)
    win->mGesture.RegisterTouchWindow(aWnd);
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

  
  
  CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels() ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
  int32_t x = NSToIntRound(aX * scale.scale);
  int32_t y = NSToIntRound(aY * scale.scale);

  
  
  
  

  
  
  
  if (mWindowType != eWindowType_popup && (mBounds.x == x) && (mBounds.y == y))
  {
    
    return NS_OK;
  }

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
    
    
    
    if (IsPlugin() &&
        (!mLayerManager || mLayerManager->GetBackendType() == LayersBackend::LAYERS_D3D9) &&
        mClipRects &&
        (mClipRectCount != 1 || !mClipRects[0].IsEqualInterior(nsIntRect(0, 0, mBounds.width, mBounds.height)))) {
      flags |= SWP_NOCOPYBITS;
    }
    VERIFY(::SetWindowPos(mWnd, nullptr, x, y, 0, 0, flags));

    SetThemeRegion();
  }
  NotifyRollupGeometryChange();
  return NS_OK;
}


NS_METHOD nsWindow::Resize(double aWidth, double aHeight, bool aRepaint)
{
  
  
  CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels() ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
  int32_t width = NSToIntRound(aWidth * scale.scale);
  int32_t height = NSToIntRound(aHeight * scale.scale);

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
    VERIFY(::SetWindowPos(mWnd, nullptr, 0, 0,
                          width, GetHeight(height), flags));
    SetThemeRegion();
  }

  if (aRepaint)
    Invalidate();

  NotifyRollupGeometryChange();
  return NS_OK;
}


NS_METHOD nsWindow::Resize(double aX, double aY, double aWidth, double aHeight, bool aRepaint)
{
  
  
  CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels() ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
  int32_t x = NSToIntRound(aX * scale.scale);
  int32_t y = NSToIntRound(aY * scale.scale);
  int32_t width = NSToIntRound(aWidth * scale.scale);
  int32_t height = NSToIntRound(aHeight * scale.scale);

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
    VERIFY(::SetWindowPos(mWnd, nullptr, x, y,
                          width, GetHeight(height), flags));
    SetThemeRegion();
  }

  if (aRepaint)
    Invalidate();

  NotifyRollupGeometryChange();
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::BeginResizeDrag(WidgetGUIEvent* aEvent,
                          int32_t aHorizontal,
                          int32_t aVertical)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  if (aEvent->mClass != eMouseEventClass) {
    
    return NS_ERROR_INVALID_ARG;
  }

  if (aEvent->AsMouseEvent()->button != WidgetMouseEvent::eLeftButton) {
    
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

  double dpiScale = GetDefaultScale().scale;

  
  int32_t logWidth = std::max<int32_t>(NSToIntRound(mBounds.width / dpiScale), 1);
  int32_t logHeight = std::max<int32_t>(NSToIntRound(mBounds.height / dpiScale), 1);

  

  RECT screenRect;

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    int32_t left, top, width, height;

    screenmgr->ScreenForRect(*aX, *aY, logWidth, logHeight,
                             getter_AddRefs(screen));
    if (screen) {
      if (mSizeMode != nsSizeMode_Fullscreen) {
        
        screen->GetAvailRectDisplayPix(&left, &top, &width, &height);
      } else {
        
        screen->GetRectDisplayPix(&left, &top, &width, &height);
      }
      screenRect.left = left;
      screenRect.right = left + width;
      screenRect.top = top;
      screenRect.bottom = top + height;
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
        }
        ::ReleaseDC(mWnd, dc);
      }
    }
  }

  if (aAllowSlop) {
    if (*aX < screenRect.left - logWidth + kWindowPositionSlop)
      *aX = screenRect.left - logWidth + kWindowPositionSlop;
    else if (*aX >= screenRect.right - kWindowPositionSlop)
      *aX = screenRect.right - kWindowPositionSlop;

    if (*aY < screenRect.top - logHeight + kWindowPositionSlop)
      *aY = screenRect.top - logHeight + kWindowPositionSlop;
    else if (*aY >= screenRect.bottom - kWindowPositionSlop)
      *aY = screenRect.bottom - kWindowPositionSlop;

  } else {

    if (*aX < screenRect.left)
      *aX = screenRect.left;
    else if (*aX >= screenRect.right - logWidth)
      *aX = screenRect.right - logWidth;

    if (*aY < screenRect.top)
      *aY = screenRect.top;
    else if (*aY >= screenRect.bottom - logHeight)
      *aY = screenRect.bottom - logHeight;
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

NS_METHOD nsWindow::GetRestoredBounds(nsIntRect &aRect)
{
  if (SizeMode() == nsSizeMode_Normal) {
    return GetScreenBounds(aRect);
  }
  if (!mWnd) {
    return NS_ERROR_FAILURE;
  }

  WINDOWPLACEMENT pl = { sizeof(WINDOWPLACEMENT) };
  VERIFY(::GetWindowPlacement(mWnd, &pl));
  const RECT& r = pl.rcNormalPosition;

  HMONITOR monitor = ::MonitorFromWindow(mWnd, MONITOR_DEFAULTTONULL);
  if (!monitor) {
    return NS_ERROR_FAILURE;
  }
  MONITORINFO mi = { sizeof(MONITORINFO) };
  VERIFY(::GetMonitorInfo(monitor, &mi));

  aRect.SetRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
  aRect.MoveBy(mi.rcWork.left - mi.rcMonitor.left,
               mi.rcWork.top - mi.rcMonitor.top);
  return NS_OK;
}



nsIntPoint nsWindow::GetClientOffset()
{
  if (!mWnd) {
    return nsIntPoint(0, 0);
  }

  RECT r1;
  GetWindowRect(mWnd, &r1);
  LayoutDeviceIntPoint pt = WidgetToScreenOffset();
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




static const wchar_t kManageWindowInfoProperty[] = L"ManageWindowInfoProperty";
typedef BOOL (WINAPI *GetWindowInfoPtr)(HWND hwnd, PWINDOWINFO pwi);
static GetWindowInfoPtr sGetWindowInfoPtrStub = nullptr;

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
      appBarData.hWnd = FindWindow(L"Shell_TrayWnd", nullptr);
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
      mBorderStyle & eBorderStyle_none)
    return NS_ERROR_INVALID_ARG;

  if (mHideChrome) {
    mFutureMarginsOnceChromeShows = margins;
    mFutureMarginsToUse = true;
    return NS_OK;
  }
  mFutureMarginsToUse = false;

  
  if (margins.top == -1 && margins.left == -1 &&
      margins.right == -1 && margins.bottom == -1) {
    mCustomNonClient = false;
    mNonClientMargins = margins;
    
    
    ResetLayout();

    int windowStatus =
      reinterpret_cast<LONG_PTR>(GetPropW(mWnd, kManageWindowInfoProperty));
    if (windowStatus) {
      ::SendMessageW(mWnd, WM_NCACTIVATE, 1 != windowStatus, 0);
    }

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
  MapWindowPoints(nullptr, mWnd, (LPPOINT)&rect, 2);
  HRGN winRgn = CreateRectRgnIndirect(&rect);

  
  
  
  GetWindowRect(mWnd, &rect);
  rect.top += mCaptionHeight;
  rect.right -= mHorResizeMargin;
  rect.bottom -= mHorResizeMargin;
  rect.left += mVertResizeMargin;
  MapWindowPoints(nullptr, mWnd, (LPPOINT)&rect, 2);
  HRGN clientRgn = CreateRectRgnIndirect(&rect);
  CombineRgn(winRgn, winRgn, clientRgn, RGN_DIFF);
  DeleteObject(clientRgn);

  
  RedrawWindow(mWnd, nullptr, winRgn, RDW_FRAME | RDW_INVALIDATE);
  DeleteObject(winRgn);
}

HRGN
nsWindow::ExcludeNonClientFromPaintRegion(HRGN aRegion)
{
  RECT rect;
  HRGN rgn = nullptr;
  if (aRegion == (HRGN)1) { 
    GetWindowRect(mWnd, &rect);
    rgn = CreateRectRgnIndirect(&rect);
  } else {
    rgn = aRegion;
  }
  GetClientRect(mWnd, &rect);
  MapWindowPoints(mWnd, nullptr, (LPPOINT)&rect, 2);
  HRGN nonClientRgn = CreateRectRgnIndirect(&rect);
  CombineRgn(rgn, rgn, nonClientRgn, RGN_DIFF);
  DeleteObject(nonClientRgn);
  return rgn;
}









void nsWindow::SetBackgroundColor(const nscolor &aColor)
{
  if (mBrush)
    ::DeleteObject(mBrush);

  mBrush = ::CreateSolidBrush(NSRGB_2_COLOREF(aColor));
  if (mWnd != nullptr) {
    ::SetClassLongPtrW(mWnd, GCLP_HBRBACKGROUND, (LONG_PTR)mBrush);
  }
}










NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  

  
  
  
  HCURSOR newCursor = nullptr;

  switch (aCursor) {
    case eCursor_select:
      newCursor = ::LoadCursor(nullptr, IDC_IBEAM);
      break;

    case eCursor_wait:
      newCursor = ::LoadCursor(nullptr, IDC_WAIT);
      break;

    case eCursor_hyperlink:
    {
      newCursor = ::LoadCursor(nullptr, IDC_HAND);
      break;
    }

    case eCursor_standard:
    case eCursor_context_menu: 
      newCursor = ::LoadCursor(nullptr, IDC_ARROW);
      break;

    case eCursor_n_resize:
    case eCursor_s_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZENS);
      break;

    case eCursor_w_resize:
    case eCursor_e_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZEWE);
      break;

    case eCursor_nw_resize:
    case eCursor_se_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZENWSE);
      break;

    case eCursor_ne_resize:
    case eCursor_sw_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZENESW);
      break;

    case eCursor_crosshair:
      newCursor = ::LoadCursor(nullptr, IDC_CROSS);
      break;

    case eCursor_move:
      newCursor = ::LoadCursor(nullptr, IDC_SIZEALL);
      break;

    case eCursor_help:
      newCursor = ::LoadCursor(nullptr, IDC_HELP);
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
      newCursor = ::LoadCursor(nullptr, IDC_APPSTARTING);
      break;

    case eCursor_zoom_in:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ZOOMIN));
      break;

    case eCursor_zoom_out:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ZOOMOUT));
      break;

    case eCursor_not_allowed:
    case eCursor_no_drop:
      newCursor = ::LoadCursor(nullptr, IDC_NO);
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
      
      newCursor = ::LoadCursor(nullptr, IDC_SIZEALL);
      break;

    case eCursor_nesw_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZENESW);
      break;

    case eCursor_nwse_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZENWSE);
      break;

    case eCursor_ns_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZENS);
      break;

    case eCursor_ew_resize:
      newCursor = ::LoadCursor(nullptr, IDC_SIZEWE);
      break;

    case eCursor_none:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_NONE));
      break;

    default:
      NS_ERROR("Invalid cursor type");
      break;
  }

  if (nullptr != newCursor) {
    mCursor = aCursor;
    HCURSOR oldCursor = ::SetCursor(newCursor);
    
    if (sHCursor == oldCursor) {
      NS_IF_RELEASE(sCursorImgContainer);
      if (sHCursor != nullptr)
        ::DestroyIcon(sHCursor);
      sHCursor = nullptr;
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

  if (sHCursor != nullptr)
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

void nsWindow::UpdateOpaqueRegion(const nsIntRegion &aOpaqueRegion)
{
  if (!HasGlass() || GetParent())
    return;

  
  
  
  MARGINS margins = { -1, -1, -1, -1 };
  if (!aOpaqueRegion.IsEmpty()) {
    nsIntRect pluginBounds;
    for (nsIWidget* child = GetFirstChild(); child; child = child->GetNextSibling()) {
      if (child->IsPlugin()) {
        
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
  default:
    break;
  }

  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
         ("glass margins: left:%d top:%d right:%d bottom:%d\n",
          margins.cxLeftWidth, margins.cyTopHeight,
          margins.cxRightWidth, margins.cyBottomHeight));

  
  if (nsUXThemeData::CheckForCompositor()) {
    WinUtils::dwmExtendFrameIntoClientAreaPtr(mWnd, &margins);
    WinUtils::dwmSetWindowAttributePtr(mWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof policy);
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
    if (mFutureMarginsToUse) {
      SetNonClientMargins(mFutureMarginsOnceChromeShows);
    }
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

  VERIFY(::RedrawWindow(mWnd, nullptr, nullptr, flags));
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
nsWindow::MakeFullScreen(bool aFullScreen, nsIScreen* aTargetScreen)
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
    if (mSizeMode != nsSizeMode_Fullscreen)
      return NS_OK;
    SetSizeMode(mOldSizeMode);
  }

  UpdateNonClientMargins();

  bool visible = mIsVisible;
  if (mOldSizeMode == nsSizeMode_Normal)
    Show(false);
  
  
  
  
  nsresult rv = nsBaseWidget::MakeFullScreen(aFullScreen, aTargetScreen);

  if (visible) {
    Show(true);
    Invalidate();

    if (!aFullScreen && mOldSizeMode == nsSizeMode_Normal) {
      
      
      
      DispatchFocusToTopLevelWindow(true);
    }
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
                                      nullptr,
                                      nsToolkit::mDllInstance,
                                      nullptr);
    case NS_NATIVE_PLUGIN_ID:
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
      return (void*)mWnd;
    case NS_NATIVE_SHAREABLE_WINDOW:
      return (void*) WinUtils::GetTopLevelHWND(mWnd);
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

  return nullptr;
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

  HICON bigIcon = (HICON)::LoadImageW(nullptr,
                                      (LPCWSTR)iconPath.get(),
                                      IMAGE_ICON,
                                      ::GetSystemMetrics(SM_CXICON),
                                      ::GetSystemMetrics(SM_CYICON),
                                      LR_LOADFROMFILE );
  HICON smallIcon = (HICON)::LoadImageW(nullptr,
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









LayoutDeviceIntPoint nsWindow::WidgetToScreenOffset()
{
  POINT point;
  point.x = 0;
  point.y = 0;
  ::ClientToScreen(mWnd, &point);
  return LayoutDeviceIntPoint(point.x, point.y);
}

LayoutDeviceIntSize
nsWindow::ClientToWindowSize(const LayoutDeviceIntSize& aClientSize)
{
  if (mWindowType == eWindowType_popup && !IsPopupWithTitleBar())
    return aClientSize;

  
  RECT r;
  r.left = 200;
  r.top = 200;
  r.right = 200 + aClientSize.width;
  r.bottom = 200 + aClientSize.height;
  ::AdjustWindowRectEx(&r, WindowStyle(), false, WindowExStyle());

  return LayoutDeviceIntSize(r.right - r.left, r.bottom - r.top);
}









NS_METHOD nsWindow::EnableDragDrop(bool aEnable)
{
  NS_ASSERTION(mWnd, "nsWindow::EnableDragDrop() called after Destroy()");

  nsresult rv = NS_ERROR_FAILURE;
  if (aEnable) {
    if (nullptr == mNativeDragTarget) {
       mNativeDragTarget = new nsNativeDragTarget(this);
       if (nullptr != mNativeDragTarget) {
         mNativeDragTarget->AddRef();
         if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget,TRUE,FALSE)) {
           if (S_OK == ::RegisterDragDrop(mWnd, (LPDROPTARGET)mNativeDragTarget)) {
             rv = NS_OK;
           }
         }
       }
    }
  } else {
    if (nullptr != mWnd && nullptr != mNativeDragTarget) {
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
    nsToolkit::gMouseTrailer->SetCaptureWindow(nullptr);
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

LayerManager*
nsWindow::GetLayerManager(PLayerTransactionChild* aShadowManager,
                          LayersBackend aBackendHint,
                          LayerManagerPersistence aPersistence,
                          bool* aAllowRetaining)
{
  if (aAllowRetaining) {
    *aAllowRetaining = true;
  }

  RECT windowRect;
  ::GetClientRect(mWnd, &windowRect);

  
  if (!mLayerManager && ShouldUseOffMainThreadCompositing()) {
    gfxWindowsPlatform::GetPlatform()->UpdateRenderMode();

    
    
    NS_ASSERTION(aShadowManager == nullptr, "Async Compositor not supported with e10s");
    CreateCompositor();
  }

  if (!mLayerManager) {
    MOZ_ASSERT(!mCompositorParent && !mCompositorChild);
    mLayerManager = CreateBasicLayerManager();
  }

  NS_ASSERTION(mLayerManager, "Couldn't provide a valid layer manager.");

  return mLayerManager;
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

  
  
  if (IsVistaOrLater()) {
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

mozilla::TemporaryRef<mozilla::gfx::DrawTarget>
nsWindow::StartRemoteDrawing()
{
  MOZ_ASSERT(!mCompositeDC);
  NS_ASSERTION(IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_GDI),
               "Unexpected render mode for remote drawing");

  HDC dc = (HDC)GetNativeData(NS_NATIVE_GRAPHIC);
  nsRefPtr<gfxASurface> surf;

  if (mTransparencyMode == eTransparencyTransparent) {
    if (!mTransparentSurface) {
      SetupTranslucentWindowMemoryBitmap(mTransparencyMode);
    }
    if (mTransparentSurface) {
      surf = mTransparentSurface;
    }
  } 
  
  if (!surf) {
    if (!dc) {
      return nullptr;
    }
    uint32_t flags = (mTransparencyMode == eTransparencyOpaque) ? 0 :
        gfxWindowsSurface::FLAG_IS_TRANSPARENT;
    surf = new gfxWindowsSurface(dc, flags);
  }

  mozilla::gfx::IntSize size(surf->GetSize().width, surf->GetSize().height);
  if (size.width <= 0 || size.height <= 0) {
    if (dc) {
      FreeNativeData(dc, NS_NATIVE_GRAPHIC);
    }
    return nullptr;
  }

  MOZ_ASSERT(!mCompositeDC);
  mCompositeDC = dc;

  return mozilla::gfx::Factory::CreateDrawTargetForCairoSurface(surf->CairoSurface(), size);
}

void
nsWindow::EndRemoteDrawing()
{
  if (mTransparencyMode == eTransparencyTransparent) {
    MOZ_ASSERT(IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)
               || mTransparentSurface);
    UpdateTranslucentWindow();
  }
  if (mCompositeDC) {
    FreeNativeData(mCompositeDC, NS_NATIVE_GRAPHIC);
  }
  mCompositeDC = nullptr;
}

void
nsWindow::UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries)
{
  nsIntRegion clearRegion;
  for (size_t i = 0; i < aThemeGeometries.Length(); i++) {
    if (aThemeGeometries[i].mType == nsNativeThemeWin::eThemeGeometryTypeWindowButtons &&
        nsUXThemeData::CheckForCompositor())
    {
      nsIntRect bounds = aThemeGeometries[i].mRect;
      clearRegion = nsIntRect(bounds.X(), bounds.Y(), bounds.Width(), bounds.Height() - 2.0);
      clearRegion.Or(clearRegion, nsIntRect(bounds.X() + 1.0, bounds.YMost() - 2.0, bounds.Width() - 1.0, 1.0));
      clearRegion.Or(clearRegion, nsIntRect(bounds.X() + 2.0, bounds.YMost() - 1.0, bounds.Width() - 3.0, 1.0));
    }
  }

  nsRefPtr<LayerManager> layerManager = GetLayerManager();
  if (layerManager) {
    layerManager->SetRegionToClear(clearRegion);
  }
}

uint32_t
nsWindow::GetMaxTouchPoints() const
{
  if (IsWin7OrLater() && IsTouchDeviceSupportPresent()) {
    return GetSystemMetrics(SM_MAXIMUMTOUCHES);
  }
  return 0;
}




















void nsWindow::InitEvent(WidgetGUIEvent& event, nsIntPoint* aPoint)
{
  if (nullptr == aPoint) {     
    
    if (mWnd != nullptr) {

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
  event.timeStamp = GetMessageTimeStamp(event.time);
}











NS_IMETHODIMP nsWindow::DispatchEvent(WidgetGUIEvent* event,
                                      nsEventStatus& aStatus)
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
  WidgetGUIEvent event(true, aMsg, this);
  InitEvent(event);

  bool result = DispatchWindowEvent(&event);
  return result;
}

bool nsWindow::DispatchKeyboardEvent(WidgetKeyboardEvent* event)
{
  nsEventStatus status = DispatchInputEvent(event);
  return ConvertStatus(status);
}

bool nsWindow::DispatchContentCommandEvent(WidgetContentCommandEvent* aEvent)
{
  nsEventStatus status;
  DispatchEvent(aEvent, status);
  return ConvertStatus(status);
}

bool nsWindow::DispatchWheelEvent(WidgetWheelEvent* aEvent)
{
  nsEventStatus status = DispatchAPZAwareEvent(aEvent->AsInputEvent());
  return ConvertStatus(status);
}

bool nsWindow::DispatchWindowEvent(WidgetGUIEvent* event)
{
  nsEventStatus status;
  DispatchEvent(event, status);
  return ConvertStatus(status);
}

bool nsWindow::DispatchWindowEvent(WidgetGUIEvent* event,
                                   nsEventStatus& aStatus)
{
  DispatchEvent(event, aStatus);
  return ConvertStatus(aStatus);
}



BOOL CALLBACK nsWindow::DispatchStarvedPaints(HWND aWnd, LPARAM aMsg)
{
  LONG_PTR proc = ::GetWindowLongPtrW(aWnd, GWLP_WNDPROC);
  if (proc == (LONG_PTR)&nsWindow::WindowProc) {
    
    
    
    if (GetUpdateRect(aWnd, nullptr, FALSE))
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

bool nsWindow::DispatchPluginEvent(UINT aMessage,
                                     WPARAM aWParam,
                                     LPARAM aLParam,
                                     bool aDispatchPendingEvents)
{
  bool ret = nsWindowBase::DispatchPluginEvent(
               WinUtils::InitMSG(aMessage, aWParam, aLParam, mWnd));
  if (aDispatchPendingEvents && !Destroyed()) {
    DispatchPendingEvents();
  }
  return ret;
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

  if (WinUtils::GetIsMouseFromTouch(aEventType) && mTouchWindow) {
    
    
    
    MOZ_ASSERT(mAPZC);
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

  WidgetMouseEvent event(true, aEventType, this, WidgetMouseEvent::eReal,
                         aIsContextMenuKey ? WidgetMouseEvent::eContextMenuKey :
                                             WidgetMouseEvent::eNormal);
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
  
  
  event.convertToPointer = true;

  nsIntPoint mpScreen = eventPoint + WidgetToScreenOffsetUntyped();

  
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
    case WidgetMouseEvent::eLeftButton:
      eventButton = VK_LBUTTON;
      break;
    case WidgetMouseEvent::eMiddleButton:
      eventButton = VK_MBUTTON;
      break;
    case WidgetMouseEvent::eRightButton:
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
    sLastMouseDownTime = curMsgTime;
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
    event.exit = IsTopLevelMouseExit(mWnd) ?
                   WidgetMouseEvent::eTopLevel : WidgetMouseEvent::eChild;
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
        case WidgetMouseEvent::eLeftButton:
          pluginEvent.event = WM_LBUTTONDOWN;
          break;
        case WidgetMouseEvent::eMiddleButton:
          pluginEvent.event = WM_MBUTTONDOWN;
          break;
        case WidgetMouseEvent::eRightButton:
          pluginEvent.event = WM_RBUTTONDOWN;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_BUTTON_UP:
      switch (aButton) {
        case WidgetMouseEvent::eLeftButton:
          pluginEvent.event = WM_LBUTTONUP;
          break;
        case WidgetMouseEvent::eMiddleButton:
          pluginEvent.event = WM_MBUTTONUP;
          break;
        case WidgetMouseEvent::eRightButton:
          pluginEvent.event = WM_RBUTTONUP;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_DOUBLECLICK:
      switch (aButton) {
        case WidgetMouseEvent::eLeftButton:
          pluginEvent.event = WM_LBUTTONDBLCLK;
          break;
        case WidgetMouseEvent::eMiddleButton:
          pluginEvent.event = WM_MBUTTONDBLCLK;
          break;
        case WidgetMouseEvent::eRightButton:
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

  event.mPluginEvent.Copy(pluginEvent);

  
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

      if (rect.Contains(LayoutDeviceIntPoint::ToUntyped(event.refPoint))) {
        if (sCurrentWindow == nullptr || sCurrentWindow != this) {
          if ((nullptr != sCurrentWindow) && (!sCurrentWindow->mInDtor)) {
            LPARAM pos = sCurrentWindow->lParamToClient(lParamToScreen(lParam));
            sCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT, wParam, pos, false, 
                                               WidgetMouseEvent::eLeftButton,
                                               aInputSource);
          }
          sCurrentWindow = this;
          if (!mInDtor) {
            LPARAM pos = sCurrentWindow->lParamToClient(lParamToScreen(lParam));
            sCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER, wParam, pos, false,
                                               WidgetMouseEvent::eLeftButton,
                                               aInputSource);
          }
        }
      }
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (sCurrentWindow == this) {
        sCurrentWindow = nullptr;
      }
    }

    result = ConvertStatus(DispatchInputEvent(&event));

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
  HWND toplevelWnd = nullptr;
  while (curWnd) {
    toplevelWnd = curWnd;

    nsWindow *win = WinUtils::GetNSWindowPtr(curWnd);
    if (win) {
      nsWindowType wintype = win->WindowType();
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
  MOZ_ASSERT_IF(msg != WM_GETOBJECT,
                !mozilla::ipc::MessageChannel::IsPumpingMessages());

  
  if (mozilla::ipc::MessageChannel::IsSpinLoopActive() &&
      (InSendMessageEx(nullptr) & (ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
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
        
        
        if ((InSendMessageEx(nullptr) & (ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
          wchar_t szClass[10];
          HWND focusWnd = (HWND)lParam;
          if (IsWindowVisible(focusWnd) &&
              GetClassNameW(focusWnd, szClass,
                            sizeof(szClass)/sizeof(char16_t)) &&
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
      (InSendMessageEx(nullptr) & (ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
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
                     x, y, 0, hWnd, nullptr);
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
  MOZ_RELEASE_ASSERT(!ipc::ParentProcessIsBlocked());

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

  
  
  nsCOMPtr<nsIWidget> kungFuDeathGrip;
  if (!targetWindow->mInDtor)
    kungFuDeathGrip = targetWindow;

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
                                  MSGResult& aResult)
{
  aResult.mResult = 0;
  aResult.mConsumed = true;

  bool eventDispatched = false;
  switch (aMsg.message) {
    case WM_CHAR:
    case WM_SYSCHAR:
      aResult.mResult = ProcessCharMessage(aMsg, &eventDispatched);
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      aResult.mResult = ProcessKeyUpMessage(aMsg, &eventDispatched);
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      aResult.mResult = ProcessKeyDownMessage(aMsg, &eventDispatched);
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

  if (!eventDispatched) {
    aResult.mConsumed = nsWindowBase::DispatchPluginEvent(aMsg);
  }
  if (!Destroyed()) {
    DispatchPendingEvents();
  }
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

bool
nsWindow::ExternalHandlerProcessMessage(UINT aMessage,
                                        WPARAM& aWParam,
                                        LPARAM& aLParam,
                                        MSGResult& aResult)
{
  if (mWindowHook.Notify(mWnd, aMessage, aWParam, aLParam, aResult)) {
    return true;
  }

  if (IMEHandler::ProcessMessage(this, aMessage, aWParam, aLParam, aResult)) {
    return true;
  }

  if (MouseScrollHandler::ProcessMessage(this, aMessage, aWParam, aLParam,
                                         aResult)) {
    return true;
  }

  if (PluginHasFocus()) {
    MSG nativeMsg = WinUtils::InitMSG(aMessage, aWParam, aLParam, mWnd);
    if (ProcessMessageForPlugin(nativeMsg, aResult)) {
      return true;
    }
  }

  return false;
}


bool
nsWindow::ProcessMessage(UINT msg, WPARAM& wParam, LPARAM& lParam,
                         LRESULT *aRetValue)
{
#if defined(EVENT_DEBUG_OUTPUT)
  
  
  PrintEvent(msg, SHOW_REPEAT_EVENTS, SHOW_MOUSEMOVE_EVENTS);
#endif

  MSGResult msgResult(aRetValue);
  if (ExternalHandlerProcessMessage(msg, wParam, lParam, msgResult)) {
    return (msgResult.mConsumed || !mWnd);
  }

  bool result = false;    
  *aRetValue = 0;

  
  LRESULT dwmHitResult;
  if (mCustomNonClient &&
      nsUXThemeData::CheckForCompositor() &&
      WinUtils::dwmDwmDefWindowProcPtr(mWnd, msg, wParam, lParam, &dwmHitResult)) {
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

    case WM_WTSSESSION_CHANGE:
    {
      switch (wParam) {
        case WTS_CONSOLE_CONNECT:
        case WTS_REMOTE_CONNECT:
        case WTS_SESSION_UNLOCK:
          
          Invalidate(true, true, true);
          break;
        default:
          break;
      }
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
      if (mMouseTransparent) {
        
        *aRetValue = HTTRANSPARENT;
        result = true;
        break;
      }

      







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
      



      UpdateGetWindowInfoCaptionStatus(FALSE != wParam);

      if (!mCustomNonClient)
        break;

      
      
      if(mSizeMode != nsSizeMode_Fullscreen &&
         nsUXThemeData::CheckForCompositor())
        break;

      if (wParam == TRUE) {
        
        *aRetValue = FALSE; 
        result = true;
        
        InvalidateNonClientRegion();
        break;
      } else {
        
        *aRetValue = TRUE; 
        result = true;
        
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
      *aRetValue = (int) OnPaint(nullptr, 0);
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
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam, mWnd);
      result = ProcessCharMessage(nativeMsg, nullptr);
      DispatchPendingEvents();
    }
    break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam, mWnd);
      nativeMsg.time = ::GetMessageTime();
      result = ProcessKeyUpMessage(nativeMsg, nullptr);
      DispatchPendingEvents();
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam, mWnd);
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
                                  false, WidgetMouseEvent::eLeftButton,
                                  MOUSE_INPUT_SOURCE());
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
                                  false, WidgetMouseEvent::eLeftButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
    }
    break;

    case WM_LBUTTONUP:
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam,
                                  false, WidgetMouseEvent::eLeftButton,
                                  MOUSE_INPUT_SOURCE());
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
                         WidgetMouseEvent::eLeftButton, MOUSE_INPUT_SOURCE());
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
                                    WidgetMouseEvent::eLeftButton :
                                    WidgetMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      if (lParam != -1 && !result && mCustomNonClient) {
        WidgetMouseEvent event(true, NS_MOUSE_MOZHITTEST, this,
                               WidgetMouseEvent::eReal,
                               WidgetMouseEvent::eNormal);
        event.refPoint = LayoutDeviceIntPoint(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
        event.inputSource = MOUSE_INPUT_SOURCE();
        event.mFlags.mOnlyChromeDispatch = true;
        if (DispatchWindowEvent(&event)) {
          
          DisplaySystemMenu(mWnd, mSizeMode, mIsRTL, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
          result = true;
        }
      }
    }
    break;

    case WM_LBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eLeftButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_MBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eMiddleButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_MBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eMiddleButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_MBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eMiddleButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCMBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, 0,
                                  lParamToClient(lParam), false,
                                  WidgetMouseEvent::eMiddleButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCMBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, 0,
                                  lParamToClient(lParam), false,
                                  WidgetMouseEvent::eMiddleButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCMBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, 0,
                                  lParamToClient(lParam), false,
                                  WidgetMouseEvent::eMiddleButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_RBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_RBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_RBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, wParam,
                                  lParam, false,
                                  WidgetMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCRBUTTONDOWN:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, 0,
                                  lParamToClient(lParam), false,
                                  WidgetMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCRBUTTONUP:
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, 0,
                                  lParamToClient(lParam), false,
                                  WidgetMouseEvent::eRightButton,
                                  MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_NCRBUTTONDBLCLK:
      result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, 0,
                                  lParamToClient(lParam), false,
                                  WidgetMouseEvent::eRightButton,
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
                         false, WidgetMouseEvent::eLeftButton,
                         MOUSE_INPUT_SOURCE());
      result = 
        DispatchMouseEvent(NS_MOUSE_BUTTON_UP, 0, lParamToClient(lParam),
                           false, WidgetMouseEvent::eLeftButton,
                           MOUSE_INPUT_SOURCE());
      DispatchPendingEvents();
      break;

    case WM_APPCOMMAND:
    {
      MSG nativeMsg = WinUtils::InitMSG(msg, wParam, lParam, mWnd);
      result = HandleAppCommandMsg(nativeMsg, aRetValue);
      break;
    }

    
    
    
    
    
    
    case WM_ACTIVATE:
      if (mWidgetListener) {
        int32_t fActive = LOWORD(wParam);

        if (WA_INACTIVE == fActive) {
          
          
          if (HIWORD(wParam))
            DispatchFocusToTopLevelWindow(false);
          else
            sJustGotDeactivate = true;

          if (mIsTopWidgetWindow)
            mLastKeyboardLayout = KeyboardLayout::GetInstance()->GetLayout();

        } else {
          StopFlashing();

          sJustGotActivate = true;
          WidgetMouseEvent event(true, NS_MOUSE_ACTIVATE, this,
                                 WidgetMouseEvent::eReal);
          InitEvent(event);
          ModifierKeyState modifierKeyState;
          modifierKeyState.InitInputEvent(event);
          DispatchInputEvent(&event);
          if (sSwitchKeyboardLayout && mLastKeyboardLayout)
            ActivateKeyboardLayout(mLastKeyboardLayout, 0);
        }
      }
      break;

    case WM_MOUSEACTIVATE:
      
      
      
      
      if (IsPopup() && IsOwnerForegroundWindow()) {
        *aRetValue = MA_NOACTIVATE;
        result = true;
      }
      break;

    case WM_WINDOWPOSCHANGING:
    {
      LPWINDOWPOS info = (LPWINDOWPOS)lParam;
      OnWindowPosChanging(info);
      result = true;
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
        RedirectedKeyDownMessageManager::Forget();
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
      WINDOWPOS* wp = (LPWINDOWPOS)lParam;
      OnWindowPosChanged(wp);
      result = true;
    }
    break;

    case WM_INPUTLANGCHANGEREQUEST:
      *aRetValue = TRUE;
      result = false;
      break;

    case WM_INPUTLANGCHANGE:
      KeyboardLayout::GetInstance()->
        OnLayoutChange(reinterpret_cast<HKL>(lParam));
      result = false; 
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
      
      
      int32_t objId = static_cast<DWORD>(lParam);
      if (objId == OBJID_CLIENT) { 
        a11y::Accessible* rootAccessible = GetAccessible(); 
        if (rootAccessible) {
          IAccessible *msaaAccessible = nullptr;
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
	  
      if (mSizeMode == nsSizeMode_Fullscreen && filteredWParam == SC_RESTORE) {
        MakeFullScreen(false);
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
          !IsPlugin()) {
        
        
        
        if (gIsPointerEventsEnabled) {
          result = false;
          break;
        }

        GESTURENOTIFYSTRUCT * gestureinfo = (GESTURENOTIFYSTRUCT*)lParam;
        nsPointWin touchPoint;
        touchPoint = gestureinfo->ptsLocation;
        touchPoint.ScreenToClient(mWnd);
        WidgetGestureNotifyEvent gestureNotifyEvent(true,
                                   NS_GESTURENOTIFY_EVENT_START, this);
        gestureNotifyEvent.refPoint = LayoutDeviceIntPoint::FromUntyped(touchPoint);
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
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_DELETE, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case WM_CUT:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_CUT, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case WM_COPY:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_COPY, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case WM_PASTE:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_PASTE, this);
      DispatchWindowEvent(&command);
      result = true;
    }
    break;

    case EM_UNDO:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_UNDO, this);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case EM_REDO:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_REDO, this);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case EM_CANPASTE:
    {
      
      
      if (wParam == 0 || wParam == CF_TEXT || wParam == CF_UNICODETEXT) {
        WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_PASTE,
                                          this, true);
        DispatchWindowEvent(&command);
        *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
        result = true;
      }
    }
    break;

    case EM_CANUNDO:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_UNDO,
                                        this, true);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case EM_CANREDO:
    {
      WidgetContentCommandEvent command(true, NS_CONTENT_COMMAND_REDO,
                                        this, true);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = true;
    }
    break;

    case WM_SETTINGCHANGE:
      if (IsWin8OrLater() && lParam &&
          !wcsicmp(L"ConvertibleSlateMode", (wchar_t*)lParam)) {
        
        
        if (GetSystemMetrics(SM_CONVERTIBLESLATEMODE) == 0 &&
            Preferences::GetBool("browser.shell.desktop-auto-switch-enabled",
                                 false)) {
          nsCOMPtr<nsIAppStartup> appStartup(do_GetService(NS_APPSTARTUP_CONTRACTID));
          if (appStartup) {
            appStartup->Quit(nsIAppStartup::eForceQuit |
                             nsIAppStartup::eRestartTouchEnvironment);
          }
        }
      }
    break;

    default:
    {
      if (msg == nsAppShell::GetTaskbarButtonCreatedMessage()) {
        SetHasTaskbarIconBeenCreated();
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
    POINT pt = { mx, my };
    ::ScreenToClient(mWnd, &pt);
    if (pt.x == mCachedHitTestPoint.x && pt.y == mCachedHitTestPoint.y &&
        TimeStamp::Now() - mCachedHitTestTime < TimeDuration::FromMilliseconds(HITTEST_CACHE_LIFETIME_MS)) {
      testResult = mCachedHitTestResult;
    } else {
      WidgetMouseEvent event(true, NS_MOUSE_MOZHITTEST, this,
                             WidgetMouseEvent::eReal,
                             WidgetMouseEvent::eNormal);
      event.refPoint = LayoutDeviceIntPoint(pt.x, pt.y);
      event.inputSource = MOUSE_INPUT_SOURCE();
      event.mFlags.mOnlyChromeDispatch = true;
      bool result = ConvertStatus(DispatchInputEvent(&event));
      if (result) {
        
        testResult = testResult == HTCLIENT ? HTCAPTION : testResult;

      } else {
        
        
        testResult = HTCLIENT;
      }
      mCachedHitTestPoint = pt;
      mCachedHitTestTime = TimeStamp::Now();
      mCachedHitTestResult = testResult;
    }
  }

  return testResult;
}

TimeStamp
nsWindow::GetMessageTimeStamp(LONG aEventTime)
{
  
  
  
  

  
  
  
  
  
  
  DWORD eventTime = static_cast<DWORD>(aEventTime);

  
  if (sFirstEventTimeStamp.IsNull()) {
    nsWindow::UpdateFirstEventTime(eventTime);
  }
  TimeStamp roughlyNow = TimeStamp::NowLoRes();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (eventTime < sFirstEventTime &&
      sFirstEventTime - eventTime < kEventTimeHalfRange &&
      roughlyNow - sFirstEventTimeStamp <
        TimeDuration::FromMilliseconds(kEventTimeHalfRange)) {
    UpdateFirstEventTime(eventTime);
  }

  double timeSinceFirstEvent =
    sFirstEventTime <= eventTime
      ? eventTime - sFirstEventTime
      : static_cast<double>(kEventTimeRange) + eventTime - sFirstEventTime;
  TimeStamp eventTimeStamp =
    sFirstEventTimeStamp + TimeDuration::FromMilliseconds(timeSinceFirstEvent);

  
  
  
  double timesWrapped =
    (roughlyNow - sFirstEventTimeStamp).ToMilliseconds() / kEventTimeRange;
  int32_t cyclesToAdd = static_cast<int32_t>(timesWrapped); 

  
  
  
  
  
  
  
  double intervalFraction = fmod(timesWrapped, 1.0);

  
  
  
  
  if (intervalFraction < 0.1 && timeSinceFirstEvent > kEventTimeRange * 0.9) {
    cyclesToAdd--;
  
  
  } else if (intervalFraction > 0.9 &&
             timeSinceFirstEvent < kEventTimeRange * 0.1) {
    cyclesToAdd++;
  }

  if (cyclesToAdd > 0) {
    eventTimeStamp +=
      TimeDuration::FromMilliseconds(kEventTimeRange * cyclesToAdd);
  }

  return eventTimeStamp;
}

void
nsWindow::UpdateFirstEventTime(DWORD aEventTime)
{
  sFirstEventTime = aEventTime;
  DWORD currentTime = ::GetTickCount();
  TimeStamp currentTimeStamp = TimeStamp::Now();
  double timeSinceFirstEvent =
    aEventTime <= currentTime
      ? currentTime - aEventTime
      : static_cast<double>(kEventTimeRange) + currentTime - aEventTime;
  sFirstEventTimeStamp =
    currentTimeStamp - TimeDuration::FromMilliseconds(timeSinceFirstEvent);
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

LRESULT nsWindow::ProcessCharMessage(const MSG &aMsg, bool *aEventDispatched)
{
  if (IMEHandler::IsComposingOn(this)) {
    IMEHandler::NotifyIME(this, REQUEST_TO_COMMIT_COMPOSITION);
  }
  
  
  
  ModifierKeyState modKeyState;
  NativeKey nativeKey(this, aMsg, modKeyState);
  return static_cast<LRESULT>(nativeKey.HandleCharMessage(aMsg,
                                                          aEventDispatched));
}

LRESULT nsWindow::ProcessKeyUpMessage(const MSG &aMsg, bool *aEventDispatched)
{
  if (IMEHandler::IsComposingOn(this)) {
    return 0;
  }

  ModifierKeyState modKeyState;
  NativeKey nativeKey(this, aMsg, modKeyState);
  return static_cast<LRESULT>(nativeKey.HandleKeyUpMessage(aEventDispatched));
}

LRESULT nsWindow::ProcessKeyDownMessage(const MSG &aMsg,
                                        bool *aEventDispatched)
{
  
  
  
  
  
  RedirectedKeyDownMessageManager::AutoFlusher redirectedMsgFlusher(this, aMsg);

  ModifierKeyState modKeyState;

  LRESULT result = 0;
  if (!IMEHandler::IsComposingOn(this)) {
    NativeKey nativeKey(this, aMsg, modKeyState);
    result =
      static_cast<LRESULT>(nativeKey.HandleKeyDownMessage(aEventDispatched));
    
    
    redirectedMsgFlusher.Cancel();
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
  KeyboardLayout* keyboardLayout = KeyboardLayout::GetInstance();
  return keyboardLayout->SynthesizeNativeKeyEvent(
           this, aNativeKeyboardLayout, aNativeKeyCode, aModifierFlags,
           aCharacters, aUnmodifiedCharacters);
}

nsresult
nsWindow::SynthesizeNativeMouseEvent(LayoutDeviceIntPoint aPoint,
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
nsWindow::SynthesizeNativeMouseScrollEvent(LayoutDeviceIntPoint aPoint,
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










void nsWindow::OnWindowPosChanged(WINDOWPOS* wp)
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
                 ("*** mSizeMode: nsSizeMode_Maximized\n"));
        break;
      default:
          PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ("*** mSizeMode: ??????\n"));
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

  
  if (!(wp->flags & SWP_NOMOVE)) {
    mBounds.x = wp->x;
    mBounds.y = wp->y;

    NotifyWindowMoved(wp->x, wp->y);
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

      ::RedrawWindow(mWnd, &drect, nullptr,
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

      ::RedrawWindow(mWnd, &drect, nullptr,
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
        
        return;
      }
    }

    
    if (::GetClientRect(mWnd, &r)) {
      rect.width  = r.right - r.left;
      rect.height = r.bottom - r.top;
    }
    
    
    OnResize(rect);
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
    MultiTouchInput touchInput, touchEndInput;

    
    for (uint32_t i = 0; i < cInputs; i++) {
      bool addToEvent = false, addToEndEvent = false;

      
      
      
      

      if (pInputs[i].dwFlags & (TOUCHEVENTF_DOWN | TOUCHEVENTF_MOVE)) {
        if (touchInput.mTimeStamp.IsNull()) {
          
          touchInput.mType = MultiTouchInput::MULTITOUCH_MOVE;
          touchInput.mTime = ::GetMessageTime();
          touchInput.mTimeStamp = GetMessageTimeStamp(touchInput.mTime);
          ModifierKeyState modifierKeyState;
          touchInput.modifiers = modifierKeyState.GetModifiers();
        }
        
        
        if (pInputs[i].dwFlags & TOUCHEVENTF_DOWN) {
          touchInput.mType = MultiTouchInput::MULTITOUCH_START;
        }
        addToEvent = true;
      }
      if (pInputs[i].dwFlags & TOUCHEVENTF_UP) {
        
        
        if (touchEndInput.mTimeStamp.IsNull()) {
          
          touchEndInput.mType = MultiTouchInput::MULTITOUCH_END;
          touchEndInput.mTime = ::GetMessageTime();
          touchEndInput.mTimeStamp = GetMessageTimeStamp(touchEndInput.mTime);
          ModifierKeyState modifierKeyState;
          touchEndInput.modifiers = modifierKeyState.GetModifiers();
        }
        addToEndEvent = true;
      }
      if (!addToEvent && !addToEndEvent) {
        
        continue;
      }

      
      nsPointWin touchPoint;
      touchPoint.x = TOUCH_COORD_TO_PIXEL(pInputs[i].x);
      touchPoint.y = TOUCH_COORD_TO_PIXEL(pInputs[i].y);
      touchPoint.ScreenToClient(mWnd);

      
      SingleTouchData touchData(pInputs[i].dwID,                                      
                                ScreenIntPoint::FromUntyped(touchPoint),              
                                
                                pInputs[i].dwFlags & TOUCHINPUTMASKF_CONTACTAREA
                                  ? ScreenSize(
                                      TOUCH_COORD_TO_PIXEL(pInputs[i].cxContact) / 2,
                                      TOUCH_COORD_TO_PIXEL(pInputs[i].cyContact) / 2)
                                  : ScreenSize(1, 1),                                 
                                0.0f,                                                 
                                0.0f);                                                

      
      if (addToEvent) {
        touchInput.mTouches.AppendElement(touchData);
      }
      if (addToEndEvent) {
        touchEndInput.mTouches.AppendElement(touchData);
      }
    }

    
    if (!touchInput.mTimeStamp.IsNull()) {
      
      WidgetTouchEvent widgetTouchEvent = touchInput.ToWidgetTouchEvent(this);
      DispatchAPZAwareEvent(&widgetTouchEvent);
    }
    
    if (!touchEndInput.mTimeStamp.IsNull()) {
      
      WidgetTouchEvent widgetTouchEvent = touchEndInput.ToWidgetTouchEvent(this);
      DispatchAPZAwareEvent(&widgetTouchEvent);
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
  if (gIsPointerEventsEnabled) {
    return false;
  }

  
  if (mGesture.IsPanEvent(lParam)) {
    if ( !mGesture.ProcessPanMessage(mWnd, wParam, lParam) )
      return false; 

    nsEventStatus status;

    WidgetWheelEvent wheelEvent(true, NS_WHEEL_WHEEL, this);

    ModifierKeyState modifierKeyState;
    modifierKeyState.InitInputEvent(wheelEvent);

    wheelEvent.button      = 0;
    wheelEvent.time        = ::GetMessageTime();
    wheelEvent.timeStamp   = GetMessageTimeStamp(wheelEvent.time);
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

  
  WidgetSimpleGestureEvent event(true, 0, this);
  if ( !mGesture.ProcessGestureMessage(mWnd, wParam, lParam, event) ) {
    return false; 
  }
  
  
  ModifierKeyState modifierKeyState;
  modifierKeyState.InitInputEvent(event);
  event.button    = 0;
  event.time      = ::GetMessageTime();
  event.timeStamp = GetMessageTimeStamp(event.time);
  event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

  nsEventStatus status;
  DispatchEvent(&event, status);
  if (status == nsEventStatus_eIgnore) {
    return false; 
  }

  
  mGesture.CloseGestureInfoHandle((HGESTUREINFO)lParam);

  return true; 
}

nsresult
nsWindow::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
  
  
  
  if (mWindowType == eWindowType_plugin_ipc_chrome) {
    return NS_OK;
  }

  
  
  
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
          GetLayerManager()->GetBackendType() != LayersBackend::LAYERS_BASIC) {
        
        
        
        nsIntRegion r;
        r.Sub(bounds, configuration.mBounds);
        r.MoveBy(-bounds.x,
                 -bounds.y);
        nsIntRect toInvalidate = r.GetBounds();

        WinUtils::InvalidatePluginAsWorkaround(w, toInvalidate);
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
  buf.SetLength(size);
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
  return ::ExtCreateRegion(nullptr, buf.Length(), data);
}

nsresult
nsWindow::SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                              bool aIntersectWithExisting)
{
  if (IsWindowClipRegionEqual(aRects)) {
    return NS_OK;
  }

  nsBaseWidget::SetWindowClipRegion(aRects, aIntersectWithExisting);

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

  
  
  
  
  if (IsPlugin()) {
    if (NULLREGION == ::CombineRgn(dest, dest, dest, RGN_OR)) {
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

  
  ::WTSUnRegisterSessionNotification(mWnd);

  
  
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
      rollupListener->Rollup(0, false, nullptr, nullptr);
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
    mBrush = nullptr;
  }


  
  if (mCursor == -1)
    SetCursor(eCursor_standard);

#ifdef MOZ_XUL
  
  if (eTransparencyTransparent == mTransparencyMode)
    SetupTranslucentWindowMemoryBitmap(eTransparencyOpaque);
#endif

  
  mGesture.PanFeedbackFinalize(mWnd, true);

  
  mWnd = nullptr;
}


bool nsWindow::OnResize(nsIntRect &aWindowRect)
{
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
nsWindow::ClearCompositor(nsWindow* aWindow)
{
  aWindow->DestroyLayerManager();
}

bool
nsWindow::ShouldUseOffMainThreadCompositing()
{
  
  
  
  if (mTransparencyMode == eTransparencyTransparent) {
    return false;
  }

  return nsBaseWidget::ShouldUseOffMainThreadCompositing();
}

void
nsWindow::GetPreferredCompositorBackends(nsTArray<LayersBackend>& aHints)
{
  LayerManagerPrefs prefs;
  GetLayerManagerPrefs(&prefs);

  
  
  
  if (!(prefs.mDisableAcceleration ||
        mTransparencyMode == eTransparencyTransparent)) {
    if (prefs.mPreferOpenGL) {
      aHints.AppendElement(LayersBackend::LAYERS_OPENGL);
    }

    if (!prefs.mPreferD3D9) {
      aHints.AppendElement(LayersBackend::LAYERS_D3D11);
    }
    if (prefs.mPreferD3D9 || !mozilla::IsVistaOrLater()) {
      
      aHints.AppendElement(LayersBackend::LAYERS_D3D9);
    }
  }
  aHints.AppendElement(LayersBackend::LAYERS_BASIC);
}

void
nsWindow::WindowUsesOMTC()
{
  ULONG_PTR style = ::GetClassLongPtr(mWnd, GCL_STYLE);
  if (!style) {
    NS_WARNING("Could not get window class style");
    return;
  }
  style |= CS_HREDRAW | CS_VREDRAW;
  DebugOnly<ULONG_PTR> result = ::SetClassLongPtr(mWnd, GCL_STYLE, style);
  NS_WARN_IF_FALSE(result, "Could not reset window class style");
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
            if (status == nsIGfxInfo::FEATURE_STATUS_OK || prefs.mForceAcceleration)
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











nsresult
nsWindow::NotifyIMEInternal(const IMENotification& aIMENotification)
{
  return IMEHandler::NotifyIME(this, aIMENotification);
}

NS_IMETHODIMP_(void)
nsWindow::SetInputContext(const InputContext& aContext,
                          const InputContextAction& aAction)
{
  InputContext newInputContext = aContext;
  IMEHandler::SetInputContext(this, newInputContext, aAction);
  mInputContext = newInputContext;
}

NS_IMETHODIMP_(InputContext)
nsWindow::GetInputContext()
{
  mInputContext.mIMEState.mOpen = IMEState::CLOSED;
  if (WinUtils::IsIMEEnabled(mInputContext) && IMEHandler::GetOpenState(this)) {
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

nsIMEUpdatePreference
nsWindow::GetIMEUpdatePreference()
{
  return IMEHandler::GetUpdatePreference();
}

#ifdef ACCESSIBILITY
#ifdef DEBUG
#define NS_LOG_WMGETOBJECT(aWnd, aHwnd, aAcc)                                  \
  if (a11y::logging::IsEnabled(a11y::logging::ePlatforms)) {                   \
    printf("Get the window:\n  {\n     HWND: %p, parent HWND: %p, wndobj: %p,\n",\
           aHwnd, ::GetParent(aHwnd), aWnd);                                   \
    printf("     acc: %p", aAcc);                                              \
    if (aAcc) {                                                                \
      nsAutoString name;                                                       \
      aAcc->Name(name);                                                        \
      printf(", accname: %s", NS_ConvertUTF16toUTF8(name).get());              \
    }                                                                          \
    printf("\n }\n");                                                          \
  }

#else
#define NS_LOG_WMGETOBJECT(aWnd, aHwnd, aAcc)
#endif

a11y::Accessible*
nsWindow::GetAccessible()
{
  
  if (a11y::PlatformDisabledState() == a11y::ePlatformIsDisabled)
    return nullptr;

  if (mInDtor || mOnDestroyCalled || mWindowType == eWindowType_invisible) {
    return nullptr;
  }

  
  nsView* view = nsView::GetViewFor(this);
  if (view) {
    nsIFrame* frame = view->GetFrame();
    if (frame && nsLayoutUtils::IsPopup(frame)) {
      nsCOMPtr<nsIAccessibilityService> accService =
        services::GetAccessibilityService();
      if (accService) {
        a11y::DocAccessible* docAcc =
          GetAccService()->GetDocAccessible(frame->PresContext()->PresShell());
        if (docAcc) {
          NS_LOG_WMGETOBJECT(this, mWnd,
                             docAcc->GetAccessibleOrDescendant(frame->GetContent()));
          return docAcc->GetAccessibleOrDescendant(frame->GetContent());
        }
      }
    }
  }

  
  NS_LOG_WMGETOBJECT(this, mWnd, GetRootAccessible());
  return GetRootAccessible();
}
#endif











#ifdef MOZ_XUL

void nsWindow::ResizeTranslucentWindow(int32_t aNewWidth, int32_t aNewHeight, bool force)
{
  if (!force && aNewWidth == mBounds.width && aNewHeight == mBounds.height)
    return;

  nsRefPtr<gfxWindowsSurface> newSurface =
    new gfxWindowsSurface(gfxIntSize(aNewWidth, aNewHeight), gfxImageFormat::ARGB32);
  mTransparentSurface = newSurface;
  mMemoryDC = newSurface->GetDC();
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
    mMemoryDC = nullptr;
  }
}

void nsWindow::ClearTranslucentWindow()
{
  if (mTransparentSurface) {
    IntSize size = mTransparentSurface->GetSize();
    RefPtr<DrawTarget> drawTarget = gfxPlatform::GetPlatform()->
      CreateDrawTargetForSurface(mTransparentSurface, size);
    drawTarget->ClearRect(Rect(0, 0, size.width, size.height));
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

  
  bool updateSuccesful = 
    ::UpdateLayeredWindow(hWnd, nullptr, (POINT*)&winRect, &winSize, mMemoryDC,
                          &srcPos, 0, &bf, ULW_ALPHA);

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
    
    
    sHookTimerId = ::SetTimer(nullptr, 0, 0, (TIMERPROC)HookTimerForPopups);
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
    while (gMSGFEvents[inx].mId != code && gMSGFEvents[inx].mStr != nullptr) {
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
          
          
          if (static_cast<nsWindow*>(mozWin)->IsPlugin())
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
    sMsgFilterHook = SetWindowsHookEx(WH_MSGFILTER, MozSpecialMsgFilter,
                                      nullptr, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sMsgFilterHook) {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
             ("***** SetWindowsHookEx is NOT installed for WH_MSGFILTER!\n"));
    }
#endif
  }

  
  if (!sCallProcHook) {
    DISPLAY_NMM_PRT("***** Hooking sCallProcHook!\n");
    sCallProcHook  = SetWindowsHookEx(WH_CALLWNDPROC, MozSpecialWndProc,
                                      nullptr, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sCallProcHook) {
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
             ("***** SetWindowsHookEx is NOT installed for WH_CALLWNDPROC!\n"));
    }
#endif
  }

  
  if (!sCallMouseHook) {
    DISPLAY_NMM_PRT("***** Hooking sCallMouseHook!\n");
    sCallMouseHook  = SetWindowsHookEx(WH_MOUSE, MozSpecialMouseProc,
                                       nullptr, GetCurrentThreadId());
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
    sCallProcHook = nullptr;
  }

  if (sMsgFilterHook) {
    DISPLAY_NMM_PRT("***** Unhooking sMsgFilterHook!\n");
    if (!::UnhookWindowsHookEx(sMsgFilterHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for sMsgFilterHook!\n");
    }
    sMsgFilterHook = nullptr;
  }

  if (sCallMouseHook) {
    DISPLAY_NMM_PRT("***** Unhooking sCallMouseHook!\n");
    if (!::UnhookWindowsHookEx(sCallMouseHook)) {
      DISPLAY_NMM_PRT("***** UnhookWindowsHookEx failed for sCallMouseHook!\n");
    }
    sCallMouseHook = nullptr;
  }
}








VOID CALLBACK nsWindow::HookTimerForPopups(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
  if (sHookTimerId != 0) {
    
    BOOL status = ::KillTimer(nullptr, sHookTimerId);
    NS_ASSERTION(status, "Hook Timer was not killed.");
    sHookTimerId = 0;
  }

  if (sRollupMsgId != 0) {
    
    LRESULT popupHandlingResult;
    nsAutoRollup autoRollup;
    DealWithPopups(sRollupMsgWnd, sRollupMsgId, 0, 0, &popupHandlingResult);
    sRollupMsgId = 0;
    sRollupMsgWnd = nullptr;
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
    if (mLayerManager &&
        mLayerManager->GetBackendType() == LayersBackend::LAYERS_BASIC) {
      mLayerManager->ClearCachedResources();
    }
    ::EnumChildWindows(mWnd, nsWindow::ClearResourcesCallback, 0);
}

static bool IsDifferentThreadWindow(HWND aWnd)
{
  return ::GetCurrentThreadId() != ::GetWindowThreadProcessId(aWnd, nullptr);
}


bool
nsWindow::EventIsInsideWindow(nsWindow* aWindow)
{
  RECT r;
  ::GetWindowRect(aWindow->mWnd, &r);
  DWORD pos = ::GetMessagePos();
  POINT mp;
  mp.x = GET_X_LPARAM(pos);
  mp.y = GET_Y_LPARAM(pos);

  
  return static_cast<bool>(::PtInRect(&r, mp));
}


bool
nsWindow::GetPopupsToRollup(nsIRollupListener* aRollupListener,
                            uint32_t* aPopupsToRollup)
{
  
  
  
  *aPopupsToRollup = UINT32_MAX;
  nsAutoTArray<nsIWidget*, 5> widgetChain;
  uint32_t sameTypeCount =
    aRollupListener->GetSubmenuWidgetChain(&widgetChain);
  for (uint32_t i = 0; i < widgetChain.Length(); ++i) {
    nsIWidget* widget = widgetChain[i];
    if (EventIsInsideWindow(static_cast<nsWindow*>(widget))) {
      
      
      
      
      if (i < sameTypeCount) {
        return false;
      }

      *aPopupsToRollup = sameTypeCount;
      break;
    }
  }
  return true;
}


bool
nsWindow::NeedsToHandleNCActivateDelayed(HWND aWnd)
{
  
  
  
  
  
  
  
  
  
  

  nsWindow* window = WinUtils::GetNSWindowPtr(aWnd);
  return window && !window->IsPopup();
}


bool
nsWindow::DealWithPopups(HWND aWnd, UINT aMessage,
                         WPARAM aWParam, LPARAM aLParam, LRESULT* aResult)
{
  NS_ASSERTION(aResult, "Bad outResult");

  
  *aResult = MA_NOACTIVATE;

  if (!::IsWindowVisible(aWnd)) {
    return false;
  }

  nsIRollupListener* rollupListener = nsBaseWidget::GetActiveRollupListener();
  NS_ENSURE_TRUE(rollupListener, false);

  nsCOMPtr<nsIWidget> popup = rollupListener->GetRollupWidget();
  if (!popup) {
    return false;
  }

  static bool sSendingNCACTIVATE = false;
  static bool sPendingNCACTIVATE = false;
  uint32_t popupsToRollup = UINT32_MAX;

  nsWindow* popupWindow = static_cast<nsWindow*>(popup.get());
  UINT nativeMessage = WinUtils::GetNativeMessage(aMessage);
  switch (nativeMessage) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
      if (!EventIsInsideWindow(popupWindow) &&
          GetPopupsToRollup(rollupListener, &popupsToRollup)) {
        break;
      }
      return false;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      
      
      if (!EventIsInsideWindow(popupWindow)) {
        *aResult = MA_ACTIVATE;
        if (rollupListener->ShouldRollupOnMouseWheelEvent() &&
            GetPopupsToRollup(rollupListener, &popupsToRollup)) {
          break;
        }
      }
      return false;

    case WM_ACTIVATEAPP:
      break;

    case WM_ACTIVATE:
      
      
      if (LOWORD(aWParam) == WA_ACTIVE && aLParam) {
        nsWindow* window = WinUtils::GetNSWindowPtr(aWnd);
        if (window && window->IsPopup()) {
          
          
          sJustGotDeactivate = false;
          
          ::PostMessageW(aWnd, MOZ_WM_REACTIVATE, aWParam, aLParam);
          return true;
        }
        
        
        nsWindow* prevWindow =
          WinUtils::GetNSWindowPtr(reinterpret_cast<HWND>(aLParam));
        if (prevWindow && prevWindow->IsPopup()) {
          return false;
        }
      } else if (LOWORD(aWParam) == WA_INACTIVE) {
        nsWindow* activeWindow =
          WinUtils::GetNSWindowPtr(reinterpret_cast<HWND>(aLParam));
        if (sPendingNCACTIVATE && NeedsToHandleNCActivateDelayed(aWnd)) {
          
          
          if (!activeWindow || !activeWindow->IsPopup()) {
            sSendingNCACTIVATE = true;
            ::SendMessageW(aWnd, WM_NCACTIVATE, false, 0);
            sSendingNCACTIVATE = false;
          }
          sPendingNCACTIVATE = false;
        }
        
        
        if (activeWindow) {
          if (activeWindow->IsPopup()) {
            return false;
          }
          nsWindow* deactiveWindow = WinUtils::GetNSWindowPtr(aWnd);
          if (deactiveWindow && deactiveWindow->IsPopup()) {
            return false;
          }
        }
      } else if (LOWORD(aWParam) == WA_CLICKACTIVE) {
        
        
        if (EventIsInsideWindow(popupWindow) ||
            !GetPopupsToRollup(rollupListener, &popupsToRollup)) {
          return false;
        }
      }
      break;

    case MOZ_WM_REACTIVATE:
      
      if (::IsWindow(reinterpret_cast<HWND>(aLParam))) {
        ::SetForegroundWindow(reinterpret_cast<HWND>(aLParam));
      }
      return true;

    case WM_NCACTIVATE:
      if (!aWParam && !sSendingNCACTIVATE &&
          NeedsToHandleNCActivateDelayed(aWnd)) {
        
        
        ::DefWindowProcW(aWnd, aMessage, TRUE, aLParam);
        
        
        *aResult = TRUE;
        sPendingNCACTIVATE = true;
        return true;
      }
      return false;

    case WM_MOUSEACTIVATE:
      if (!EventIsInsideWindow(popupWindow) &&
          GetPopupsToRollup(rollupListener, &popupsToRollup)) {
        
        
        
        if (HIWORD(aLParam) == WM_MOUSEMOVE &&
            !rollupListener->ShouldRollupOnMouseActivate()) {
          return true;
        }
        
        return false;
      }

      
      
      
      
      return true;

    case WM_KILLFOCUS:
      
      
      if (IsDifferentThreadWindow(reinterpret_cast<HWND>(aWParam))) {
        break;
      }
      return false;

    case WM_MOVING:
    case WM_SIZING:
    case WM_MENUSELECT:
      break;

    default:
      return false;
  }

  
  NS_ASSERTION(!mLastRollup, "mLastRollup is null");

  bool consumeRollupEvent;
  if (nativeMessage == WM_LBUTTONDOWN) {
    POINT pt;
    pt.x = GET_X_LPARAM(aLParam);
    pt.y = GET_Y_LPARAM(aLParam);
    ::ClientToScreen(aWnd, &pt);
    nsIntPoint pos(pt.x, pt.y);

    consumeRollupEvent =
      rollupListener->Rollup(popupsToRollup, true, &pos, &mLastRollup);
    NS_IF_ADDREF(mLastRollup);
  } else {
    consumeRollupEvent =
      rollupListener->Rollup(popupsToRollup, true, nullptr, nullptr);
  }

  
  sProcessHook = false;
  sRollupMsgId = 0;
  sRollupMsgWnd = nullptr;

  
  if (consumeRollupEvent && nativeMessage != WM_RBUTTONDOWN) {
    *aResult = MA_ACTIVATE;
    return true;
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

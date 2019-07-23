








































































































#include "nsWindow.h"

#include <windows.h>
#include <process.h>
#include <commctrl.h>
#include <unknwn.h>

#include "prlog.h"
#include "prtime.h"
#include "prprf.h"
#include "prmem.h"

#include "nsIAppShell.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMNSUIEvent.h"
#include "nsITheme.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIScreenManager.h"
#include "imgIContainer.h"
#include "nsIFile.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIRegion.h"
#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsIMM32Handler.h"
#include "nsILocalFile.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsIDeviceContext.h"

#include "nsGUIEvent.h"
#include "nsFont.h"
#include "nsRect.h"
#include "nsThreadUtils.h"
#include "nsNativeCharsetUtils.h"
#include "nsWidgetAtoms.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsWidgetsCID.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

#if defined(WINCE)
#include "nsWindowCE.h"
#endif

#if defined(WINCE_WINDOWS_MOBILE)
#define KILL_PRIORITY_ID 2444
#endif

#include "nsWindowGfx.h"

#if !defined(WINCE)
#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"
#include "nsKeyboardLayout.h"
#include "nsNativeDragTarget.h"
#include <mmsystem.h> 
#include <zmouse.h>
#include <pbt.h>
#include <richedit.h>
#endif 

#if defined(ACCESSIBILITY)
#include "oleidl.h"
#include <winuser.h>
#if !defined(WINABLEAPI)
#include <winable.h>
#endif 
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessNode.h"
#endif 

#if defined(NS_ENABLE_TSF)
#include "nsTextStore.h"
#endif 

#if defined(MOZ_SPLASHSCREEN)
#include "nsSplashScreen.h"
#endif 


#include "nsplugindefs.h"

#include "nsWindowDefs.h"


#include "nsITimer.h"
#ifdef WINCE_WINDOWS_MOBILE
#include "nsGfxCIID.h"
#endif







class nsScrollPrefObserver : public nsIObserver
{
public:
  nsScrollPrefObserver();
  int GetScrollAccelerationStart();
  int GetScrollAccelerationFactor();
  int GetScrollNumLines();
  void RemoveObservers();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

private:
  nsCOMPtr<nsIPrefBranch2> mPrefBranch;
  int mScrollAccelerationStart;
  int mScrollAccelerationFactor;
  int mScrollNumLines;
};

NS_IMPL_ISUPPORTS1(nsScrollPrefObserver, nsScrollPrefObserver)

nsScrollPrefObserver::nsScrollPrefObserver()
{
  nsresult rv;
  mPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);

  rv = mPrefBranch->GetIntPref("mousewheel.acceleration.start",
                               &mScrollAccelerationStart);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), 
                    "Failed to get pref: mousewheel.acceleration.start");
  rv = mPrefBranch->AddObserver("mousewheel.acceleration.start", 
                                this, PR_FALSE);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), 
                    "Failed to add pref observer: mousewheel.acceleration.start");
                    
  rv = mPrefBranch->GetIntPref("mousewheel.acceleration.factor",
                               &mScrollAccelerationFactor);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), 
                    "Failed to get pref: mousewheel.acceleration.factor");
  rv = mPrefBranch->AddObserver("mousewheel.acceleration.factor", 
                                this, PR_FALSE);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), 
                    "Failed to add pref observer: mousewheel.acceleration.factor");
                    
  rv = mPrefBranch->GetIntPref("mousewheel.withnokey.numlines",
                               &mScrollNumLines);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), 
                    "Failed to get pref: mousewheel.withnokey.numlines");
  rv = mPrefBranch->AddObserver("mousewheel.withnokey.numlines", 
                                this, PR_FALSE);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), 
                    "Failed to add pref observer: mousewheel.withnokey.numlines");
}

int nsScrollPrefObserver::GetScrollAccelerationStart()
{
  return mScrollAccelerationStart;
}

int nsScrollPrefObserver::GetScrollAccelerationFactor()
{
  return mScrollAccelerationFactor;
}

int nsScrollPrefObserver::GetScrollNumLines()
{
  return mScrollNumLines;
}

void nsScrollPrefObserver::RemoveObservers()
{
  mPrefBranch->RemoveObserver("mousewheel.acceleration.start", this);
  mPrefBranch->RemoveObserver("mousewheel.acceleration.factor", this);
  mPrefBranch->RemoveObserver("mousewheel.withnokey.numlines", this);
}

NS_IMETHODIMP nsScrollPrefObserver::Observe(nsISupports *aSubject,
                                            const char *aTopic,
                                            const PRUnichar *aData)
{
  mPrefBranch->GetIntPref("mousewheel.acceleration.start",
                          &mScrollAccelerationStart);
  mPrefBranch->GetIntPref("mousewheel.acceleration.factor",
                          &mScrollAccelerationFactor);
  mPrefBranch->GetIntPref("mousewheel.withnokey.numlines",
                          &mScrollNumLines);

  return NS_OK;
}

















PRUint32        nsWindow::sInstanceCount          = 0;
PRBool          nsWindow::sSwitchKeyboardLayout   = PR_FALSE;
BOOL            nsWindow::sIsRegistered           = FALSE;
BOOL            nsWindow::sIsPopupClassRegistered = FALSE;
BOOL            nsWindow::sIsOleInitialized       = FALSE;
HCURSOR         nsWindow::sHCursor                = NULL;
imgIContainer*  nsWindow::sCursorImgContainer     = nsnull;
nsWindow*       nsWindow::sCurrentWindow          = nsnull;
PRBool          nsWindow::sJustGotDeactivate      = PR_FALSE;
PRBool          nsWindow::sJustGotActivate        = PR_FALSE;


TriStateBool    nsWindow::sCanQuit                = TRI_UNKNOWN;




HHOOK           nsWindow::sMsgFilterHook          = NULL;
HHOOK           nsWindow::sCallProcHook           = NULL;
HHOOK           nsWindow::sCallMouseHook          = NULL;
PRPackedBool    nsWindow::sProcessHook            = PR_FALSE;
UINT            nsWindow::sRollupMsgId            = 0;
HWND            nsWindow::sRollupMsgWnd           = NULL;
UINT            nsWindow::sHookTimerId            = 0;


nsIRollupListener* nsWindow::sRollupListener      = nsnull;
nsIWidget*      nsWindow::sRollupWidget           = nsnull;
PRBool          nsWindow::sRollupConsumeEvent     = PR_FALSE;



POINT           nsWindow::sLastMousePoint         = {0};
POINT           nsWindow::sLastMouseMovePoint     = {0};
LONG            nsWindow::sLastMouseDownTime      = 0L;
LONG            nsWindow::sLastClickCount         = 0L;
BYTE            nsWindow::sLastMouseButton        = 0;


int             nsWindow::sTrimOnMinimize         = 2;

#ifdef ACCESSIBILITY
BOOL            nsWindow::sIsAccessibilityOn      = FALSE;

HINSTANCE       nsWindow::sAccLib                 = 0;
LPFNLRESULTFROMOBJECT 
                nsWindow::sLresultFromObject      = 0;
#endif 







static const char *sScreenManagerContractID       = "@mozilla.org/gfx/screenmanager;1";

#ifdef PR_LOGGING
PRLogModuleInfo* gWindowsLog                      = nsnull;
#endif

#ifndef WINCE

static KeyboardLayout gKbdLayout;
#endif

#ifdef WINCE_WINDOWS_MOBILE


const int WM_HTCNAV = 0x0400 + 200;

typedef int (__stdcall * HTCApiNavOpen)(HANDLE, int);
typedef int (__stdcall * HTCApiNavSetMode)(HANDLE, unsigned int);

HTCApiNavOpen    gHTCApiNavOpen = nsnull;
HTCApiNavSetMode gHTCApiNavSetMode = nsnull;
static PRBool    gCheckForHTCApi = PR_FALSE;
#endif





#if !defined(WINCE)
static PRUint32 gLastInputEventTime               = 0;
#else
PRUint32        gLastInputEventTime               = 0;
#endif



PRBool          gDisableNativeTheme               = PR_FALSE;


static PRBool   gWindowsVisible                   = PR_FALSE;

static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);
#ifdef WINCE_WINDOWS_MOBILE
static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);
#endif


static nsScrollPrefObserver* gScrollPrefObserver  = nsnull;


















nsWindow::nsWindow() : nsBaseWidget()
{
#ifdef PR_LOGGING
  if (!gWindowsLog)
    gWindowsLog = PR_NewLogModule("nsWindowsWidgets");
#endif

  mWnd                  = nsnull;
  mPaintDC              = nsnull;
  mPrevWndProc          = nsnull;
  mOldIMC               = nsnull;
  mNativeDragTarget     = nsnull;
  mInDtor               = PR_FALSE;
  mIsVisible            = PR_FALSE;
  mHas3DBorder          = PR_FALSE;
  mIsInMouseCapture     = PR_FALSE;
  mIsPluginWindow       = PR_FALSE;
  mIsTopWidgetWindow    = PR_FALSE;
  mInWheelProcessing    = PR_FALSE;
  mUnicodeWidget        = PR_TRUE;
  mWindowType           = eWindowType_child;
  mBorderStyle          = eBorderStyle_default;
  mPopupType            = ePopupTypeAny;
  mDisplayPanFeedback   = PR_FALSE;
  mLastPoint.x          = 0;
  mLastPoint.y          = 0;
  mLastSize.width       = 0;
  mLastSize.height      = 0;
  mOldStyle             = 0;
  mOldExStyle           = 0;
  mPainting             = 0;
  mLastKeyboardLayout   = 0;
  mBlurSuppressLevel    = 0;
  mIMEEnabled           = nsIWidget::IME_STATUS_ENABLED;
  mLeadByte             = '\0';
#ifdef MOZ_XUL
  mTransparentSurface   = nsnull;
  mMemoryDC             = nsnull;
  mTransparencyMode     = eTransparencyOpaque;
#endif
  mBackground           = ::GetSysColor(COLOR_BTNFACE);
  mBrush                = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
  mForeground           = ::GetSysColor(COLOR_WINDOWTEXT);

  
  mScrollSeriesCounter = 0;

#ifdef WINCE_WINDOWS_MOBILE
  mInvalidatedRegion = do_CreateInstance(kRegionCID);
  mInvalidatedRegion->Init();
#endif

  
  if (!sInstanceCount) {
#if !defined(WINCE)
  gKbdLayout.LoadLayout(::GetKeyboardLayout(0));
#endif

  
  nsIMM32Handler::Initialize();

  
  NS_IF_ADDREF(gScrollPrefObserver = new nsScrollPrefObserver());

#ifdef NS_ENABLE_TSF
  nsTextStore::Initialize();
#endif

#if !defined(WINCE)
  if (SUCCEEDED(::OleInitialize(NULL)))
    sIsOleInitialized = TRUE;
  NS_ASSERTION(sIsOleInitialized, "***** OLE is not initialized!\n");
#endif

#if defined(HEAP_DUMP_EVENT)
  InitHeapDump();
#endif
  } 

  
  gLastInputEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  sInstanceCount++;
}

nsWindow::~nsWindow()
{
  mInDtor = PR_TRUE;

  
  
  
  
  if (NULL != mWnd)
    Destroy();

  sInstanceCount--;

  
  if (sInstanceCount == 0) {
#ifdef NS_ENABLE_TSF
    nsTextStore::Terminate();
#endif

#if !defined(WINCE)
    NS_IF_RELEASE(sCursorImgContainer);
    if (sIsOleInitialized) {
      ::OleFlushClipboard();
      ::OleUninitialize();
      sIsOleInitialized = FALSE;
    }
    
    nsIMM32Handler::Terminate();
#endif 

    gScrollPrefObserver->RemoveObservers();
    NS_RELEASE(gScrollPrefObserver);
  }

#if !defined(WINCE)
  NS_IF_RELEASE(mNativeDragTarget);
#endif 
}

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)










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
  } else if (mWindowType == eWindowType_invisible) {
    
    style &= ~0x40000000; 
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

  if (sTrimOnMinimize == 2 && mWindowType == eWindowType_invisible) {
    









    sTrimOnMinimize = 0;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
      nsCOMPtr<nsIPrefBranch> prefBranch;
      prefs->GetBranch(0, getter_AddRefs(prefBranch));
      if (prefBranch) {

        PRBool temp;
        if (NS_SUCCEEDED(prefBranch->GetBoolPref("config.trim_on_minimize",
                                                 &temp))
            && temp)
          sTrimOnMinimize = 1;

        if (NS_SUCCEEDED(prefBranch->GetBoolPref("intl.keyboard.per_window_layout",
                                                 &temp)))
          sSwitchKeyboardLayout = temp;

        if (NS_SUCCEEDED(prefBranch->GetBoolPref("mozilla.widget.disable-native-theme",
                                                 &temp)))
          gDisableNativeTheme = temp;
      }
    }
  }
#if defined(WINCE_HAVE_SOFTKB)
  if (mWindowType == eWindowType_dialog || mWindowType == eWindowType_toplevel )
     nsWindowCE::CreateSoftKeyMenuBar(mWnd);
#endif

  return NS_OK;
}


NS_METHOD nsWindow::Destroy()
{
  
  if (nsnull == mWnd)
    return NS_OK;

  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);

  
  
  
  
  
  
  
  
  
  
  VERIFY(::DestroyWindow(mWnd));
  
  
  
  if (PR_FALSE == mOnDestroyCalled)
    OnDestroy();

  return NS_OK;
}











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
    wc.lpszClassName = kClassNameDropShadow;

    nsWindow::sIsPopupClassRegistered = ::RegisterClassW(&wc);
    if (!nsWindow::sIsPopupClassRegistered) {
      
      
      wc.style = CS_DBLCLKS;
      nsWindow::sIsPopupClassRegistered = ::RegisterClassW(&wc);
    }
  }

  return kClassNameDropShadow;
}










#if !defined(WINCE) 
DWORD nsWindow::WindowStyle()
{
  DWORD style;

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
      NS_ERROR("unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      style = WS_OVERLAPPED | WS_BORDER | WS_DLGFRAME | WS_SYSMENU |
              WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
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
  }
  VERIFY_WINDOW_STYLE(style);
  return style;
}
#endif 


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
      NS_ERROR("unknown border style");
      

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









static PRUnichar sPropName[40] = L"";
static PRUnichar* GetNSWindowPropName()
{
  if (!*sPropName)
  {
    _snwprintf(sPropName, 39, L"MozillansIWidgetPtr%p", GetCurrentProcessId());
    sPropName[39] = '\0';
  }
  return sPropName;
}

nsWindow * nsWindow::GetNSWindowPtr(HWND aWnd)
{
  return (nsWindow *) ::GetPropW(aWnd, GetNSWindowPropName());
}

BOOL nsWindow::SetNSWindowPtr(HWND aWnd, nsWindow * ptr)
{
  if (ptr == NULL) {
    ::RemovePropW(aWnd, GetNSWindowPropName());
    return TRUE;
  } else {
    return ::SetPropW(aWnd, GetNSWindowPropName(), (HANDLE)ptr);
  }
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
    
    VERIFY(::SetParent(mWnd, nsnull));
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

  
  
  
  if (mInDtor || mOnDestroyCalled)
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
        
        
        if (widget->mInDtor) {
          widget = nsnull;
        }
      }
    }
  }

  return widget;
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
          case nsSizeMode_Fullscreen:
            ::SetForegroundWindow(mWnd);
            ::ShowWindow(mWnd, SW_SHOWMAXIMIZED);
            MakeFullScreen(TRUE);
            break;

          case nsSizeMode_Maximized :
            ::SetForegroundWindow(mWnd);
            ::ShowWindow(mWnd, SW_SHOWMAXIMIZED);
            break;
          
#else
          case nsSizeMode_Fullscreen:
            ::ShowWindow(mWnd, SW_SHOWMAXIMIZED);
            break;

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
    ClearThemeRegion();
    VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, 0, 0,
                          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
    SetThemeRegion();
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
    UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;

#ifndef WINCE
    if (!aRepaint) {
      flags |= SWP_NOREDRAW;
    }
#endif

    ClearThemeRegion();
    VERIFY(::SetWindowPos(mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), flags));
    SetThemeRegion();
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
    UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE;
#ifndef WINCE
    if (!aRepaint) {
      flags |= SWP_NOREDRAW;
    }
#endif

    ClearThemeRegion();
    VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), flags));
    SetThemeRegion();
  }

  if (aRepaint)
    Invalidate(PR_FALSE);

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


#if !defined(WINCE) 
NS_IMETHODIMP nsWindow::SetSizeMode(PRInt32 aMode) {

  nsresult rv;

  
  
  
  if (aMode == mSizeMode)
    return NS_OK;

  
  rv = nsBaseWidget::SetSizeMode(aMode);
  if (NS_SUCCEEDED(rv) && mIsVisible) {
    int mode;

    switch (aMode) {
      case nsSizeMode_Fullscreen :
        mode = SW_MAXIMIZE;
        break;

      case nsSizeMode_Maximized :
        mode = SW_MAXIMIZE;
        break;
      case nsSizeMode_Minimized :
        mode = sTrimOnMinimize ? SW_MINIMIZE : SW_SHOWMINIMIZED;
        if (!sTrimOnMinimize) {
          
          HWND hwndBelow = ::GetNextWindow(mWnd, GW_HWNDNEXT);
          while (hwndBelow && (!::IsWindowEnabled(hwndBelow) || !::IsWindowVisible(hwndBelow) ||
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
      default :
        mode = SW_RESTORE;
    }
    ::ShowWindow(mWnd, mode);
    
    
    if (mode == SW_RESTORE || mode == SW_MAXIMIZE)
      DispatchFocusToTopLevelWindow(NS_ACTIVATE);
  }
  return rv;
}
#endif 


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
                                  PRUint32 aHotspotX, PRUint32 aHotspotY)
{
  if (sCursorImgContainer == aCursor && sHCursor) {
    ::SetCursor(sHCursor);
    return NS_OK;
  }

  PRInt32 width;
  PRInt32 height;

  nsresult rv;
  rv = aCursor->GetWidth(&width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCursor->GetHeight(&height);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (width > 128 || height > 128)
    return NS_ERROR_NOT_AVAILABLE;

  HCURSOR cursor;
  rv = nsWindowGfx::CreateIcon(aCursor, PR_TRUE, aHotspotX, aHotspotY, &cursor);
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
  return GetTopLevelWindow(PR_TRUE)->GetWindowTranslucencyInner();
}

void nsWindow::SetTransparencyMode(nsTransparencyMode aMode)
{
  GetTopLevelWindow(PR_TRUE)->SetWindowTranslucencyInner(aMode);
}
#endif









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

#ifdef WINCE_WINDOWS_MOBILE
static inline void AddRECTToRegion(const RECT& aRect, nsIRegion* aRegion)
{
  aRegion->Union(aRect.left, aRect.top, aRect.right - aRect.left, aRect.bottom - aRect.top);
}
#endif


NS_METHOD nsWindow::Invalidate(PRBool aIsSynchronous)
{
  if (mWnd)
  {
#ifdef WIDGET_DEBUG_OUTPUT
    debug_DumpInvalidate(stdout,
                         this,
                         nsnull,
                         aIsSynchronous,
                         nsCAutoString("noname"),
                         (PRInt32) mWnd);
#endif 
#ifdef WINCE_WINDOWS_MOBILE
    
    RECT r;
    GetClientRect(mWnd, &r);
    AddRECTToRegion(r, mInvalidatedRegion);
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
#ifdef WIDGET_DEBUG_OUTPUT
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

#ifdef WINCE_WINDOWS_MOBILE
    
    AddRECTToRegion(rect, mInvalidatedRegion);
#endif
    VERIFY(::InvalidateRect(mWnd, &rect, FALSE));

    if (aIsSynchronous) {
      VERIFY(::UpdateWindow(mWnd));
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(PRBool aFullScreen)
{
#if WINCE_WINDOWS_MOBILE
  RECT rc;
  if (aFullScreen) {
    SetForegroundWindow(mWnd);
    SHFullScreen(mWnd, SHFS_HIDETASKBAR | SHFS_HIDESTARTICON);
    SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  }
  else {
    SHFullScreen(mWnd, SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON);
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, FALSE);
  }
  MoveWindow(mWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);

  if (aFullScreen)
    mSizeMode = nsSizeMode_Fullscreen;
#endif

  return nsBaseWidget::MakeFullScreen(aFullScreen);
}









NS_IMETHODIMP nsWindow::Update()
{
  nsresult rv = NS_OK;

  
  
  if (mWnd)
    VERIFY(::UpdateWindow(mWnd));

  return rv;
}









static PRBool
ClipRegionContainedInRect(const nsTArray<nsIntRect>& aClipRects,
                          const nsIntRect& aRect)
{
  for (PRUint32 i = 0; i < aClipRects.Length(); ++i) {
    if (!aRect.Contains(aClipRects[i]))
      return PR_FALSE;
  }
  return PR_TRUE;
}

void
nsWindow::Scroll(const nsIntPoint& aDelta,
                 const nsTArray<nsIntRect>& aDestRects,
                 const nsTArray<Configuration>& aConfigurations)
{
  
  
  
  
  
  
  nsTHashtable<nsPtrHashKey<nsWindow> > scrolledWidgets;
  scrolledWidgets.Init();
  for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
    const Configuration& configuration = aConfigurations[i];
    nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
    NS_ASSERTION(w->GetParent() == this,
                 "Configured widget is not a child");
    if (configuration.mBounds == w->mBounds + aDelta) {
      scrolledWidgets.PutEntry(w);
    }
    w->SetWindowClipRegion(configuration.mClipRegion, PR_TRUE);
  }

  for (PRUint32 i = 0; i < aDestRects.Length(); ++i) {
    nsIntRect affectedRect;
    affectedRect.UnionRect(aDestRects[i], aDestRects[i] - aDelta);
    
    
    UINT flags = SW_SCROLLCHILDREN | SW_INVALIDATE;
    
    
    for (nsWindow* w = static_cast<nsWindow*>(GetFirstChild()); w;
         w = static_cast<nsWindow*>(w->GetNextSibling())) {
      if (w->mBounds.Intersects(affectedRect)) {
        
        nsPtrHashKey<nsWindow>* entry = scrolledWidgets.GetEntry(w);
        if (entry) {
          
          
          
          
          scrolledWidgets.RawRemoveEntry(entry);
        } else {
          flags &= ~SW_SCROLLCHILDREN;
          
          
          
          
          break;
        }
      }
    }

    if (flags & SW_SCROLLCHILDREN) {
      for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
        const Configuration& configuration = aConfigurations[i];
        nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
        
        
        
        
        
        if (w->mBounds.Intersects(affectedRect) &&
            !ClipRegionContainedInRect(configuration.mClipRegion,
                                       affectedRect - (w->mBounds.TopLeft() + aDelta))) {
          w->Invalidate(PR_FALSE);
        }
      }

      
      
      
      
      
      for (nsWindow* w = static_cast<nsWindow*>(GetFirstChild()); w;
           w = static_cast<nsWindow*>(w->GetNextSibling())) {
        if (w->mBounds.Intersects(affectedRect)) {
          w->mBounds += aDelta;
        }
      }
    }

    RECT clip = { affectedRect.x, affectedRect.y, affectedRect.XMost(), affectedRect.YMost() };
    ::ScrollWindowEx(mWnd, aDelta.x, aDelta.y, &clip, &clip, NULL, NULL, flags);
  }

  
  
  
  
  ConfigureChildren(aConfigurations);
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









nsIntPoint nsWindow::WidgetToScreenOffset()
{
  POINT point;
  point.x = 0;
  point.y = 0;
  ::ClientToScreen(mWnd, &point);
  return nsIntPoint(point.x, point.y);
}









#if !defined(WINCE) 
NS_METHOD nsWindow::EnableDragDrop(PRBool aEnable)
{
  nsresult rv = NS_ERROR_FAILURE;
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
  return rv;
}
#endif









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











NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener,
                                            PRBool aDoCapture,
                                            PRBool aConsumeRollupEvent)
{
  if (aDoCapture) {
    


    NS_ASSERTION(!sRollupWidget, "rollup widget reassigned before release");
    sRollupConsumeEvent = aConsumeRollupEvent;
    NS_IF_RELEASE(sRollupListener);
    NS_IF_RELEASE(sRollupWidget);
    sRollupListener = aListener;
    NS_ADDREF(aListener);
    sRollupWidget = this;
    NS_ADDREF(this);

#ifndef WINCE
    if (!sMsgFilterHook && !sCallProcHook && !sCallMouseHook) {
      RegisterSpecialDropdownHooks();
    }
    sProcessHook = PR_TRUE;
#endif
    
  } else {
    NS_IF_RELEASE(sRollupListener);
    NS_IF_RELEASE(sRollupWidget);
    
#ifndef WINCE
    sProcessHook = PR_FALSE;
    UnregisterSpecialDropdownHooks();
#endif
  }

  return NS_OK;
}










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
  
  
  
  
  
  if (HIWORD(GetQueueStatus(QS_INPUT)))
    return PR_TRUE;
#ifdef WINCE
  return PR_FALSE;
#else
  GUITHREADINFO guiInfo;
  guiInfo.cbSize = sizeof(GUITHREADINFO);
  if (!GetGUIThreadInfo(GetCurrentThreadId(), &guiInfo))
    return PR_FALSE;
  return GUI_INMOVESIZE == (guiInfo.flags & GUI_INMOVESIZE);
#endif
}









gfxASurface *nsWindow::GetThebesSurface()
{
  if (mPaintDC)
    return (new gfxWindowsSurface(mPaintDC));

  return (new gfxWindowsSurface(mWnd));
}








 
NS_IMETHODIMP
nsWindow::OnDefaultButtonLoaded(const nsIntRect &aButtonRect)
{
#ifdef WINCE
  return NS_ERROR_NOT_IMPLEMENTED;
#else
  if (aButtonRect.IsEmpty())
    return NS_OK;

  
  HWND activeWnd = ::GetActiveWindow();
  if (activeWnd != ::GetForegroundWindow() ||
      GetTopLevelHWND(mWnd, PR_TRUE) != GetTopLevelHWND(activeWnd, PR_TRUE)) {
    return NS_OK;
  }

  PRBool isAlwaysSnapCursor = PR_FALSE;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    nsCOMPtr<nsIPrefBranch> prefBranch;
    prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
    if (prefBranch) {
      prefBranch->GetBoolPref("ui.cursor_snapping.always_enabled",
                              &isAlwaysSnapCursor);
    }
  }

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
#endif
}




















MSG nsWindow::InitMSG(UINT aMessage, WPARAM wParam, LPARAM lParam)
{
  MSG msg;
  msg.message = aMessage;
  msg.wParam  = wParam;
  msg.lParam  = lParam;
  return msg;
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

#ifndef WINCE
  event.time = ::GetMessageTime();
#else
  event.time = PR_Now() / 1000;
#endif

  mLastPoint = event.refPoint;
}











NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
#ifdef WIDGET_DEBUG_OUTPUT
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

PRBool nsWindow::DispatchStandardEvent(PRUint32 aMsg)
{
  nsGUIEvent event(PR_TRUE, aMsg, this);
  InitEvent(event);

  PRBool result = DispatchWindowEvent(&event);
  return result;
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

    
    
    
#if !defined(WINCE)
    ::EnumChildWindows(topWnd, nsWindow::DispatchStarvedPaints, NULL);
#else
    nsWindowCE::EnumChildWindows(topWnd, nsWindow::DispatchStarvedPaints, NULL);
#endif
  }
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

void nsWindow::RemoveMessageAndDispatchPluginEvent(UINT aFirstMsg,
                                                   UINT aLastMsg)
{
  MSG msg;
  ::GetMessageW(&msg, mWnd, aFirstMsg, aLastMsg);
  DispatchPluginEvent(msg);
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
    if ((sLastMouseMovePoint.x == mpScreen.x) && (sLastMouseMovePoint.y == mpScreen.y))
      return result;
    sLastMouseMovePoint.x = mpScreen.x;
    sLastMouseMovePoint.y = mpScreen.y;
  }

  PRBool insideMovementThreshold = (abs(sLastMousePoint.x - eventPoint.x) < (short)::GetSystemMetrics(SM_CXDOUBLECLK)) &&
                                   (abs(sLastMousePoint.y - eventPoint.y) < (short)::GetSystemMetrics(SM_CYDOUBLECLK));

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

  
  
#ifndef WINCE
  LONG curMsgTime = ::GetMessageTime();
#else
  LONG curMsgTime = PR_Now() / 1000;
#endif

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
  event.clickCount = sLastClickCount;

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
        if (sCurrentWindow == NULL || sCurrentWindow != this) {
          if ((nsnull != sCurrentWindow) && (!sCurrentWindow->mInDtor)) {
            LPARAM pos = sCurrentWindow->lParamToClient(lParamToScreen(lParam));
            sCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT, wParam, pos);
          }
          sCurrentWindow = this;
          if (!mInDtor) {
            LPARAM pos = sCurrentWindow->lParamToClient(lParamToScreen(lParam));
            sCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER, wParam, pos);
          }
        }
      }
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (sCurrentWindow == this) {
        sCurrentWindow = nsnull;
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
    sJustGotActivate = PR_FALSE;
  sJustGotDeactivate = PR_FALSE;

  
  
  
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

PRBool nsWindow::IsTopLevelMouseExit(HWND aWnd)
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

PRBool nsWindow::BlurEventsSuppressed()
{
  
  if (mBlurSuppressLevel > 0)
    return PR_TRUE;

  
  HWND parentWnd = ::GetParent(mWnd);
  if (parentWnd) {
    nsWindow *parent = GetNSWindowPtr(parentWnd);
    if (parent)
      return parent->BlurEventsSuppressed();
  }
  return PR_FALSE;
}




void nsWindow::SuppressBlurEvents(PRBool aSuppress)
{
  if (aSuppress)
    ++mBlurSuppressLevel; 
  else {
    NS_ASSERTION(mBlurSuppressLevel > 0, "unbalanced blur event suppression");
    if (mBlurSuppressLevel > 0)
      --mBlurSuppressLevel;
  }
}

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  return aStatus == nsEventStatus_eConsumeNoDefault;
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
  if (!someWindow->mInDtor) 
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


PRBool nsWindow::ProcessMessage(UINT msg, WPARAM &wParam, LPARAM &lParam,
                                LRESULT *aRetValue)
{
  
  if (mWindowHook.Notify(mWnd, msg, wParam, lParam, aRetValue))
    return PR_TRUE;
  
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
  *aRetValue = 0;

  static PRBool getWheelInfo = PR_TRUE;

#if defined(EVENT_DEBUG_OUTPUT)
  
  
  PrintEvent(msg, SHOW_REPEAT_EVENTS, SHOW_MOUSEMOVE_EVENTS);
#endif

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

    case WM_HOTKEY:
      result = OnHotKey(wParam, lParam);
      break;

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
#ifdef WINCE_WINDOWS_MOBILE
      
      
      SetTimer(mWnd, KILL_PRIORITY_ID, 2000 , NULL);
#endif
      
      
      
      LPARAM lParamScreen = lParamToScreen(lParam);
      POINT mp;
      mp.x      = GET_X_LPARAM(lParamScreen);
      mp.y      = GET_Y_LPARAM(lParamScreen);
      PRBool userMovedMouse = PR_FALSE;
      if ((sLastMouseMovePoint.x != mp.x) || (sLastMouseMovePoint.y != mp.y)) {
        userMovedMouse = PR_TRUE;
      }

      result = DispatchMouseEvent(NS_MOUSE_MOVE, wParam, lParam);
      if (userMovedMouse) {
        DispatchPendingEvents();
      }
    }
    break;

#ifdef WINCE_WINDOWS_MOBILE
    case WM_TIMER:
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
      KillTimer(mWnd, KILL_PRIORITY_ID);
      break;
#endif

    case WM_LBUTTONDOWN:
    {
#ifdef WINCE_WINDOWS_MOBILE
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
      SetTimer(mWnd, KILL_PRIORITY_ID, 2000 , NULL);
#endif
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, wParam, lParam,
                                  PR_FALSE, nsMouseEvent::eLeftButton);
      DispatchPendingEvents();
    }
    break;

    case WM_LBUTTONUP:
    {
      result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, wParam, lParam,
                                  PR_FALSE, nsMouseEvent::eLeftButton);
      DispatchPendingEvents();

#ifdef WINCE_WINDOWS_MOBILE
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
      KillTimer(mWnd, KILL_PRIORITY_ID);
#endif
    }
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
        if (mIsTopWidgetWindow && sSoftKeyboardState)
          nsWindowCE::ToggleSoftKB(fActive);
        if (nsWindowCE::sShowSIPButton != TRI_TRUE && WA_INACTIVE != fActive) {
          HWND hWndSIPB = FindWindowW(L"MS_SIPBUTTON", NULL ); 
          if (hWndSIPB)
            ShowWindow(hWndSIPB, SW_HIDE);
        }

#endif

        if (WA_INACTIVE == fActive) {
          
          
          
          if (HIWORD(wParam))
            result = DispatchFocusToTopLevelWindow(NS_DEACTIVATE);
          else
            sJustGotDeactivate = PR_TRUE;
#ifndef WINCE
          if (mIsTopWidgetWindow)
            mLastKeyboardLayout = gKbdLayout.GetLayout();
#endif

        } else {
          StopFlashing();

          sJustGotActivate = PR_TRUE;
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

          if (sSwitchKeyboardLayout && mLastKeyboardLayout)
            ActivateKeyboardLayout(mLastKeyboardLayout, 0);
#else
          *aRetValue = 0;
#endif
          if (mSizeMode == nsSizeMode_Fullscreen)
            MakeFullScreen(TRUE);
        }
      }
#ifdef WINCE_WINDOWS_MOBILE
      if (!gCheckForHTCApi && gHTCApiNavOpen == nsnull) {
        gCheckForHTCApi = PR_TRUE;

        HINSTANCE library = LoadLibrary(L"HTCAPI.dll"); 
        gHTCApiNavOpen    = (HTCApiNavOpen)    GetProcAddress(library, "HTCNavOpen"); 
        gHTCApiNavSetMode = (HTCApiNavSetMode) GetProcAddress(library ,"HTCNavSetMode"); 
      }
      
      if (gHTCApiNavOpen != nsnull) {
        gHTCApiNavOpen(mWnd, 1 );

        if (gHTCApiNavSetMode != nsnull)
          gHTCApiNavSetMode ( mWnd, 4);
        
      }
#endif
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
      OnWindowPosChanging(info);
    }
    break;
#endif

    case WM_SETFOCUS:
      if (sJustGotActivate) {
        result = DispatchFocusToTopLevelWindow(NS_ACTIVATE);
      }

#ifdef ACCESSIBILITY
      if (nsWindow::sIsAccessibilityOn) {
        
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
      if (sJustGotDeactivate) {
        result = DispatchFocusToTopLevelWindow(NS_DEACTIVATE);
      }
      break;

    case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS *wp = (LPWINDOWPOS)lParam;
      OnWindowPosChanged(wp, result);
    }
    break;

    case WM_SETTINGCHANGE:
#if !defined (WINCE_WINDOWS_MOBILE)
      getWheelInfo = PR_TRUE;
#else
      switch (wParam) {
        case SPI_SIPMOVE:
        case SPI_SETSIPINFO:
        case SPI_SETCURRENTIM:
          nsWindowCE::NotifySoftKbObservers();
          break;
      }
#endif
      OnSettingsChange(wParam, lParam);
      break;

#ifndef WINCE
    case WM_INPUTLANGCHANGEREQUEST:
      *aRetValue = TRUE;
      result = PR_FALSE;
      break;

    case WM_INPUTLANGCHANGE:
      result = OnInputLangChange((HKL)lParam);
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
      
      if (!sTrimOnMinimize && wParam == SC_MINIMIZE) {
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

  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL:
    {
      
      
      
      
      
      
      if (OnMouseWheel(msg, wParam, lParam, getWheelInfo, result, aRetValue))
        return result;
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

#if !defined(WINCE)
  
  case WM_TABLET_QUERYSYSTEMGESTURESTATUS:
    
    
    result = PR_TRUE;
    *aRetValue = TABLET_ROTATE_GESTURE_ENABLE;
    break;
    
  case WM_GESTURE:
    result = OnGesture(wParam, lParam);
    break;

  case WM_GESTURENOTIFY:
    {
      if (mWindowType != eWindowType_invisible &&
          mWindowType != eWindowType_plugin &&
          mWindowType != eWindowType_java &&
          mWindowType != eWindowType_toplevel) {
        
        
        
        
        GESTURENOTIFYSTRUCT * gestureinfo = (GESTURENOTIFYSTRUCT*)lParam;
        nsPointWin touchPoint;
        touchPoint = gestureinfo->ptsLocation;
        touchPoint.ScreenToClient(mWnd);
        nsGestureNotifyEvent gestureNotifyEvent(PR_TRUE, NS_GESTURENOTIFY_EVENT_START, this);
        gestureNotifyEvent.refPoint = touchPoint;
        nsEventStatus status;
        DispatchEvent(&gestureNotifyEvent, status);
        mDisplayPanFeedback = gestureNotifyEvent.displayPanFeedback;
        mGesture.SetWinGestureSupport(mWnd, gestureNotifyEvent.panDirection);
      }
      result = PR_FALSE; 
    }
    break;
#endif 

    case WM_CLEAR:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_DELETE, this);
      DispatchWindowEvent(&command);
      result = PR_TRUE;
    }
    break;

    case WM_CUT:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_CUT, this);
      DispatchWindowEvent(&command);
      result = PR_TRUE;
    }
    break;

    case WM_COPY:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_COPY, this);
      DispatchWindowEvent(&command);
      result = PR_TRUE;
    }
    break;

    case WM_PASTE:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_PASTE, this);
      DispatchWindowEvent(&command);
      result = PR_TRUE;
    }
    break;

#ifndef WINCE
    case EM_UNDO:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_UNDO, this);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = PR_TRUE;
    }
    break;

    case EM_REDO:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_REDO, this);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = PR_TRUE;
    }
    break;

    case EM_CANPASTE:
    {
      
      
      if (wParam == 0 || wParam == CF_TEXT || wParam == CF_UNICODETEXT) {
        nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_PASTE,
                                      this, PR_TRUE);
        DispatchWindowEvent(&command);
        *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
        result = PR_TRUE;
      }
    }
    break;

    case EM_CANUNDO:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_UNDO,
                                    this, PR_TRUE);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = PR_TRUE;
    }
    break;

    case EM_CANREDO:
    {
      nsContentCommandEvent command(PR_TRUE, NS_CONTENT_COMMAND_REDO,
                                    this, PR_TRUE);
      DispatchWindowEvent(&command);
      *aRetValue = (LRESULT)(command.mSucceeded && command.mIsEnabled);
      result = PR_TRUE;
    }
    break;
#endif

#ifdef WINCE_WINDOWS_MOBILE
   
   case WM_HTCNAV:
   {
     int distance = wParam & 0x000000FF;
     if ( (wParam & 0x000000100) != 0) 
       distance *= -1;
     if (OnMouseWheel(WM_MOUSEWHEEL, MAKEWPARAM(0, distance), 
                      MAKELPARAM(GetSystemMetrics(SM_CXSCREEN) / 2, 
                                 GetSystemMetrics(SM_CYSCREEN) / 2), 
                      getWheelInfo, result, aRetValue))
        return result;
   }
   break;
#endif

    default:
    {
#ifdef NS_ENABLE_TSF
      if (msg == WM_USER_TSF_TEXTCHANGE) {
        nsTextStore::OnTextChangeMsg();
      }
#endif 
#if defined(HEAP_DUMP_EVENT)
      if (msg == GetHeapMsg()) {
        HeapDump(msg, wParam, lParam);
        result = PR_TRUE;
      }
#endif
    }
    break;
  }

  
  if (mWnd) {
    return result;
  }
  else {
    
    
    return PR_TRUE;
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
  
  
#if !defined(WINCE)
  ::EnumChildWindows(aTopWindow, nsWindow::BroadcastMsgToChildren, aMsg);
#else
  nsWindowCE::EnumChildWindows(aTopWindow, nsWindow::BroadcastMsgToChildren, aMsg);
#endif
  return TRUE;
}



void nsWindow::GlobalMsgWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_SYSCOLORCHANGE:
      
      
      
      
      
      
      
      
      
#if !defined(WINCE)
     ::EnumThreadWindows(GetCurrentThreadId(), nsWindow::BroadcastMsg, msg);
#endif
    break;
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

LRESULT nsWindow::ProcessCharMessage(const MSG &aMsg, PRBool *aEventDispatched)
{
  NS_PRECONDITION(aMsg.message == WM_CHAR || aMsg.message == WM_SYSCHAR,
                  "message is not keydown event");
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
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
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
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
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
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

void nsWindow::OnWindowPosChanged(WINDOWPOS *wp, PRBool& result)
{
  if (wp == nsnull)
    return;

  
  
  
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
    if (mWnd == toplevelWnd && IsIconic(toplevelWnd)) {
      result = PR_FALSE;
      return;
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
    event.mSizeMode = mSizeMode;
#endif
    
    
    
    
    
    
    
    mSizeMode = event.mSizeMode;

    InitEvent(event);

    result = DispatchWindowEvent(&event);
  }
}

#if !defined(WINCE)
void nsWindow::OnWindowPosChanging(LPWINDOWPOS& info)
{
  
  if (!(info->flags & SWP_NOZORDER)) {
    HWND hwndAfter = info->hwndInsertAfter;
    
    nsZLevelEvent event(PR_TRUE, NS_SETZLEVEL, this);
    nsWindow *aboveWindow = 0;

    InitEvent(event);

    if (hwndAfter == HWND_BOTTOM)
      event.mPlacement = nsWindowZBottom;
    else if (hwndAfter == HWND_TOP || hwndAfter == HWND_TOPMOST || hwndAfter == HWND_NOTOPMOST)
      event.mPlacement = nsWindowZTop;
    else {
      event.mPlacement = nsWindowZRelative;
      aboveWindow = GetNSWindowPtr(hwndAfter);
    }
    event.mReqBelow = aboveWindow;
    event.mActualBelow = nsnull;

    event.mImmediate = PR_FALSE;
    event.mAdjusted = PR_FALSE;
    DispatchWindowEvent(&event);

    if (event.mAdjusted) {
      if (event.mPlacement == nsWindowZBottom)
        info->hwndInsertAfter = HWND_BOTTOM;
      else if (event.mPlacement == nsWindowZTop)
        info->hwndInsertAfter = HWND_TOP;
      else {
        info->hwndInsertAfter = (HWND)event.mActualBelow->GetNativeData(NS_NATIVE_WINDOW);
      }
    }
    NS_IF_RELEASE(event.mActualBelow);
  }
  
  if (mWindowType == eWindowType_invisible)
    info->flags &= ~SWP_SHOWWINDOW;
}
#endif


#if !defined(WINCE)
PRBool nsWindow::OnGesture(WPARAM wParam, LPARAM lParam)
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

    PRInt32 scrollOverflowX = 0;
    PRInt32 scrollOverflowY = 0;

    if (mGesture.PanDeltaToPixelScrollX(event)) {
      DispatchEvent(&event, status);
      scrollOverflowX = event.scrollOverflow;
    }

    if (mGesture.PanDeltaToPixelScrollY(event)) {
      DispatchEvent(&event, status);
      scrollOverflowY = event.scrollOverflow;
    }

    if (mDisplayPanFeedback) {
      mGesture.UpdatePanFeedbackX(mWnd, scrollOverflowX, endFeedback);
      mGesture.UpdatePanFeedbackY(mWnd, scrollOverflowY, endFeedback);
      mGesture.PanFeedbackFinalize(mWnd, endFeedback);
    }

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
#endif 






PRBool nsWindow::OnMouseWheel(UINT msg, WPARAM wParam, LPARAM lParam, PRBool& getWheelInfo, PRBool& result, LRESULT *aRetValue)
{
  
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
    return PR_FALSE; 

  
  

  POINT point;
  point.x = GET_X_LPARAM(lParam);
  point.y = GET_Y_LPARAM(lParam);
  HWND destWnd = ::WindowFromPoint(point);

  
  
  
  

  if (!destWnd) {
    
    return PR_FALSE; 
  }

  
  DWORD processId = 0;
  GetWindowThreadProcessId(destWnd, &processId);
  if (processId != GetCurrentProcessId())
  {
    
    return PR_FALSE; 
  }

  nsWindow* destWindow = GetNSWindowPtr(destWnd);
  if (!destWindow || destWindow->mIsPluginWindow) {
    
    
    
    
    
    
    
    
    
    HWND parentWnd = ::GetParent(destWnd);
    while (parentWnd) {
      nsWindow* parentWindow = GetNSWindowPtr(parentWnd);
      if (parentWindow) {
        
        
        
        
        
        if (mInWheelProcessing) {
          destWnd = parentWnd;
          destWindow = parentWindow;
        } else {
          
          
          
          
          mInWheelProcessing = PR_TRUE;
          if (0 == ::SendMessageW(destWnd, msg, wParam, lParam)) {
            result = PR_TRUE; 
          }
          destWnd = nsnull;
          mInWheelProcessing = PR_FALSE;
        }
        return PR_FALSE; 
      }
      parentWnd = ::GetParent(parentWnd);
    } 
  }
  if (destWnd == nsnull)
    return PR_FALSE;
  if (destWnd != mWnd) {
    if (destWindow) {
      result = destWindow->ProcessMessage(msg, wParam, lParam, aRetValue);
      return PR_TRUE; 
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

  
  
  UpdateMouseWheelSeriesCounter();

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
        
        scrollEvent.delta = ComputeMouseWheelDelta(currentVDelta, 
                                                   iDeltaPerLine, 
                                                   ulScrollLines);
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

  if (!scrollEvent.delta)
    return PR_FALSE; 

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
  
  return PR_FALSE; 
} 


void nsWindow::OnMouseWheelTimeout(nsITimer* aTimer, void* aClosure) 
{
  nsWindow* window = (nsWindow*) aClosure;
  window->mScrollSeriesCounter = 0;
}


void nsWindow::UpdateMouseWheelSeriesCounter() 
{
  mScrollSeriesCounter++;

  int scrollSeriesTimeout = 80;
  static nsITimer* scrollTimer;
  if (!scrollTimer) {
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!timer)
      return;
    timer.swap(scrollTimer);
  }

  scrollTimer->Cancel();
  nsresult rv = 
    scrollTimer->InitWithFuncCallback(OnMouseWheelTimeout, this,
                                      scrollSeriesTimeout,
                                      nsITimer::TYPE_ONE_SHOT);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "nsITimer::InitWithFuncCallback failed");
}



int nsWindow::ComputeMouseWheelDelta(int currentVDelta,
                                     int iDeltaPerLine,
                                     ULONG ulScrollLines)
{
  
  int scrollAccelerationStart = gScrollPrefObserver->GetScrollAccelerationStart();
  
  int scrollNumLines = gScrollPrefObserver->GetScrollNumLines();
  
  int scrollAccelerationFactor = gScrollPrefObserver->GetScrollAccelerationFactor();

  
  int ulScrollLinesInt = static_cast<int>(ulScrollLines);
  
  int delta = scrollNumLines * currentVDelta / (iDeltaPerLine * ulScrollLinesInt);

  
  if (mScrollSeriesCounter < scrollAccelerationStart ||
      scrollAccelerationStart < 0 ||
      scrollAccelerationFactor < 0)
    return delta;
  else
    return int(0.5 + delta * mScrollSeriesCounter *
           (double) scrollAccelerationFactor / 10);
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
        PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
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

    PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
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
  } else {
    DispatchKeyEvent(NS_KEY_PRESS, 0, nsnull, DOMKeyCode, nsnull, aModKeyState,
                     extraFlags);
  }
#else
  {
    UINT unichar = ::MapVirtualKey(virtualKeyCode, MAPVK_VK_TO_CHAR);
    
    if (unichar & 0x80) {
      return noDefault;
    }
    DispatchKeyEvent(NS_KEY_PRESS, unichar, nsnull, DOMKeyCode, nsnull, aModKeyState,
                     extraFlags);
  }
#endif

  return noDefault;
}


LRESULT nsWindow::OnKeyUp(const MSG &aMsg,
                          nsModifierKeyState &aModKeyState,
                          PRBool *aEventDispatched)
{
  UINT virtualKeyCode = aMsg.wParam;

  PR_LOG(gWindowsLog, PR_LOG_ALWAYS,
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

void
nsWindow::SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray, PRUint32 aModifiers)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sModifierKeyMap); ++i) {
    const PRUint32* map = sModifierKeyMap[i];
    if (aModifiers & map[0]) {
      aArray->AppendElement(KeyPair(map[1], map[2]));
    }
  }
}

nsresult
nsWindow::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
  
  
  
  for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
    const Configuration& configuration = aConfigurations[i];
    nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
    NS_ASSERTION(w->GetParent() == this,
                 "Configured widget is not a child");
#ifdef WINCE
    
    
    
    ::SetWindowRgn(w->mWnd, NULL, TRUE);
#endif
    nsIntRect bounds;
    w->GetBounds(bounds);
    if (bounds.Size() != configuration.mBounds.Size()) {
      w->Resize(configuration.mBounds.x, configuration.mBounds.y,
                configuration.mBounds.width, configuration.mBounds.height,
                PR_TRUE);
    } else if (bounds.TopLeft() != configuration.mBounds.TopLeft()) {
      w->Move(configuration.mBounds.x, configuration.mBounds.y);
    }
    nsresult rv = w->SetWindowClipRegion(configuration.mClipRegion, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

static HRGN
CreateHRGNFromArray(const nsTArray<nsIntRect>& aRects)
{
  PRInt32 size = sizeof(RGNDATAHEADER) + sizeof(RECT)*aRects.Length();
  nsAutoTArray<PRUint8,100> buf;
  if (!buf.SetLength(size))
    return NULL;
  RGNDATA* data = reinterpret_cast<RGNDATA*>(buf.Elements());
  RECT* rects = reinterpret_cast<RECT*>(data->Buffer);
  data->rdh.dwSize = sizeof(data->rdh);
  data->rdh.iType = RDH_RECTANGLES;
  data->rdh.nCount = aRects.Length();
  nsIntRect bounds;
  for (PRUint32 i = 0; i < aRects.Length(); ++i) {
    const nsIntRect& r = aRects[i];
    bounds.UnionRect(bounds, r);
    ::SetRect(&rects[i], r.x, r.y, r.XMost(), r.YMost());
  }
  ::SetRect(&data->rdh.rcBound, bounds.x, bounds.y, bounds.XMost(), bounds.YMost());
  return ::ExtCreateRegion(NULL, buf.Length(), data);
}

nsresult
nsWindow::SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                              PRBool aIntersectWithExisting)
{
  if (!aIntersectWithExisting) {
    if (!StoreWindowClipRegion(aRects))
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

  if (!::SetWindowRgn(mWnd, dest, TRUE)) {
    ::DeleteObject(dest);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


void nsWindow::OnDestroy()
{
  mOnDestroyCalled = PR_TRUE;

  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  
  
  if (!mInDtor)
    DispatchStandardEvent(NS_DESTROY);

  
  mEventCallback = nsnull;

  
  
  SubclassWindow(FALSE);

  
  
  if (sCurrentWindow == this)
    sCurrentWindow = nsnull;

  
  nsBaseWidget::Destroy();

  
  nsBaseWidget::OnDestroy();
  
  
  
  
  

  
  EnableDragDrop(PR_FALSE);

  
  
  if ( this == sRollupWidget ) {
    if ( sRollupListener )
      sRollupListener->Rollup(nsnull, nsnull);
    CaptureRollupEvents(nsnull, PR_FALSE, PR_TRUE);
  }

  
  if (mOldIMC) {
    mOldIMC = ::ImmAssociateContext(mWnd, mOldIMC);
    NS_ASSERTION(!mOldIMC, "Another IMC was associated");
  }

  
  MouseTrailer* mtrailer = nsToolkit::gMouseTrailer;
  if (mtrailer) {
    if (mtrailer->GetMouseTrailerWindow() == mWnd)
      mtrailer->DestroyTimer();

    if (mtrailer->GetCaptureWindow() == mWnd)
      mtrailer->SetCaptureWindow(nsnull);
  }

  
  if (mBrush) {
    VERIFY(::DeleteObject(mBrush));
    mBrush = NULL;
  }

  
  HICON icon;
  icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM) 0);
  if (icon)
    ::DestroyIcon(icon);

  icon = (HICON) ::SendMessageW(mWnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM) 0);
  if (icon)
    ::DestroyIcon(icon);

  
  if (mCursor == -1)
    SetCursor(eCursor_standard);

#ifdef MOZ_XUL
  
  if (eTransparencyTransparent == mTransparencyMode)
    SetupTranslucentWindowMemoryBitmap(eTransparencyOpaque);
#endif

  
  mWnd = NULL;
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

#if !defined(WINCE) 
PRBool nsWindow::OnHotKey(WPARAM wParam, LPARAM lParam)
{
  return PR_TRUE;
}
#endif 

void nsWindow::OnSettingsChange(WPARAM wParam, LPARAM lParam)
{
  nsWindowGfx::OnSettingsChangeGfx(wParam);
}


PRBool nsWindow::OnScroll(UINT scrollCode, int cPos)
{
  return PR_FALSE;
}


HBRUSH nsWindow::OnControlColor()
{
  return mBrush;
}


PRBool nsWindow::AutoErase()
{
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
  sSoftKeyboardState = (aState != nsIWidget::IME_STATUS_DISABLED);
  nsWindowCE::ToggleSoftKB(sSoftKeyboardState);
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

#ifdef ACCESSIBILITY
already_AddRefed<nsIAccessible> nsWindow::GetRootAccessible()
{
  nsWindow::sIsAccessibilityOn = TRUE;

  if (mInDtor || mOnDestroyCalled || mWindowType == eWindowType_invisible) {
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

STDMETHODIMP_(LRESULT)
nsWindow::LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN pAcc)
{
  
  if (!sAccLib)
    sAccLib =::LoadLibraryW(L"OLEACC.DLL");

  if (sAccLib) {
    if (!sLresultFromObject)
      sLresultFromObject = (LPFNLRESULTFROMOBJECT)GetProcAddress(sAccLib,"LresultFromObject");

    if (sLresultFromObject)
      return sLresultFromObject(riid,wParam,pAcc);
  }

  return 0;
}
#endif











#ifdef MOZ_XUL

void nsWindow::ResizeTranslucentWindow(PRInt32 aNewWidth, PRInt32 aNewHeight, PRBool force)
{
  if (!force && aNewWidth == mBounds.width && aNewHeight == mBounds.height)
    return;

  mTransparentSurface = new gfxWindowsSurface(gfxIntSize(aNewWidth, aNewHeight), gfxASurface::ImageFormatARGB32);
  mMemoryDC = mTransparentSurface->GetDC();
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











#ifndef WINCE

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

  if (sProcessHook && code == MSGF_MENU) {
    MSG* pMsg = (MSG*)lParam;
    ScheduleHookTimer( pMsg->hwnd, pMsg->message);
  }

  return ::CallNextHookEx(sMsgFilterHook, code, wParam, lParam);
}



LRESULT CALLBACK nsWindow::MozSpecialMouseProc(int code, WPARAM wParam, LPARAM lParam)
{
  if (sProcessHook) {
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
      printf("***** SetWindowsHookEx is NOT installed for WH_MSGFILTER!\n");
    }
#endif
  }

  
  if (!sCallProcHook) {
    DISPLAY_NMM_PRT("***** Hooking sCallProcHook!\n");
    sCallProcHook  = SetWindowsHookEx(WH_CALLWNDPROC, MozSpecialWndProc, NULL, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sCallProcHook) {
      printf("***** SetWindowsHookEx is NOT installed for WH_CALLWNDPROC!\n");
    }
#endif
  }

  
  if (!sCallMouseHook) {
    DISPLAY_NMM_PRT("***** Hooking sCallMouseHook!\n");
    sCallMouseHook  = SetWindowsHookEx(WH_MOUSE, MozSpecialMouseProc, NULL, GetCurrentThreadId());
#ifdef POPUP_ROLLUP_DEBUG_OUTPUT
    if (!sCallMouseHook) {
      printf("***** SetWindowsHookEx is NOT installed for WH_MOUSE!\n");
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
#endif 

static PRBool IsDifferentThreadWindow(HWND aWnd)
{
  return ::GetCurrentThreadId() != ::GetWindowThreadProcessId(aWnd, NULL);
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


BOOL
nsWindow::DealWithPopups(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam, LRESULT* outResult)
{
  if (sRollupListener && sRollupWidget && ::IsWindowVisible(inWnd)) {

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
      
      PRBool rollup = !nsWindow::EventIsInsideWindow(inMsg, (nsWindow*)sRollupWidget);

      if (rollup && (inMsg == WM_MOUSEWHEEL || inMsg == WM_MOUSEHWHEEL))
      {
        sRollupListener->ShouldRollupOnMouseWheelEvent(&rollup);
        *outResult = PR_TRUE;
      }

      
      
      PRUint32 popupsToRollup = PR_UINT32_MAX;
      if (rollup) {
        nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(sRollupListener) );
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
            
            
            sRollupListener->ShouldRollupOnMouseActivate(&rollup);
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
        
        
        PRBool consumeRollupEvent = sRollupConsumeEvent;
        
        sRollupListener->Rollup(popupsToRollup, inMsg == WM_LBUTTONDOWN ? &mLastRollup : nsnull);

        
        sProcessHook = PR_FALSE;
        sRollupMsgId = 0;
        sRollupMsgWnd = NULL;

        
        
        
        
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












nsModifierKeyState::nsModifierKeyState()
{
  mIsShiftDown   = IS_VK_DOWN(NS_VK_SHIFT);
  mIsControlDown = IS_VK_DOWN(NS_VK_CONTROL);
  mIsAltDown     = IS_VK_DOWN(NS_VK_ALT);
}


PRInt32 nsWindow::GetWindowsVersion()
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





HWND nsWindow::GetTopLevelHWND(HWND aWnd, PRBool aStopOnDialogOrPopup)
{
  HWND curWnd = aWnd;
  HWND topWnd = NULL;
  HWND upWnd = NULL;

  while (curWnd) {
    topWnd = curWnd;

    if (aStopOnDialogOrPopup) {
      DWORD_PTR style = ::GetWindowLongPtrW(curWnd, GWL_STYLE);

      VERIFY_WINDOW_STYLE(style);

      if (!(style & WS_CHILD)) 
        break;
    }

    upWnd = ::GetParent(curWnd); 

#ifdef WINCE
    
    
    
    if (upWnd && ::GetWindow(curWnd, GW_OWNER) == upWnd) {
      DWORD_PTR style = ::GetWindowLongPtrW(curWnd, GWL_STYLE);
      if ((style & WS_DLGFRAME) != 0)
        break;
    }
#endif

    curWnd = upWnd;
  }

  return topWnd;
}

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

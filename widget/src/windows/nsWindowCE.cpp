








































#include "nsWindowCE.h"
#include "nsIObserverService.h"
#include "resource.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

















#if defined(WINCE_HAVE_SOFTKB)
PRBool          nsWindow::sSoftKeyMenuBar         = PR_FALSE;
PRBool          nsWindow::sSoftKeyboardState      = PR_FALSE;
PRBool          nsWindowCE::sSIPInTransition      = PR_FALSE;
TriStateBool    nsWindowCE::sShowSIPButton        = TRI_UNKNOWN;
#endif











#ifdef WINCE_HAVE_SOFTKB
void nsWindowCE::NotifySoftKbObservers(LPRECT visRect)
{
  if (!visRect) {
    SIPINFO sipInfo;
    memset(&sipInfo, 0, sizeof(SIPINFO));
    sipInfo.cbSize = sizeof(SIPINFO);
    if (SipGetInfo(&sipInfo)) 
      visRect = &(sipInfo.rcVisibleDesktop);
    else
      return;
  }
  
  
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  if (observerService) {
    wchar_t rectBuf[256];
    _snwprintf(rectBuf, 256, L"{\"left\": %d, \"top\": %d,"
               L" \"right\": %d, \"bottom\": %d}", 
               visRect->left, visRect->top, visRect->right, visRect->bottom);
    observerService->NotifyObservers(nsnull, "softkb-change", rectBuf);
  }
}

void nsWindowCE::ToggleSoftKB(PRBool show)
{
  sSIPInTransition = PR_TRUE;
  HWND hWndSIP = FindWindowW(L"SipWndClass", NULL );
  if (hWndSIP)
    ShowWindow(hWndSIP, show ? SW_SHOW: SW_HIDE);

  HWND hWndSIPB = FindWindowW(L"MS_SIPBUTTON", NULL ); 
  if (hWndSIPB)
      ShowWindow(hWndSIPB, show ? SW_SHOW: SW_HIDE);

  SipShowIM(show ? SIPF_ON : SIPF_OFF);

  if (sShowSIPButton == TRI_UNKNOWN) {
    PRBool tmpBool = PR_FALSE;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
      nsCOMPtr<nsIPrefBranch> prefBranch;
      prefs->GetBranch(0, getter_AddRefs(prefBranch));
      if (prefBranch) {
        nsresult rv = prefBranch->GetBoolPref("ui.sip.showSIPButton", &tmpBool);
        if (NS_SUCCEEDED(rv))
          sShowSIPButton = tmpBool ? TRI_TRUE : TRI_FALSE;
      }
    }
  }
  if (sShowSIPButton != TRI_TRUE && hWndSIPB && hWndSIP) {
    ShowWindow(hWndSIPB, SW_HIDE);
    int sX = GetSystemMetrics(SM_CXSCREEN);
    int sY = GetSystemMetrics(SM_CYSCREEN);
    RECT sipRect;
    GetWindowRect(hWndSIP, &sipRect);
    int sipH = sipRect.bottom - sipRect.top;
    int sipW = sipRect.right - sipRect.left;
    sipRect.left = (sX - sipW)/2;
    sipRect.top =  sY - sipH;
    sipRect.bottom = sY;
    sipRect.right = sipRect.left + sipW;
    MoveWindow(hWndSIP, (sX - sipW)/2, sY - sipH, sipW, sipH, TRUE);
    SIPINFO sipInfo;
    RECT visRect;
    visRect.top = 0;
    visRect.left = 0;
    visRect.bottom = show ? sipRect.top : sY;
    visRect.right = sX;
    sipInfo.cbSize = sizeof(SIPINFO);
    sipInfo.fdwFlags = SIPF_DOCKED | SIPF_LOCKED | (show ? SIPF_ON : SIPF_OFF);
    sipInfo.rcSipRect = sipRect;
    sipInfo.rcVisibleDesktop = visRect;
    sipInfo.dwImDataSize = 0;
    sipInfo.pvImData = NULL;
    SipSetInfo(&sipInfo);
    NotifySoftKbObservers(&visRect);
  } else {
    NotifySoftKbObservers();
  }
  sSIPInTransition = PR_FALSE;
}

void nsWindowCE::CreateSoftKeyMenuBar(HWND wnd)
{
  if (!wnd)
    return;
  
  static HWND sSoftKeyMenuBar = nsnull;
  
  if (sSoftKeyMenuBar != nsnull)
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
  
  sSoftKeyMenuBar = mbi.hwndMB;
}
#endif  

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

BOOL nsWindowCE::EnumChildWindows(HWND inParent, WNDENUMPROC inFunc, LPARAM inParam)
{
  ECWWindows myParams;
  myParams.params = inParam;
  myParams.func   = inFunc;
  myParams.parent = inParent;
  
  return EnumWindows(MyEnumWindowsProc, (LPARAM) &myParams);
}





















DWORD nsWindow::WindowStyle()
{
  DWORD style;

  










  



  

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
      style = WS_POPUP;
      break;

    default:
      NS_ERROR("unknown border style");
      

    case eWindowType_toplevel:
    case eWindowType_invisible:
      style = WS_BORDER;
#if !defined(WINCE_WINDOWS_MOBILE)
      style |= WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
#endif
      break;
  }

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










NS_IMETHODIMP nsWindow::SetSizeMode(PRInt32 aMode)
{
#if defined(WINCE_HAVE_SOFTKB)
  if (aMode == 0 && mSizeMode == 3  && nsWindowCE::sSIPInTransition) {
      
    return NS_OK;
  }
#endif
  nsresult rv;

  
  
  
  if (aMode == mSizeMode)
    return NS_OK;

#ifdef WINCE_WINDOWS_MOBILE
  
  
  if (mWindowType == eWindowType_dialog || mWindowType == eWindowType_toplevel) {
    if (aMode == nsSizeMode_Normal)
      aMode = nsSizeMode_Maximized;
  }

  
  if (aMode == nsSizeMode_Minimized)
    return NS_OK;
#endif

  
  rv = nsBaseWidget::SetSizeMode(aMode);
  if (NS_SUCCEEDED(rv) && mIsVisible) {
    int mode;

    switch (aMode) {
      case nsSizeMode_Fullscreen :
      case nsSizeMode_Maximized :
        mode = SW_MAXIMIZE;
        break;
      default :
        mode = SW_RESTORE;
    }
    ::ShowWindow(mWnd, mode);
  }
  return rv;
}









NS_METHOD nsWindow::EnableDragDrop(PRBool aEnable)
{
  return NS_ERROR_FAILURE;
}






















PRBool nsWindow::OnHotKey(WPARAM wParam, LPARAM lParam)
{
  
  
  
  
  
  
  
  if (VK_TSOFT1 == HIWORD(lParam) && (0 != (MOD_KEYUP & LOWORD(lParam))))
  {
    keybd_event(VK_F19, 0, 0, 0);
    keybd_event(VK_F19, 0, KEYEVENTF_KEYUP, 0);
    return PR_FALSE;
  }
  
  if (VK_TSOFT2 == HIWORD(lParam) && (0 != (MOD_KEYUP & LOWORD(lParam))))
  {
    keybd_event(VK_F20, 0, 0, 0);
    keybd_event(VK_F20, 0, KEYEVENTF_KEYUP, 0);
    return PR_FALSE;
  }
  
  if (VK_TBACK == HIWORD(lParam) && (0 != (MOD_KEYUP & LOWORD(lParam))))
  {
    keybd_event(VK_BACK, 0, 0, 0);
    keybd_event(VK_BACK, 0, KEYEVENTF_KEYUP, 0);
    return PR_FALSE;
  }

  switch (wParam) 
  {
    case VK_APP1:
      keybd_event(VK_F1, 0, 0, 0);
      keybd_event(VK_F1, 0, KEYEVENTF_KEYUP, 0);
      break;

    case VK_APP2:
      keybd_event(VK_F2, 0, 0, 0);
      keybd_event(VK_F2, 0, KEYEVENTF_KEYUP, 0);
      break;

    case VK_APP3:
      keybd_event(VK_F3, 0, 0, 0);
      keybd_event(VK_F3, 0, KEYEVENTF_KEYUP, 0);
      break;

    case VK_APP4:
      keybd_event(VK_F4, 0, 0, 0);
      keybd_event(VK_F4, 0, KEYEVENTF_KEYUP, 0);
      break;

    case VK_APP5:
      keybd_event(VK_F5, 0, 0, 0);
      keybd_event(VK_F5, 0, KEYEVENTF_KEYUP, 0);
      break;

    case VK_APP6:
      keybd_event(VK_F6, 0, 0, 0);
      keybd_event(VK_F6, 0, KEYEVENTF_KEYUP, 0);
      break;
  }
  return PR_FALSE;
}

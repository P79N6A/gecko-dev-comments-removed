
























































#include "WinUtils.h"
#include "nsWindow.h"
#include "nsWindowDefs.h"
#include "nsGUIEvent.h"
#include "nsIDOMMouseEvent.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace widget {


WinUtils::SHCreateItemFromParsingNamePtr WinUtils::sCreateItemFromParsingName = nsnull;

 
WinUtils::WinVersion
WinUtils::GetWindowsVersion()
{
  static PRInt32 version = 0;

  if (version) {
    return static_cast<WinVersion>(version);
  }

  OSVERSIONINFOEX osInfo;
  osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  
  ::GetVersionEx((OSVERSIONINFO*)&osInfo);
  version =
    (osInfo.dwMajorVersion & 0xff) << 8 | (osInfo.dwMinorVersion & 0xff);
  return static_cast<WinVersion>(version);
}


bool
WinUtils::GetRegistryKey(HKEY aRoot,
                         const PRUnichar* aKeyName,
                         const PRUnichar* aValueName,
                         PRUnichar* aBuffer,
                         DWORD aBufferLength)
{
  NS_PRECONDITION(aKeyName, "The key name is NULL");

  HKEY key;
  LONG result =
    ::RegOpenKeyExW(aRoot, aKeyName, NULL, KEY_READ | KEY_WOW64_32KEY, &key);
  if (result != ERROR_SUCCESS) {
    result =
      ::RegOpenKeyExW(aRoot, aKeyName, NULL, KEY_READ | KEY_WOW64_64KEY, &key);
    if (result != ERROR_SUCCESS) {
      return false;
    }
  }

  DWORD type;
  result =
    ::RegQueryValueExW(key, aValueName, NULL, &type, (BYTE*) aBuffer,
                       &aBufferLength);
  ::RegCloseKey(key);
  if (result != ERROR_SUCCESS || type != REG_SZ) {
    return false;
  }
  if (aBuffer) {
    aBuffer[aBufferLength / sizeof(*aBuffer) - 1] = 0;
  }
  return true;
}


HWND
WinUtils::GetTopLevelHWND(HWND aWnd,
                          bool aStopIfNotChild,
                          bool aStopIfNotPopup)
{
  HWND curWnd = aWnd;
  HWND topWnd = NULL;

  while (curWnd) {
    topWnd = curWnd;

    if (aStopIfNotChild) {
      DWORD_PTR style = ::GetWindowLongPtrW(curWnd, GWL_STYLE);

      VERIFY_WINDOW_STYLE(style);

      if (!(style & WS_CHILD)) 
        break;
    }

    HWND upWnd = ::GetParent(curWnd); 

    
    
    if (!upWnd && !aStopIfNotPopup) {
      upWnd = ::GetWindow(curWnd, GW_OWNER);
    }
    curWnd = upWnd;
  }

  return topWnd;
}

static PRUnichar*
GetNSWindowPropName()
{
  static PRUnichar sPropName[40] = L"";
  if (!*sPropName) {
    _snwprintf(sPropName, 39, L"MozillansIWidgetPtr%p",
               ::GetCurrentProcessId());
    sPropName[39] = '\0';
  }
  return sPropName;
}


bool
WinUtils::SetNSWindowPtr(HWND aWnd, nsWindow* aWindow)
{
  if (!aWindow) {
    ::RemovePropW(aWnd, GetNSWindowPropName());
    return true;
  }
  return ::SetPropW(aWnd, GetNSWindowPropName(), (HANDLE)aWindow);
}


nsWindow*
WinUtils::GetNSWindowPtr(HWND aWnd)
{
  return static_cast<nsWindow*>(::GetPropW(aWnd, GetNSWindowPropName()));
}

static BOOL CALLBACK
AddMonitor(HMONITOR, HDC, LPRECT, LPARAM aParam)
{
  (*(PRInt32*)aParam)++;
  return TRUE;
}


PRInt32
WinUtils::GetMonitorCount()
{
  PRInt32 monitorCount = 0;
  EnumDisplayMonitors(NULL, NULL, AddMonitor, (LPARAM)&monitorCount);
  return monitorCount;
}


bool
WinUtils::IsOurProcessWindow(HWND aWnd)
{
  if (!aWnd) {
    return false;
  }
  DWORD processId = 0;
  ::GetWindowThreadProcessId(aWnd, &processId);
  return (processId == ::GetCurrentProcessId());
}


HWND
WinUtils::FindOurProcessWindow(HWND aWnd)
{
  for (HWND wnd = ::GetParent(aWnd); wnd; wnd = ::GetParent(wnd)) {
    if (IsOurProcessWindow(wnd)) {
      return wnd;
    }
  }
  return NULL;
}

static bool
IsPointInWindow(HWND aWnd, const POINT& aPointInScreen)
{
  RECT bounds;
  if (!::GetWindowRect(aWnd, &bounds)) {
    return false;
  }

  return (aPointInScreen.x >= bounds.left && aPointInScreen.x < bounds.right &&
          aPointInScreen.y >= bounds.top && aPointInScreen.y < bounds.bottom);
}






static HWND
FindTopmostWindowAtPoint(HWND aWnd, const POINT& aPointInScreen)
{
  if (!::IsWindowVisible(aWnd) || !IsPointInWindow(aWnd, aPointInScreen)) {
    return NULL;
  }

  HWND childWnd = ::GetTopWindow(aWnd);
  while (childWnd) {
    HWND topmostWnd = FindTopmostWindowAtPoint(childWnd, aPointInScreen);
    if (topmostWnd) {
      return topmostWnd;
    }
    childWnd = ::GetNextWindow(childWnd, GW_HWNDNEXT);
  }

  return aWnd;
}

struct FindOurWindowAtPointInfo
{
  POINT mInPointInScreen;
  HWND mOutWnd;
};

static BOOL CALLBACK
FindOurWindowAtPointCallback(HWND aWnd, LPARAM aLPARAM)
{
  if (!WinUtils::IsOurProcessWindow(aWnd)) {
    
    return TRUE;
  }

  
  
  
  
  FindOurWindowAtPointInfo* info =
    reinterpret_cast<FindOurWindowAtPointInfo*>(aLPARAM);
  HWND childWnd = FindTopmostWindowAtPoint(aWnd, info->mInPointInScreen);
  if (!childWnd) {
    
    return TRUE;
  }

  
  info->mOutWnd = childWnd;
  return FALSE;
}


HWND
WinUtils::FindOurWindowAtPoint(const POINT& aPointInScreen)
{
  FindOurWindowAtPointInfo info;
  info.mInPointInScreen = aPointInScreen;
  info.mOutWnd = NULL;

  
  EnumWindows(FindOurWindowAtPointCallback, reinterpret_cast<LPARAM>(&info));
  return info.mOutWnd;
}


UINT
WinUtils::GetInternalMessage(UINT aNativeMessage)
{
  switch (aNativeMessage) {
    case WM_MOUSEWHEEL:
      return MOZ_WM_MOUSEVWHEEL;
    case WM_MOUSEHWHEEL:
      return MOZ_WM_MOUSEHWHEEL;
    case WM_VSCROLL:
      return MOZ_WM_VSCROLL;
    case WM_HSCROLL:
      return MOZ_WM_HSCROLL;
    default:
      return aNativeMessage;
  }
}


UINT
WinUtils::GetNativeMessage(UINT aInternalMessage)
{
  switch (aInternalMessage) {
    case MOZ_WM_MOUSEVWHEEL:
      return WM_MOUSEWHEEL;
    case MOZ_WM_MOUSEHWHEEL:
      return WM_MOUSEHWHEEL;
    case MOZ_WM_VSCROLL:
      return WM_VSCROLL;
    case MOZ_WM_HSCROLL:
      return WM_HSCROLL;
    default:
      return aInternalMessage;
  }
}


PRUint16
WinUtils::GetMouseInputSource()
{
  PRInt32 inputSource = nsIDOMMouseEvent::MOZ_SOURCE_MOUSE;
  LPARAM lParamExtraInfo = ::GetMessageExtraInfo();
  if ((lParamExtraInfo & TABLET_INK_SIGNATURE) == TABLET_INK_CHECK) {
    inputSource = (lParamExtraInfo & TABLET_INK_TOUCH) ?
      nsIDOMMouseEvent::MOZ_SOURCE_TOUCH : nsIDOMMouseEvent::MOZ_SOURCE_PEN;
  }
  return static_cast<PRUint16>(inputSource);
}


MSG
WinUtils::InitMSG(UINT aMessage, WPARAM wParam, LPARAM lParam)
{
  MSG msg;
  msg.message = aMessage;
  msg.wParam  = wParam;
  msg.lParam  = lParam;
  return msg;
}


bool
WinUtils::VistaCreateItemFromParsingNameInit()
{
  
  if (sCreateItemFromParsingName) {
    return true;
  }
  static HMODULE sShellDll = nsnull;
  if (sShellDll) {
    return false;
  }
  static const PRUnichar kSehllLibraryName[] =  L"shell32.dll";
  sShellDll = ::LoadLibraryW(kSehllLibraryName);
  if (!sShellDll) {
    return false;
  }
  sCreateItemFromParsingName = (SHCreateItemFromParsingNamePtr)
    GetProcAddress(sShellDll, "SHCreateItemFromParsingName");
  return sCreateItemFromParsingName != nsnull;
}


HRESULT
WinUtils::SHCreateItemFromParsingName(PCWSTR pszPath, IBindCtx *pbc,
                                      REFIID riid, void **ppv)
{
  NS_ENSURE_TRUE(sCreateItemFromParsingName, E_FAIL);
  return sCreateItemFromParsingName(pszPath, pbc, riid, ppv);
}


} 
} 

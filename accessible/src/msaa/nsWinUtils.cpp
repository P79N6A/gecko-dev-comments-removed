







































#include "nsWinUtils.h"

#include "nsIWinAccessNode.h"
#include "nsRootAccessible.h"

#include "nsArrayUtils.h"
#include "nsIDocShellTreeItem.h"



const PRUnichar* kPropNameTabContent = L"AccessibleTabWindow";

HRESULT
nsWinUtils::ConvertToIA2Array(nsIArray *aGeckoArray, IUnknown ***aIA2Array,
                              long *aIA2ArrayLen)
{
  *aIA2Array = NULL;
  *aIA2ArrayLen = 0;

  if (!aGeckoArray)
    return S_FALSE;

  PRUint32 length = 0;
  nsresult rv = aGeckoArray->GetLength(&length);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (length == 0)
    return S_FALSE;

  *aIA2Array =
    static_cast<IUnknown**>(::CoTaskMemAlloc((length) * sizeof(IUnknown*)));
  if (!*aIA2Array)
    return E_OUTOFMEMORY;

  PRUint32 idx = 0;
  for (; idx < length; ++idx) {
    nsCOMPtr<nsIWinAccessNode> winAccessNode =
      do_QueryElementAt(aGeckoArray, idx, &rv);
    if (NS_FAILED(rv))
      break;

    void *instancePtr = NULL;
    nsresult rv = winAccessNode->QueryNativeInterface(IID_IUnknown,
                                                      &instancePtr);
    if (NS_FAILED(rv))
      break;

    (*aIA2Array)[idx] = static_cast<IUnknown*>(instancePtr);
  }

  if (NS_FAILED(rv)) {
    for (PRUint32 idx2 = 0; idx2 < idx; idx2++) {
      (*aIA2Array)[idx2]->Release();
      (*aIA2Array)[idx2] = NULL;
    }

    ::CoTaskMemFree(*aIA2Array);
    return GetHRESULT(rv);
  }

  *aIA2ArrayLen = length;
  return S_OK;
}

bool
nsWinUtils::MaybeStartWindowEmulation()
{
  
  
  if (IsWindowEmulationFor(0)) {
    RegisterNativeWindow(kClassNameTabContent);
    nsAccessNodeWrap::sHWNDCache.Init(4);
    return true;
  }
  return false;
}

void
nsWinUtils::ShutdownWindowEmulation()
{
  
  
  if (IsWindowEmulationFor(0))
    ::UnregisterClassW(kClassNameTabContent, GetModuleHandle(NULL));
}

bool
nsWinUtils::IsWindowEmulationStarted()
{
  return nsAccessNodeWrap::sHWNDCache.IsInitialized();
}

void
nsWinUtils::RegisterNativeWindow(LPCWSTR aWindowClass)
{
  WNDCLASSW wc;
  wc.style = CS_GLOBALCLASS;
  wc.lpfnWndProc = nsAccessNodeWrap::WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hIcon = NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = aWindowClass;
  ::RegisterClassW(&wc);
}

HWND
nsWinUtils::CreateNativeWindow(LPCWSTR aWindowClass, HWND aParentWnd,
                               int aX, int aY, int aWidth, int aHeight,
                               bool aIsActive)
{
  HWND hwnd = ::CreateWindowExW(WS_EX_TRANSPARENT, aWindowClass,
                                L"NetscapeDispatchWnd",
                                WS_CHILD | (aIsActive ? WS_VISIBLE : 0),
                                aX, aY, aWidth, aHeight,
                                aParentWnd,
                                NULL,
                                GetModuleHandle(NULL),
                                NULL);
  if (hwnd) {
    
    ::SetPropW(hwnd, kPropNameTabContent, (HANDLE)1);
  }
  return hwnd;
}

void
nsWinUtils::ShowNativeWindow(HWND aWnd)
{
  ::ShowWindow(aWnd, SW_SHOW);
}

void
nsWinUtils::HideNativeWindow(HWND aWnd)
{
  ::SetWindowPos(aWnd, NULL, 0, 0, 0, 0,
                 SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE |
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

bool
nsWinUtils::IsWindowEmulationFor(LPCWSTR kModuleHandle)
{
#ifdef MOZ_E10S_COMPAT
  
  return kModuleHandle ? ::GetModuleHandleW(kModuleHandle) : true;
#else
  return kModuleHandle ? ::GetModuleHandleW(kModuleHandle) :
    ::GetModuleHandleW(kJAWSModuleHandle) ||
    ::GetModuleHandleW(kWEModuleHandle)  ||
    ::GetModuleHandleW(kDolphinModuleHandle);
#endif
}

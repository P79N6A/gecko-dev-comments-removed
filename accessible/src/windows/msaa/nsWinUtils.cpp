






#include "nsWinUtils.h"

#include "Compatibility.h"
#include "DocAccessible.h"
#include "nsCoreUtils.h"

#include "mozilla/Preferences.h"
#include "nsArrayUtils.h"
#include "nsIArray.h"
#include "nsIDocument.h"
#include "nsIDocShellTreeItem.h"

using namespace mozilla;
using namespace mozilla::a11y;



const PRUnichar* kPropNameTabContent = L"AccessibleTabWindow";

already_AddRefed<nsIDOMCSSStyleDeclaration>
nsWinUtils::GetComputedStyleDeclaration(nsIContent* aContent)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aContent);
  if (!elm)
    return nullptr;

  
  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(elm->OwnerDoc()->GetWindow());
  if (!window)
    return nullptr;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(elm));
  window->GetComputedStyle(domElement, EmptyString(), getter_AddRefs(cssDecl));
  return cssDecl.forget();
}

bool
nsWinUtils::MaybeStartWindowEmulation()
{
  
  
  if (Compatibility::IsJAWS() || Compatibility::IsWE() ||
      Compatibility::IsDolphin() ||
      Preferences::GetBool("browser.tabs.remote")) {
    RegisterNativeWindow(kClassNameTabContent);
    nsAccessNodeWrap::sHWNDCache.Init(4);
    return true;
  }

  return false;
}

void
nsWinUtils::ShutdownWindowEmulation()
{
  
  
  if (IsWindowEmulationStarted())
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

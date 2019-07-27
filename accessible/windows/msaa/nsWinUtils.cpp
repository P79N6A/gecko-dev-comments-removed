






#include "nsWinUtils.h"

#include "Compatibility.h"
#include "DocAccessible.h"
#include "nsAccessibilityService.h"
#include "nsCoreUtils.h"

#include "mozilla/Preferences.h"
#include "nsArrayUtils.h"
#include "nsIArray.h"
#include "nsIDocument.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMElement.h"
#include "nsXULAppAPI.h"

using namespace mozilla;
using namespace mozilla::a11y;



const wchar_t* kPropNameTabContent = L"AccessibleTabWindow";




static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam);

nsRefPtrHashtable<nsPtrHashKey<void>, DocAccessible>* nsWinUtils::sHWNDCache = nullptr;

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
  
  
  if (IPCAccessibilityActive())
    return false;

  if (Compatibility::IsJAWS() || Compatibility::IsWE() ||
      Compatibility::IsDolphin() ||
      XRE_GetProcessType() == GeckoProcessType_Content) {
    RegisterNativeWindow(kClassNameTabContent);
    sHWNDCache = new nsRefPtrHashtable<nsPtrHashKey<void>, DocAccessible>(2);
    return true;
  }

  return false;
}

void
nsWinUtils::ShutdownWindowEmulation()
{
  
  
  if (IsWindowEmulationStarted())
    ::UnregisterClassW(kClassNameTabContent, GetModuleHandle(nullptr));
}

bool
nsWinUtils::IsWindowEmulationStarted()
{
  return sHWNDCache != nullptr;
}

void
nsWinUtils::RegisterNativeWindow(LPCWSTR aWindowClass)
{
  WNDCLASSW wc;
  wc.style = CS_GLOBALCLASS;
  wc.lpfnWndProc = WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hIcon = nullptr;
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
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
                                nullptr,
                                GetModuleHandle(nullptr),
                                nullptr);
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
  ::SetWindowPos(aWnd, nullptr, 0, 0, 0, 0,
                 SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE |
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK
WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  
  
  

  switch (msg) {
    case WM_GETOBJECT:
    {
      if (lParam == OBJID_CLIENT) {
        DocAccessible* document =
          nsWinUtils::sHWNDCache->GetWeak(static_cast<void*>(hWnd));
        if (document) {
          IAccessible* msaaAccessible = nullptr;
          document->GetNativeInterface((void**)&msaaAccessible); 
          if (msaaAccessible) {
            LRESULT result = ::LresultFromObject(IID_IAccessible, wParam,
                                                 msaaAccessible); 
            msaaAccessible->Release(); 
            return result;
          }
        }
      }
      return 0;
    }
    case WM_NCHITTEST:
    {
      LRESULT lRet = ::DefWindowProc(hWnd, msg, wParam, lParam);
      if (HTCLIENT == lRet)
        lRet = HTTRANSPARENT;
      return lRet;
    }
  }

  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

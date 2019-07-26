




#pragma once

#include "nsDebug.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsPoint.h"
#include "nsRect.h"

#include "mozwrlbase.h"

#include <stdio.h>
#include <windows.foundation.h>
#include <windows.ui.viewmanagement.h>

void Log(const char *fmt, ...);
void LogW(const wchar_t *fmt, ...);

#define LogFunction() Log(__FUNCTION__)
#define LogThread() Log("%s: IsMainThread:%d ThreadId:%X", __FUNCTION__, NS_IsMainThread(), GetCurrentThreadId())
#define LogThis() Log("[%X] %s", this, __FUNCTION__)
#define LogException(e) Log("%s Exception:%s", __FUNCTION__, e->ToString()->Data())
#define LogHRESULT(hr) Log("%s hr=%X", __FUNCTION__, hr)


#ifdef DEBUG
#define DebugLogHR(hr) LogHRESULT(hr)
#else
#define DebugLogHR(hr)
#endif
#define AssertHRESULT(hr)           \
  if (FAILED(hr)) {                 \
    DebugLogHR(hr);                 \
    return;                         \
  }
#define AssertRetHRESULT(hr, res)   \
  if (FAILED(hr)) {                 \
    DebugLogHR(hr);                 \
    return res;                     \
  }


#define POINT_CEIL_X(position) (uint32_t)ceil(position.X)
#define POINT_CEIL_Y(position) (uint32_t)ceil(position.Y)

class nsIBrowserDOMWindow;
class nsIDOMWindow;

namespace mozilla {
namespace widget {
namespace winrt {

template<unsigned int size, typename T>
HRESULT ActivateGenericInstance(wchar_t const (&RuntimeClassName)[size], Microsoft::WRL::ComPtr<T>& aOutObject) {
  Microsoft::WRL::ComPtr<IActivationFactory> factory;
  HRESULT hr = ABI::Windows::Foundation::GetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClassName).Get(),
                                                              factory.GetAddressOf());
  if (FAILED(hr))
    return hr;
  Microsoft::WRL::ComPtr<IInspectable> inspect;
  hr = factory->ActivateInstance(inspect.GetAddressOf());
  if (FAILED(hr))
    return hr;
  return inspect.As(&aOutObject);
}

} } }

class MetroUtils
{
  typedef ABI::Windows::Foundation::IUriRuntimeClass IUriRuntimeClass;
  typedef Microsoft::WRL::Wrappers::HString HString;
  typedef ABI::Windows::UI::ViewManagement::ApplicationViewState ApplicationViewState;
  typedef ABI::Windows::Foundation::Point Point;
  typedef ABI::Windows::Foundation::Rect Rect;

public:
  
  
  
  static int32_t LogToPhys(FLOAT aValue);
  static nsIntPoint LogToPhys(const Point& aPt);
  static nsIntRect LogToPhys(const Rect& aRect);
  static FLOAT PhysToLog(int32_t aValue);
  static Point PhysToLog(const nsIntPoint& aPt);

  static nsresult FireObserver(const char* aMessage, const PRUnichar* aData = nullptr);

  static HRESULT CreateUri(HSTRING aUriStr, Microsoft::WRL::ComPtr<IUriRuntimeClass>& aUriOut);
  static HRESULT CreateUri(HString& aHString, Microsoft::WRL::ComPtr<IUriRuntimeClass>& aUriOut);
  static HRESULT GetViewState(ApplicationViewState& aState);
  static HRESULT TryUnsnap(bool* aResult = nullptr);
  static HRESULT ShowSettingsFlyout();

private:
  static nsresult GetBrowserDOMWindow(nsCOMPtr<nsIBrowserDOMWindow> &aBWin);
  static nsresult GetMostRecentWindow(const PRUnichar* aType, nsIDOMWindow** aWindow);
};

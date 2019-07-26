




#pragma once

#include <windows.system.h>
#include <windows.ui.core.h>
#include <UIAutomation.h>
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

#include "mozwrlbase.h"

#include "nsCOMPtr.h"
#include "mozilla/a11y/Accessible.h"
#include "UIAAccessibilityBridge.h"


#include "UIABridge.h"

namespace mozilla {
namespace widget {
namespace winrt {

using namespace Microsoft::WRL;


[uuid("D3EDD951-0715-4501-A8E5-25D97EF35D5A")]
class UIABridge : public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
  IUIABridge,
  IUIAElement,
  IRawElementProviderSimple,
  IRawElementProviderFragment,
  IRawElementProviderFragmentRoot>
{
  typedef ABI::Windows::UI::Core::ICoreWindow ICoreWindow;

  InspectableClass(L"IUIABridge", BaseTrust);

public:
  UIABridge() {}

  
  IFACEMETHODIMP Init(IInspectable* view, IInspectable* window, LONG_PTR inner);
  IFACEMETHODIMP Disconnect();
  IFACEMETHODIMP FocusChangeEvent();

  
  IFACEMETHODIMP SetFocusInternal(LONG_PTR aAccessible);
  IFACEMETHODIMP ClearFocus();

  
  IFACEMETHODIMP ElementProviderFromPoint(double x, double y, IRawElementProviderFragment ** retVal);
  IFACEMETHODIMP GetFocus(IRawElementProviderFragment ** retVal);

  
  IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment ** retVal);
  IFACEMETHODIMP GetRuntimeId(SAFEARRAY ** retVal);
  IFACEMETHODIMP get_BoundingRectangle(UiaRect * retVal);
  IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY ** retVal);
  IFACEMETHODIMP SetFocus();
  IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot * * retVal);

  
  IFACEMETHODIMP get_ProviderOptions(ProviderOptions * retVal);
  IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown * * retVal);
  IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT * retVal );
  IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple ** retVal);

protected:
  bool Connected();

private:
  Microsoft::WRL::ComPtr<ICoreWindow> mWindow;
#if defined(ACCESSIBILITY)
  nsRefPtr<AccessibilityBridge> mAccBridge;
  nsRefPtr<mozilla::a11y::Accessible> mAccessible;
#endif
};

[uuid("4438135F-F624-43DE-A417-275CE7A1A0CD")]
class UIATextElement : public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
  IUIAElement,
  IRawElementProviderSimple,
  IRawElementProviderFragment,
  ITextProvider,
  IValueProvider >
{
  typedef ABI::Windows::Foundation::Rect Rect;

  InspectableClass(L"UIATextElement", BaseTrust);

public:
  UIATextElement() {}

  
  IFACEMETHODIMP SetFocusInternal(LONG_PTR aAccessible);
  IFACEMETHODIMP ClearFocus();
  IFACEMETHODIMP FocusChangeEvent();

  
  IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment ** retVal);
  IFACEMETHODIMP GetRuntimeId(SAFEARRAY ** retVal);
  IFACEMETHODIMP get_BoundingRectangle(UiaRect * retVal);
  IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY ** retVal);
  IFACEMETHODIMP SetFocus();
  IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot * * retVal);

  
  IFACEMETHODIMP get_ProviderOptions(ProviderOptions * retVal);
  IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown * * retVal);
  IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT * retVal );
  IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple ** retVal);

  
  IFACEMETHODIMP GetSelection(SAFEARRAY * *pRetVal);
  IFACEMETHODIMP GetVisibleRanges(SAFEARRAY * *pRetVal);
  IFACEMETHODIMP RangeFromChild(IRawElementProviderSimple *childElement, ITextRangeProvider **pRetVal);
  IFACEMETHODIMP RangeFromPoint(UiaPoint point, ITextRangeProvider **pRetVal);
  IFACEMETHODIMP get_DocumentRange(ITextRangeProvider **pRetVal);
  IFACEMETHODIMP get_SupportedTextSelection(SupportedTextSelection *pRetVal);

  
  IFACEMETHODIMP SetValue(LPCWSTR val);
  IFACEMETHODIMP get_Value(BSTR *pRetVal);
  IFACEMETHODIMP get_IsReadOnly(BOOL *pRetVal);

  void SetIndexID(int id) {
    mIndexID = id;
  }

private:
  int mIndexID;
  nsCOMPtr<nsIAccessible> mAccessItem;
};

} } }

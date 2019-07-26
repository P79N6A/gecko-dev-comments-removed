




#include "mozilla/dom/BarProps.h"
#include "mozilla/dom/BarPropBinding.h"
#include "nsContentUtils.h"
#include "nsGlobalWindow.h"
#include "nsIDocShell.h"
#include "nsIScrollable.h"
#include "nsIWebBrowserChrome.h"

namespace mozilla {
namespace dom {




BarProp::BarProp(nsGlobalWindow* aWindow)
  : mDOMWindow(aWindow)
{
  MOZ_ASSERT(aWindow->IsInnerWindow());
  SetIsDOMBinding();
}

BarProp::~BarProp()
{
}

nsPIDOMWindow*
BarProp::GetParentObject() const
{
  return mDOMWindow;
}

JSObject*
BarProp::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return BarPropBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(BarProp, mDOMWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(BarProp)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BarProp)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BarProp)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

bool
BarProp::GetVisibleByFlag(uint32_t aChromeFlag, ErrorResult& aRv)
{
  nsCOMPtr<nsIWebBrowserChrome> browserChrome = GetBrowserChrome();
  NS_ENSURE_TRUE(browserChrome, false);

  uint32_t chromeFlags;

  if (NS_FAILED(browserChrome->GetChromeFlags(&chromeFlags))) {
    aRv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  return (chromeFlags & aChromeFlag);
}

void
BarProp::SetVisibleByFlag(bool aVisible, uint32_t aChromeFlag,
                          ErrorResult& aRv)
{
  nsCOMPtr<nsIWebBrowserChrome> browserChrome = GetBrowserChrome();
  NS_ENSURE_TRUE_VOID(browserChrome);

  if (!nsContentUtils::IsCallerChrome()) {
    return;
  }

  uint32_t chromeFlags;

  if (NS_FAILED(browserChrome->GetChromeFlags(&chromeFlags))) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (aVisible)
    chromeFlags |= aChromeFlag;
  else
    chromeFlags &= ~aChromeFlag;

  if (NS_FAILED(browserChrome->SetChromeFlags(chromeFlags))) {
    aRv.Throw(NS_ERROR_FAILURE);
  }
}

already_AddRefed<nsIWebBrowserChrome>
BarProp::GetBrowserChrome()
{
  if (!mDOMWindow) {
    return nullptr;
  }

  return mDOMWindow->GetWebBrowserChrome();
}





MenubarProp::MenubarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

MenubarProp::~MenubarProp()
{
}

bool
MenubarProp::GetVisible(ErrorResult& aRv)
{
  return BarProp::GetVisibleByFlag(nsIWebBrowserChrome::CHROME_MENUBAR, aRv);
}

void
MenubarProp::SetVisible(bool aVisible, ErrorResult& aRv)
{
  BarProp::SetVisibleByFlag(aVisible, nsIWebBrowserChrome::CHROME_MENUBAR, aRv);
}





ToolbarProp::ToolbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

ToolbarProp::~ToolbarProp()
{
}

bool
ToolbarProp::GetVisible(ErrorResult& aRv)
{
  return BarProp::GetVisibleByFlag(nsIWebBrowserChrome::CHROME_TOOLBAR, aRv);
}

void
ToolbarProp::SetVisible(bool aVisible, ErrorResult& aRv)
{
  BarProp::SetVisibleByFlag(aVisible, nsIWebBrowserChrome::CHROME_TOOLBAR,
                            aRv);
}





LocationbarProp::LocationbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

LocationbarProp::~LocationbarProp()
{
}

bool
LocationbarProp::GetVisible(ErrorResult& aRv)
{
  return BarProp::GetVisibleByFlag(nsIWebBrowserChrome::CHROME_LOCATIONBAR,
                                   aRv);
}

void
LocationbarProp::SetVisible(bool aVisible, ErrorResult& aRv)
{
  BarProp::SetVisibleByFlag(aVisible, nsIWebBrowserChrome::CHROME_LOCATIONBAR,
                            aRv);
}





PersonalbarProp::PersonalbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

PersonalbarProp::~PersonalbarProp()
{
}

bool
PersonalbarProp::GetVisible(ErrorResult& aRv)
{
  return BarProp::GetVisibleByFlag(nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR,
                                   aRv);
}

void
PersonalbarProp::SetVisible(bool aVisible, ErrorResult& aRv)
{
  BarProp::SetVisibleByFlag(aVisible,
                            nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR,
                            aRv);
}





StatusbarProp::StatusbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

StatusbarProp::~StatusbarProp()
{
}

bool
StatusbarProp::GetVisible(ErrorResult& aRv)
{
  return BarProp::GetVisibleByFlag(nsIWebBrowserChrome::CHROME_STATUSBAR, aRv);
}

void
StatusbarProp::SetVisible(bool aVisible, ErrorResult& aRv)
{
  return BarProp::SetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_STATUSBAR, aRv);
}





ScrollbarsProp::ScrollbarsProp(nsGlobalWindow *aWindow)
: BarProp(aWindow)
{
}

ScrollbarsProp::~ScrollbarsProp()
{
}

bool
ScrollbarsProp::GetVisible(ErrorResult& aRv)
{
  if (!mDOMWindow) {
    return true;
  }

  nsCOMPtr<nsIScrollable> scroller =
    do_QueryInterface(mDOMWindow->GetDocShell());

  if (!scroller) {
    return true;
  }

  int32_t prefValue;
  scroller->GetDefaultScrollbarPreferences(
              nsIScrollable::ScrollOrientation_Y, &prefValue);
  if (prefValue != nsIScrollable::Scrollbar_Never) {
    return true;
  }

  scroller->GetDefaultScrollbarPreferences(
              nsIScrollable::ScrollOrientation_X, &prefValue);
  return prefValue != nsIScrollable::Scrollbar_Never;
}

void
ScrollbarsProp::SetVisible(bool aVisible, ErrorResult& aRv)
{
  if (!nsContentUtils::IsCallerChrome()) {
    return;
  }

  






  nsCOMPtr<nsIScrollable> scroller =
    do_QueryInterface(mDOMWindow->GetDocShell());

  if (scroller) {
    int32_t prefValue;

    if (aVisible) {
      prefValue = nsIScrollable::Scrollbar_Auto;
    } else {
      prefValue = nsIScrollable::Scrollbar_Never;
    }

    scroller->SetDefaultScrollbarPreferences(
                nsIScrollable::ScrollOrientation_Y, prefValue);
    scroller->SetDefaultScrollbarPreferences(
                nsIScrollable::ScrollOrientation_X, prefValue);
  }

  












}

} 
} 


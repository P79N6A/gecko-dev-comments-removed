




#include "mozilla/dom/BarProps.h"

#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsGlobalWindow.h"
#include "nsStyleConsts.h"
#include "nsIDocShell.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollable.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDOMWindow.h"
#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"

DOMCI_DATA(BarProp, mozilla::dom::BarProp)

namespace mozilla {
namespace dom {




BarProp::BarProp(nsGlobalWindow *aWindow)
{
  mDOMWindow = aWindow;
  nsISupports *supwin = static_cast<nsIScriptGlobalObject *>(aWindow);
  mDOMWindowWeakref = do_GetWeakReference(supwin);
}

BarProp::~BarProp()
{
}


NS_INTERFACE_MAP_BEGIN(BarProp)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBarProp)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BarProp)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(BarProp)
NS_IMPL_RELEASE(BarProp)

NS_IMETHODIMP
BarProp::GetVisibleByFlag(bool *aVisible, uint32_t aChromeFlag)
{
  *aVisible = false;

  nsCOMPtr<nsIWebBrowserChrome> browserChrome = GetBrowserChrome();
  NS_ENSURE_TRUE(browserChrome, NS_OK);

  uint32_t chromeFlags;

  NS_ENSURE_SUCCESS(browserChrome->GetChromeFlags(&chromeFlags),
                    NS_ERROR_FAILURE);
  if (chromeFlags & aChromeFlag)
    *aVisible = true;

  return NS_OK;
}

NS_IMETHODIMP
BarProp::SetVisibleByFlag(bool aVisible, uint32_t aChromeFlag)
{
  nsCOMPtr<nsIWebBrowserChrome> browserChrome = GetBrowserChrome();
  NS_ENSURE_TRUE(browserChrome, NS_OK);

  if (!nsContentUtils::IsCallerChrome())
    return NS_OK;

  uint32_t chromeFlags;

  NS_ENSURE_SUCCESS(browserChrome->GetChromeFlags(&chromeFlags),
                    NS_ERROR_FAILURE);
  if (aVisible)
    chromeFlags |= aChromeFlag;
  else
    chromeFlags &= ~aChromeFlag;
  NS_ENSURE_SUCCESS(browserChrome->SetChromeFlags(chromeFlags),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

already_AddRefed<nsIWebBrowserChrome>
BarProp::GetBrowserChrome()
{
  
  nsCOMPtr<nsIDOMWindow> domwin(do_QueryReferent(mDOMWindowWeakref));
  if (!domwin)
    return nullptr;

  return mDOMWindow->GetWebBrowserChrome();
}





MenubarProp::MenubarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

MenubarProp::~MenubarProp()
{
}

NS_IMETHODIMP
MenubarProp::GetVisible(bool *aVisible)
{
  return BarProp::GetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_MENUBAR);
}

NS_IMETHODIMP
MenubarProp::SetVisible(bool aVisible)
{
  return BarProp::SetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_MENUBAR);
}





ToolbarProp::ToolbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

ToolbarProp::~ToolbarProp()
{
}

NS_IMETHODIMP
ToolbarProp::GetVisible(bool *aVisible)
{
  return BarProp::GetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_TOOLBAR);
}

NS_IMETHODIMP
ToolbarProp::SetVisible(bool aVisible)
{
  return BarProp::SetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_TOOLBAR);
}





LocationbarProp::LocationbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

LocationbarProp::~LocationbarProp()
{
}

NS_IMETHODIMP
LocationbarProp::GetVisible(bool *aVisible)
{
  return
    BarProp::GetVisibleByFlag(aVisible,
                              nsIWebBrowserChrome::CHROME_LOCATIONBAR);
}

NS_IMETHODIMP
LocationbarProp::SetVisible(bool aVisible)
{
  return
    BarProp::SetVisibleByFlag(aVisible,
                              nsIWebBrowserChrome::CHROME_LOCATIONBAR);
}





PersonalbarProp::PersonalbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

PersonalbarProp::~PersonalbarProp()
{
}

NS_IMETHODIMP
PersonalbarProp::GetVisible(bool *aVisible)
{
  return
    BarProp::GetVisibleByFlag(aVisible,
                              nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);
}

NS_IMETHODIMP
PersonalbarProp::SetVisible(bool aVisible)
{
  return
    BarProp::SetVisibleByFlag(aVisible,
                              nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);
}





StatusbarProp::StatusbarProp(nsGlobalWindow *aWindow)
  : BarProp(aWindow)
{
}

StatusbarProp::~StatusbarProp()
{
}

NS_IMETHODIMP
StatusbarProp::GetVisible(bool *aVisible)
{
  return BarProp::GetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_STATUSBAR);
}

NS_IMETHODIMP
StatusbarProp::SetVisible(bool aVisible)
{
  return BarProp::SetVisibleByFlag(aVisible,
                                   nsIWebBrowserChrome::CHROME_STATUSBAR);
}





ScrollbarsProp::ScrollbarsProp(nsGlobalWindow *aWindow)
: BarProp(aWindow)
{
}

ScrollbarsProp::~ScrollbarsProp()
{
}

NS_IMETHODIMP
ScrollbarsProp::GetVisible(bool *aVisible)
{
  *aVisible = true; 

  nsCOMPtr<nsIDOMWindow> domwin(do_QueryReferent(mDOMWindowWeakref));
  if (domwin) { 
    nsCOMPtr<nsIScrollable> scroller =
      do_QueryInterface(mDOMWindow->GetDocShell());

    if (scroller) {
      int32_t prefValue;
      scroller->GetDefaultScrollbarPreferences(
                  nsIScrollable::ScrollOrientation_Y, &prefValue);
      if (prefValue == nsIScrollable::Scrollbar_Never) 
        scroller->GetDefaultScrollbarPreferences(
                    nsIScrollable::ScrollOrientation_X, &prefValue);

      if (prefValue == nsIScrollable::Scrollbar_Never)
        *aVisible = false;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
ScrollbarsProp::SetVisible(bool aVisible)
{
  if (!nsContentUtils::IsCallerChrome())
    return NS_OK;

  






  nsCOMPtr<nsIDOMWindow> domwin(do_QueryReferent(mDOMWindowWeakref));
  if (domwin) { 
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

  













  return NS_OK;
}

} 
} 


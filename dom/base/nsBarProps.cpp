





































#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsBarProps.h"
#include "nsGlobalWindow.h"
#include "nsStyleConsts.h"
#include "nsIDocShell.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollable.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDOMWindowInternal.h"
#include "nsDOMClassInfo.h"




nsBarProp::nsBarProp(nsGlobalWindow *aWindow)
{
  mDOMWindow = aWindow;
  nsISupports *supwin = static_cast<nsIScriptGlobalObject *>(aWindow);
  mDOMWindowWeakref = do_GetWeakReference(supwin);
}

nsBarProp::~nsBarProp()
{
}


DOMCI_DATA(BarProp, nsBarProp)


NS_INTERFACE_MAP_BEGIN(nsBarProp)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBarProp)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BarProp)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsBarProp)
NS_IMPL_RELEASE(nsBarProp)

NS_IMETHODIMP
nsBarProp::GetVisibleByFlag(PRBool *aVisible, PRUint32 aChromeFlag)
{
  *aVisible = PR_FALSE;

  nsCOMPtr<nsIWebBrowserChrome> browserChrome = GetBrowserChrome();
  NS_ENSURE_TRUE(browserChrome, NS_OK);

  PRUint32 chromeFlags;

  NS_ENSURE_SUCCESS(browserChrome->GetChromeFlags(&chromeFlags),
                    NS_ERROR_FAILURE);
  if (chromeFlags & aChromeFlag)
    *aVisible = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsBarProp::SetVisibleByFlag(PRBool aVisible, PRUint32 aChromeFlag)
{
  nsCOMPtr<nsIWebBrowserChrome> browserChrome = GetBrowserChrome();
  NS_ENSURE_TRUE(browserChrome, NS_OK);

  PRBool enabled = PR_FALSE;

  nsCOMPtr<nsIScriptSecurityManager>
           securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  if (securityManager)
    securityManager->IsCapabilityEnabled("UniversalBrowserWrite", &enabled);
  if (!enabled)
    return NS_OK;

  PRUint32 chromeFlags;

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
nsBarProp::GetBrowserChrome()
{
  
  nsCOMPtr<nsIDOMWindow> domwin(do_QueryReferent(mDOMWindowWeakref));
  if (!domwin)
    return nsnull;

  nsIWebBrowserChrome *browserChrome = nsnull;
  mDOMWindow->GetWebBrowserChrome(&browserChrome);
  return browserChrome;
}





nsMenubarProp::nsMenubarProp(nsGlobalWindow *aWindow) 
  : nsBarProp(aWindow)
{
}

nsMenubarProp::~nsMenubarProp()
{
}

NS_IMETHODIMP
nsMenubarProp::GetVisible(PRBool *aVisible)
{
  return nsBarProp::GetVisibleByFlag(aVisible,
                                     nsIWebBrowserChrome::CHROME_MENUBAR);
}

NS_IMETHODIMP
nsMenubarProp::SetVisible(PRBool aVisible)
{
  return nsBarProp::SetVisibleByFlag(aVisible,
                                     nsIWebBrowserChrome::CHROME_MENUBAR);
}





nsToolbarProp::nsToolbarProp(nsGlobalWindow *aWindow)
  : nsBarProp(aWindow)
{
}

nsToolbarProp::~nsToolbarProp()
{
}

NS_IMETHODIMP
nsToolbarProp::GetVisible(PRBool *aVisible)
{
  return nsBarProp::GetVisibleByFlag(aVisible,
                                     nsIWebBrowserChrome::CHROME_TOOLBAR);
}

NS_IMETHODIMP
nsToolbarProp::SetVisible(PRBool aVisible)
{
  return nsBarProp::SetVisibleByFlag(aVisible,
                                     nsIWebBrowserChrome::CHROME_TOOLBAR);
}





nsLocationbarProp::nsLocationbarProp(nsGlobalWindow *aWindow)
  : nsBarProp(aWindow)
{
}

nsLocationbarProp::~nsLocationbarProp()
{
}

NS_IMETHODIMP
nsLocationbarProp::GetVisible(PRBool *aVisible)
{
  return
    nsBarProp::GetVisibleByFlag(aVisible,
                                nsIWebBrowserChrome::CHROME_LOCATIONBAR);
}

NS_IMETHODIMP
nsLocationbarProp::SetVisible(PRBool aVisible)
{
  return
    nsBarProp::SetVisibleByFlag(aVisible,
                                nsIWebBrowserChrome::CHROME_LOCATIONBAR);
}





nsPersonalbarProp::nsPersonalbarProp(nsGlobalWindow *aWindow)
  : nsBarProp(aWindow)
{
}

nsPersonalbarProp::~nsPersonalbarProp()
{
}

NS_IMETHODIMP
nsPersonalbarProp::GetVisible(PRBool *aVisible)
{
  return
    nsBarProp::GetVisibleByFlag(aVisible,
                                nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);
}

NS_IMETHODIMP
nsPersonalbarProp::SetVisible(PRBool aVisible)
{
  return
    nsBarProp::SetVisibleByFlag(aVisible,
                                nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);
}





nsStatusbarProp::nsStatusbarProp(nsGlobalWindow *aWindow)
  : nsBarProp(aWindow)
{
}

nsStatusbarProp::~nsStatusbarProp()
{
}

NS_IMETHODIMP
nsStatusbarProp::GetVisible(PRBool *aVisible)
{
  return nsBarProp::GetVisibleByFlag(aVisible,
                                     nsIWebBrowserChrome::CHROME_STATUSBAR);
}

NS_IMETHODIMP
nsStatusbarProp::SetVisible(PRBool aVisible)
{
  return nsBarProp::SetVisibleByFlag(aVisible,
                                     nsIWebBrowserChrome::CHROME_STATUSBAR);
}





nsScrollbarsProp::nsScrollbarsProp(nsGlobalWindow *aWindow)
: nsBarProp(aWindow)
{
}

nsScrollbarsProp::~nsScrollbarsProp()
{
}

NS_IMETHODIMP
nsScrollbarsProp::GetVisible(PRBool *aVisible)
{
  *aVisible = PR_TRUE; 

  nsCOMPtr<nsIDOMWindow> domwin(do_QueryReferent(mDOMWindowWeakref));
  if (domwin) { 
    nsCOMPtr<nsIScrollable> scroller =
      do_QueryInterface(mDOMWindow->GetDocShell());

    if (scroller) {
      PRInt32 prefValue;
      scroller->GetDefaultScrollbarPreferences(
                  nsIScrollable::ScrollOrientation_Y, &prefValue);
      if (prefValue == nsIScrollable::Scrollbar_Never) 
        scroller->GetDefaultScrollbarPreferences(
                    nsIScrollable::ScrollOrientation_X, &prefValue);

      if (prefValue == nsIScrollable::Scrollbar_Never)
        *aVisible = PR_FALSE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsScrollbarsProp::SetVisible(PRBool aVisible)
{
  PRBool   enabled = PR_FALSE;

  nsCOMPtr<nsIScriptSecurityManager>
           securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  if (securityManager)
    securityManager->IsCapabilityEnabled("UniversalBrowserWrite", &enabled);
  if (!enabled)
    return NS_OK;

  






  nsCOMPtr<nsIDOMWindow> domwin(do_QueryReferent(mDOMWindowWeakref));
  if (domwin) { 
    nsCOMPtr<nsIScrollable> scroller =
      do_QueryInterface(mDOMWindow->GetDocShell());

    if (scroller) {
      PRInt32 prefValue;

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


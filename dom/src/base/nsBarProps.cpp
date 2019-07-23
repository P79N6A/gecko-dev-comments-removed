





































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




nsBarProp::nsBarProp() : mBrowserChrome(nsnull)
{
}

nsBarProp::~nsBarProp()
{
}



NS_INTERFACE_MAP_BEGIN(nsBarProp)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBarProp)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BarProp)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsBarProp)
NS_IMPL_RELEASE(nsBarProp)


NS_IMETHODIMP
nsBarProp::SetWebBrowserChrome(nsIWebBrowserChrome* aBrowserChrome)
{
  mBrowserChrome = aBrowserChrome;
  return NS_OK;
}

NS_IMETHODIMP
nsBarProp::GetVisibleByFlag(PRBool *aVisible, PRUint32 aChromeFlag)
{
  NS_ENSURE_TRUE(mBrowserChrome, NS_ERROR_FAILURE);

  PRUint32 chromeFlags;
  *aVisible = PR_FALSE;

  NS_ENSURE_SUCCESS(mBrowserChrome->GetChromeFlags(&chromeFlags),
                    NS_ERROR_FAILURE);
  if(chromeFlags & aChromeFlag)
    *aVisible = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsBarProp::SetVisibleByFlag(PRBool aVisible, PRUint32 aChromeFlag)
{
  NS_ENSURE_TRUE(mBrowserChrome, NS_ERROR_FAILURE);

  PRBool   enabled = PR_FALSE;

  nsCOMPtr<nsIScriptSecurityManager>
           securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  if (securityManager)
    securityManager->IsCapabilityEnabled("UniversalBrowserWrite", &enabled);
  if (!enabled)
    return NS_OK;

  PRUint32 chromeFlags;

  NS_ENSURE_SUCCESS(mBrowserChrome->GetChromeFlags(&chromeFlags),
                    NS_ERROR_FAILURE);
  if(aVisible)
    chromeFlags |= aChromeFlag;
  else
    chromeFlags &= ~aChromeFlag;
  NS_ENSURE_SUCCESS(mBrowserChrome->SetChromeFlags(chromeFlags),
                    NS_ERROR_FAILURE);

  return NS_OK;
}





nsMenubarProp::nsMenubarProp()
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





nsToolbarProp::nsToolbarProp()
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





nsLocationbarProp::nsLocationbarProp()
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





nsPersonalbarProp::nsPersonalbarProp()
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





nsStatusbarProp::nsStatusbarProp()
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
{
  mDOMWindow = aWindow;
  nsISupports *supwin = NS_STATIC_CAST(nsIScriptGlobalObject *, aWindow);
  mDOMWindowWeakref = do_GetWeakReference(supwin);
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


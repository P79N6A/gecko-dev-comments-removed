







































#include "nsIAppShellService.h"
#include "nsISupportsArray.h"
#include "nsIComponentManager.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#include "nsIWindowMediator.h"
#include "nsIWindowWatcher.h"
#include "nsPIWindowWatcher.h"
#include "nsIDOMWindowInternal.h"
#include "nsWebShellWindow.h"

#include "nsIEnumerator.h"
#include "nsCRT.h"
#include "nsITimelineService.h"
#include "prprf.h"    

#include "nsWidgetsCID.h"
#include "nsIRequestObserver.h"


#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "jsapi.h"

#include "nsAppShellService.h"
#include "nsISupportsPrimitives.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeDecoder.h"


#define DEFAULT_HIDDENWINDOW_URL "resource://gre/res/hiddenWindow.html"

class nsIAppShell;

nsAppShellService::nsAppShellService() : 
  mXPCOMShuttingDown(PR_FALSE),
  mModalWindowCount(0),
  mApplicationProvidedHiddenWindow(PR_FALSE)
{
  nsCOMPtr<nsIObserverService> obs
    (do_GetService("@mozilla.org/observer-service;1"));

  if (obs)
    obs->AddObserver(this, "xpcom-shutdown", PR_FALSE);
}

nsAppShellService::~nsAppShellService()
{
}





NS_IMPL_ISUPPORTS2(nsAppShellService,
                   nsIAppShellService,
                   nsIObserver)

nsresult 
nsAppShellService::SetXPConnectSafeContext()
{
  nsresult rv;

  nsCOMPtr<nsIThreadJSContextStack> cxstack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindowInternal> junk;
  JSContext *cx;
  rv = GetHiddenWindowAndJSContext(getter_AddRefs(junk), &cx);
  NS_ENSURE_SUCCESS(rv, rv);

  return cxstack->SetSafeJSContext(cx);
}  

nsresult nsAppShellService::ClearXPConnectSafeContext()
{
  nsresult rv;

  nsCOMPtr<nsIThreadJSContextStack> cxstack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("XPConnect ContextStack gone before XPCOM shutdown?");
    return rv;
  }

  nsCOMPtr<nsIDOMWindowInternal> junk;
  JSContext *cx;
  rv = GetHiddenWindowAndJSContext(getter_AddRefs(junk), &cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext *safe_cx;
  rv = cxstack->GetSafeJSContext(&safe_cx);
  NS_ENSURE_SUCCESS(rv, rv);

  if (cx == safe_cx)
    rv = cxstack->SetSafeJSContext(nsnull);

  return rv;
}

NS_IMETHODIMP
nsAppShellService::CreateHiddenWindow(nsIAppShell* aAppShell)
{
  nsresult rv;
  PRInt32 initialHeight = 100, initialWidth = 100;
    
#ifdef XP_MACOSX
  PRUint32    chromeMask = 0;
  nsCOMPtr<nsIPrefBranch> prefBranch;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
  nsXPIDLCString prefVal;
  rv = prefBranch->GetCharPref("browser.hiddenWindowChromeURL", getter_Copies(prefVal));
  const char* hiddenWindowURL = prefVal.get() ? prefVal.get() : DEFAULT_HIDDENWINDOW_URL;
  mApplicationProvidedHiddenWindow = prefVal.get() ? PR_TRUE : PR_FALSE;
#else
  static const char hiddenWindowURL[] = DEFAULT_HIDDENWINDOW_URL;
  PRUint32    chromeMask =  nsIWebBrowserChrome::CHROME_ALL;
#endif

  nsCOMPtr<nsIURI> url;
  rv = NS_NewURI(getter_AddRefs(url), hiddenWindowURL);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsWebShellWindow> newWindow;
  rv = JustCreateTopWindow(nsnull, url,
                           chromeMask, initialWidth, initialHeight,
                           PR_TRUE, aAppShell, getter_AddRefs(newWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  mHiddenWindow.swap(newWindow);

#ifdef XP_MACOSX
  
  
  
  
  mHiddenWindow->SetPosition ( -32000, -32000 );
  mHiddenWindow->SetVisibility ( PR_TRUE );
#endif

  
  
  
  
  SetXPConnectSafeContext();

  

  return NS_OK;
}

NS_IMETHODIMP
nsAppShellService::DestroyHiddenWindow()
{
  if (mHiddenWindow) {
    ClearXPConnectSafeContext();
    mHiddenWindow->Destroy();

    mHiddenWindow = nsnull;
  }

  return NS_OK;
}




NS_IMETHODIMP
nsAppShellService::CreateTopLevelWindow(nsIXULWindow *aParent,
                                        nsIURI *aUrl, 
                                        PRUint32 aChromeMask,
                                        PRInt32 aInitialWidth,
                                        PRInt32 aInitialHeight,
                                        nsIAppShell* aAppShell,
                                        nsIXULWindow **aResult)

{
  nsresult rv;

  nsWebShellWindow *newWindow = nsnull;
  rv = JustCreateTopWindow(aParent, aUrl,
                           aChromeMask, aInitialWidth, aInitialHeight,
                           PR_FALSE, aAppShell, &newWindow);  

  *aResult = newWindow; 

  if (NS_SUCCEEDED(rv)) {
    
    RegisterTopLevelWindow(*aResult);
    nsCOMPtr<nsIXULWindow> parent;
    if (aChromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT)
      parent = aParent;
    (*aResult)->SetZLevel(CalculateWindowZLevel(parent, aChromeMask));
  }

  return rv;
}

PRUint32
nsAppShellService::CalculateWindowZLevel(nsIXULWindow *aParent,
                                         PRUint32      aChromeMask)
{
  PRUint32 zLevel;

  zLevel = nsIXULWindow::normalZ;
  if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RAISED)
    zLevel = nsIXULWindow::raisedZ;
  else if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_LOWERED)
    zLevel = nsIXULWindow::loweredZ;

#ifdef XP_MACOSX
  





  PRUint32 modalDepMask = nsIWebBrowserChrome::CHROME_MODAL |
                          nsIWebBrowserChrome::CHROME_DEPENDENT;
  if (aParent && (aChromeMask & modalDepMask)) {
    aParent->GetZLevel(&zLevel);
  }
#else
  



  if ((aChromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT) && aParent)
    aParent->GetZLevel(&zLevel);
#endif

  return zLevel;
}




nsresult
nsAppShellService::JustCreateTopWindow(nsIXULWindow *aParent,
                                       nsIURI *aUrl, 
                                       PRUint32 aChromeMask,
                                       PRInt32 aInitialWidth,
                                       PRInt32 aInitialHeight,
                                       PRBool aIsHiddenWindow,
                                       nsIAppShell* aAppShell,
                                       nsWebShellWindow **aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIXULWindow> parent;
  if (aChromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT)
    parent = aParent;

  nsRefPtr<nsWebShellWindow> window = new nsWebShellWindow(aChromeMask);
  NS_ENSURE_TRUE(window, NS_ERROR_OUT_OF_MEMORY);

  nsWidgetInitData widgetInitData;

  if (aIsHiddenWindow)
    widgetInitData.mWindowType = eWindowType_invisible;
  else
    widgetInitData.mWindowType = aChromeMask & nsIWebBrowserChrome::CHROME_OPENAS_DIALOG ?
      eWindowType_dialog : eWindowType_toplevel;

  if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_POPUP)
    widgetInitData.mWindowType = eWindowType_popup;

#ifdef XP_MACOSX
  
  
  
  
  
  
  PRUint32 sheetMask = nsIWebBrowserChrome::CHROME_OPENAS_DIALOG |
                       nsIWebBrowserChrome::CHROME_MODAL |
                       nsIWebBrowserChrome::CHROME_OPENAS_CHROME;
  if (parent && ((aChromeMask & sheetMask) == sheetMask))
    widgetInitData.mWindowType = eWindowType_sheet;
#endif

  widgetInitData.mContentType = eContentTypeUI;                

  
  
  if (aChromeMask & nsIWebBrowserChrome::CHROME_DEFAULT)
    widgetInitData.mBorderStyle = eBorderStyle_default;
  else if ((aChromeMask & nsIWebBrowserChrome::CHROME_ALL) == nsIWebBrowserChrome::CHROME_ALL)
    widgetInitData.mBorderStyle = eBorderStyle_all;
  else {
    widgetInitData.mBorderStyle = eBorderStyle_none; 
    if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_BORDERS)
      widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_border);
    if (aChromeMask & nsIWebBrowserChrome::CHROME_TITLEBAR)
      widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_title);
    if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_CLOSE)
      widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_close);
    if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE) {
      widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_resizeh);
      
      if (!(aChromeMask & nsIWebBrowserChrome::CHROME_OPENAS_DIALOG))
        widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_maximize);
    }
    
    if (!(aChromeMask & nsIWebBrowserChrome::CHROME_OPENAS_DIALOG))
      widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_minimize | eBorderStyle_menu);
    
    if (aChromeMask & nsIWebBrowserChrome::CHROME_WINDOW_MIN) {
      widgetInitData.mBorderStyle = static_cast<enum nsBorderStyle>(widgetInitData.mBorderStyle | eBorderStyle_minimize);
    }  
  }

  if (aInitialWidth == nsIAppShellService::SIZE_TO_CONTENT ||
      aInitialHeight == nsIAppShellService::SIZE_TO_CONTENT) {
    aInitialWidth = 1;
    aInitialHeight = 1;
    window->SetIntrinsicallySized(PR_TRUE);
  }

  PRBool center = aChromeMask & nsIWebBrowserChrome::CHROME_CENTER_SCREEN;

  nsresult rv = window->Initialize(parent, center ? aParent : nsnull,
                                   aAppShell, aUrl,
                                   aInitialWidth, aInitialHeight,
                                   aIsHiddenWindow, widgetInitData);
      
  NS_ENSURE_SUCCESS(rv, rv);

  window.swap(*aResult); 
  if (parent)
    parent->AddChildWindow(*aResult);

  if (center)
    rv = (*aResult)->Center(parent, parent ? PR_FALSE : PR_TRUE, PR_FALSE);

  return rv;
}

NS_IMETHODIMP
nsAppShellService::GetHiddenWindow(nsIXULWindow **aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  *aWindow = mHiddenWindow;
  NS_IF_ADDREF(*aWindow);
  return *aWindow ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsAppShellService::GetHiddenDOMWindow(nsIDOMWindowInternal **aWindow)
{
  nsresult rv;
  nsCOMPtr<nsIDocShell> docShell;
  NS_ENSURE_TRUE(mHiddenWindow, NS_ERROR_FAILURE);

  rv = mHiddenWindow->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMWindowInternal> hiddenDOMWindow(do_GetInterface(docShell, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  *aWindow = hiddenDOMWindow;
  NS_IF_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsAppShellService::GetHiddenWindowAndJSContext(nsIDOMWindowInternal **aWindow,
                                               JSContext    **aJSContext)
{
    nsresult rv = NS_OK;
    if ( aWindow && aJSContext ) {
        *aWindow    = nsnull;
        *aJSContext = nsnull;

        if ( mHiddenWindow ) {
            
            do {
                
                nsCOMPtr<nsIDocShell> docShell;
                rv = mHiddenWindow->GetDocShell(getter_AddRefs(docShell));
                if (NS_FAILED(rv)) break;

                
                nsCOMPtr<nsIDOMWindowInternal> hiddenDOMWindow(do_GetInterface(docShell));
                if(!hiddenDOMWindow) break;

                
                nsCOMPtr<nsIScriptGlobalObject> sgo;
                sgo = do_QueryInterface( hiddenDOMWindow );
                if (!sgo) { rv = NS_ERROR_FAILURE; break; }

                
                nsIScriptContext *scriptContext = sgo->GetContext();
                if (!scriptContext) { rv = NS_ERROR_FAILURE; break; }

                
                JSContext *jsContext = (JSContext*)scriptContext->GetNativeContext();
                if (!jsContext) { rv = NS_ERROR_FAILURE; break; }

                
                *aWindow    = hiddenDOMWindow.get();
                NS_IF_ADDREF( *aWindow );
                *aJSContext = jsContext;
            } while (0);
        } else {
            rv = NS_ERROR_FAILURE;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

NS_IMETHODIMP
nsAppShellService::GetApplicationProvidedHiddenWindow(PRBool* aAPHW)
{
    *aAPHW = mApplicationProvidedHiddenWindow;
    return NS_OK;
}




NS_IMETHODIMP
nsAppShellService::RegisterTopLevelWindow(nsIXULWindow* aWindow)
{
  
  nsCOMPtr<nsIWindowMediator> mediator
    ( do_GetService(NS_WINDOWMEDIATOR_CONTRACTID) );
  NS_ASSERTION(mediator, "Couldn't get window mediator.");

  if (mediator)
    mediator->RegisterWindow(aWindow);

  
  nsCOMPtr<nsPIWindowWatcher> wwatcher ( do_GetService(NS_WINDOWWATCHER_CONTRACTID) );
  NS_ASSERTION(wwatcher, "No windowwatcher?");
  if (wwatcher) {
    nsCOMPtr<nsIDocShell> docShell;
    aWindow->GetDocShell(getter_AddRefs(docShell));
    NS_ASSERTION(docShell, "Window has no docshell");
    if (docShell) {
      nsCOMPtr<nsIDOMWindow> domWindow(do_GetInterface(docShell));
      NS_ASSERTION(domWindow, "Couldn't get DOM window.");
      if (domWindow)
        wwatcher->AddWindow(domWindow, 0);
    }
  }

  
  nsCOMPtr<nsIObserverService> obssvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ASSERTION(obssvc, "Couldn't get observer service.");

  if (obssvc)
    obssvc->NotifyObservers(aWindow, "xul-window-registered", nsnull);

  return NS_OK;
}


NS_IMETHODIMP
nsAppShellService::UnregisterTopLevelWindow(nsIXULWindow* aWindow)
{
  if (mXPCOMShuttingDown) {
    







    return NS_ERROR_FAILURE;
  }
  
  NS_ENSURE_ARG_POINTER(aWindow);

  if (aWindow == mHiddenWindow) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIWindowMediator> mediator
    ( do_GetService(NS_WINDOWMEDIATOR_CONTRACTID) );
  NS_ASSERTION(mediator, "Couldn't get window mediator. Doing xpcom shutdown?");

  if (mediator)
    mediator->UnregisterWindow(aWindow);
	
  
  nsCOMPtr<nsPIWindowWatcher> wwatcher ( do_GetService(NS_WINDOWWATCHER_CONTRACTID) );
  NS_ASSERTION(wwatcher, "Couldn't get windowwatcher, doing xpcom shutdown?");
  if (wwatcher) {
    nsCOMPtr<nsIDocShell> docShell;
    aWindow->GetDocShell(getter_AddRefs(docShell));
    if (docShell) {
      nsCOMPtr<nsIDOMWindow> domWindow(do_GetInterface(docShell));
      if (domWindow)
        wwatcher->RemoveWindow(domWindow);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsAppShellService::Observe(nsISupports* aSubject, const char *aTopic,
                           const PRUnichar *aData)
{
  NS_ASSERTION(!strcmp(aTopic, "xpcom-shutdown"), "Unexpected observer topic!");

  mXPCOMShuttingDown = PR_TRUE;
  if (mHiddenWindow) {
    ClearXPConnectSafeContext();
    mHiddenWindow->Destroy();
  }

  return NS_OK;
}

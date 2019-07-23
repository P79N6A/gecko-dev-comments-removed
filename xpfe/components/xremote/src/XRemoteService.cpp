






































#include "XRemoteService.h"
#include "XRemoteContentListener.h"
#include "nsIBrowserDOMWindow.h"

#include <nsIGenericFactory.h>
#include <nsIWebNavigation.h>
#include <nsPIDOMWindow.h>
#include <nsIDOMChromeWindow.h>
#include <nsIDocShell.h>
#include <nsIBaseWindow.h>
#include <nsIServiceManager.h>
#include <nsString.h>
#include <nsCRT.h>
#include <nsIPref.h>
#include <nsIWindowWatcher.h>
#include <nsXPCOM.h>
#include <nsISupportsPrimitives.h>
#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIURILoader.h>
#include <nsCURILoader.h>
#include <nsIURIFixup.h>
#include <nsCDefaultURIFixup.h>
#include <nsIURI.h>
#include <nsNetUtil.h>
#include <nsIWindowMediator.h>
#include <nsCExternalHandlerService.h>
#include <nsIExternalProtocolService.h>
#include <nsIProfile.h>

#include "nsICmdLineHandler.h"

XRemoteService::XRemoteService()
{
}

XRemoteService::~XRemoteService()
{
}

NS_IMPL_ISUPPORTS1(XRemoteService, nsISuiteRemoteService)

NS_IMETHODIMP
XRemoteService::ParseCommand(const char *aCommand, nsIDOMWindow* aWindow)
{
  NS_ASSERTION(aCommand, "Tell me what to do, or shut up!");

  
  nsCString tempString(aCommand);

  
  PRInt32 begin_arg = tempString.FindChar('(');
  PRInt32 end_arg = tempString.RFindChar(')');

  
  
  if (begin_arg == kNotFound || end_arg == kNotFound ||
      begin_arg == 0 || end_arg < begin_arg)
    return NS_ERROR_INVALID_ARG;

  
  tempString.Truncate(end_arg);

  
  nsCString argument(tempString);
  argument.Cut(0, begin_arg + 1);
  argument.Trim(" ", PR_TRUE, PR_TRUE);

  
  tempString.Truncate(begin_arg);

  
  nsCString action(tempString);
  action.Trim(" ", PR_TRUE, PR_TRUE);
  ToLowerCase(action);

  
  PRUint32  index = 0;
  PRBool    raiseWindow = PR_TRUE;
  nsCString lastArgument;

  FindLastInList(argument, lastArgument, &index);
  if (lastArgument.LowerCaseEqualsLiteral("noraise")) {
    argument.Truncate(index);
    raiseWindow = PR_FALSE;
  }

  nsresult rv = NS_OK;
  
  








  







  if (action.Equals("openurl") || action.Equals("openfile")) {
    rv = OpenURL(argument, aWindow, PR_TRUE);
  }

  










  else if (action.Equals("saveas")) {
    if (argument.IsEmpty()) {
      rv = NS_ERROR_NOT_IMPLEMENTED;
    }
    else {
      
      index = 0;
      FindLastInList(argument, lastArgument, &index);
      if (lastArgument.LowerCaseEqualsLiteral("html")) {
	argument.Truncate(index);
	rv = NS_ERROR_NOT_IMPLEMENTED;
      }
      else if (lastArgument.EqualsIgnoreCase("text", PR_TRUE)) {
	argument.Truncate(index);
	rv = NS_ERROR_NOT_IMPLEMENTED;
      }
      else if (lastArgument.EqualsIgnoreCase("postscript", PR_TRUE)) {
	argument.Truncate(index);
	rv = NS_ERROR_NOT_IMPLEMENTED;
      }
      else {
	rv = NS_ERROR_NOT_IMPLEMENTED;
      }
    }
   
  }

  







  else if (action.Equals("mailto")) {
    
    
    nsCString tempArg("mailto:");
    tempArg.Append(argument);
    rv = OpenURL(tempArg, aWindow, PR_FALSE);
  }

  









  else if (action.Equals("addbookmark")) {
    rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  




  else if (action.Equals("ping")) {
    
    rv = NS_OK;
  }

  





  else if (action.Equals("xfedocommand")) {
    rv = XfeDoCommand(argument, aWindow);
  }

  
  else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

void
XRemoteService::FindRestInList(nsCString &aString, nsCString &retString,
                               PRUint32 *aIndexRet)
{
  
  *aIndexRet = 0;
  nsCString tempString;
  PRInt32   strIndex;
  
  strIndex = aString.FindChar(',');

  
  if (strIndex == kNotFound)
    return;

  
  tempString = Substring(aString, strIndex+1, aString.Length());

  
  tempString.Trim(" ", PR_TRUE, PR_TRUE);

  
  if (tempString.IsEmpty())
    return;

  *aIndexRet = strIndex;

  
  retString = tempString;

}

void
XRemoteService::FindLastInList(nsCString &aString, nsCString &retString,
			       PRUint32 *aIndexRet)
{
  
  *aIndexRet = 0;
  
  nsCString tempString = aString;
  PRInt32   strIndex;
  
  strIndex = tempString.RFindChar(',');

  
  if (strIndex == kNotFound)
    return;

  
  tempString.Cut(0, strIndex + 1);

  
  tempString.Trim(" ", PR_TRUE, PR_TRUE);

  
  if (tempString.IsEmpty())
    return;

  *aIndexRet = strIndex;

  
  retString = tempString;

}

nsresult
XRemoteService::OpenChromeWindow(nsIDOMWindow *aParent,
				 const char *aUrl, const char *aFeatures,
				 nsISupports *aArguments,
				 nsIDOMWindow **_retval)
{
  nsCOMPtr<nsIWindowWatcher> watcher;
  watcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID);
    
  if (!watcher)
    return NS_ERROR_FAILURE;

  return watcher->OpenWindow(aParent, aUrl, "_blank",
			     aFeatures, aArguments, _retval);
}

nsresult
XRemoteService::GetBrowserLocation(char **_retval)
{
  
  nsCOMPtr<nsIPref> prefs;
  prefs = do_GetService(NS_PREF_CONTRACTID);
  if (!prefs)
    return NS_ERROR_FAILURE;
  
  prefs->CopyCharPref("browser.chromeURL", _retval);

  
  if (!*_retval)
    *_retval = nsCRT::strdup("chrome://navigator/content/navigator.xul");

  return NS_OK;
}

nsresult
XRemoteService::GetMailLocation(char **_retval)
{
  
  nsCOMPtr<nsIPref> prefs;
  prefs = do_GetService(NS_PREF_CONTRACTID);
  if (!prefs)
    return NS_ERROR_FAILURE;
  
  PRInt32 retval = 0;
  nsresult rv;
  rv = prefs->GetIntPref("mail.pane_config", &retval);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (!retval)
    *_retval = nsCRT::strdup("chrome://messenger/content/messenger.xul");
  else
    *_retval = nsCRT::strdup("chrome://messenger/content/mail3PaneWindowVertLayout.xul");

  return NS_OK;
  
}

nsresult
XRemoteService::GetComposeLocation(const char **_retval)
{
  
  *_retval = "chrome://messenger/content/messengercompose/messengercompose.xul";

  return NS_OK;
}

nsresult
XRemoteService::GetCalendarLocation(char **_retval)
{
  
  nsCOMPtr<nsIPref> prefs;
  prefs = do_GetService(NS_PREF_CONTRACTID);
  if (!prefs)
    return NS_ERROR_FAILURE;

  prefs->CopyCharPref("calendar.chromeURL", _retval);

  
  if (!*_retval)
    *_retval = nsCRT::strdup("chrome://calendar/content/calendar.xul");

  return NS_OK;
}

PRBool
XRemoteService::MayOpenURL(const nsCString &aURL)
{
  
  PRBool allowURL= PR_FALSE;

  nsCOMPtr<nsIExternalProtocolService> extProtService =
      do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
  if (extProtService) {
    nsCAutoString scheme;

    
    if (aURL.IsEmpty()) {
      scheme.AssignLiteral("about");
    }
    else {
      nsCOMPtr<nsIURIFixup> fixup = do_GetService(NS_URIFIXUP_CONTRACTID);
      if (fixup) {
        nsCOMPtr<nsIURI> uri;
        nsresult rv =
          fixup->CreateFixupURI(aURL, nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI,
                                getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv) && uri) {
          uri->GetScheme(scheme);
        }
      }
    }

    if (!scheme.IsEmpty()) {
      
      
      PRBool isExposed;
      nsresult rv = extProtService->IsExposedProtocol(scheme.get(), &isExposed);
      if (NS_SUCCEEDED(rv) && isExposed)
        allowURL = PR_TRUE; 
    }
  }

  return allowURL;
}

nsresult
XRemoteService::OpenURL(nsCString &aArgument,
			nsIDOMWindow *aParent,
			PRBool aOpenBrowser)
{
  
  nsCOMPtr<nsIDOMWindowInternal> finalWindow = do_QueryInterface(aParent);

  
  nsCString lastArgument;
  PRBool    newWindow = PR_FALSE, newTab = PR_FALSE;
  PRUint32  index = 0;
  FindLastInList(aArgument, lastArgument, &index);

  newTab = lastArgument.LowerCaseEqualsLiteral("new-tab");

  if (newTab || lastArgument.LowerCaseEqualsLiteral("new-window")) {
    aArgument.Truncate(index);
    
    if (!newTab && aOpenBrowser)
      newWindow = PR_TRUE;
    
    
    FindLastInList(aArgument, lastArgument, &index);
    if (lastArgument.LowerCaseEqualsLiteral("noraise"))
      aArgument.Truncate(index);
  }

  nsCOMPtr<nsIBrowserDOMWindow> bwin;

  
  
  
  
  
  
  if (aOpenBrowser && (!newWindow || newTab)) {
    nsCOMPtr<nsIDOMWindowInternal> lastUsedWindow;
    FindWindow(NS_LITERAL_STRING("navigator:browser").get(),
	       getter_AddRefs(lastUsedWindow));

    if (lastUsedWindow) {
      finalWindow = lastUsedWindow;
      nsCOMPtr<nsIWebNavigation> navNav(do_GetInterface(finalWindow));
      nsCOMPtr<nsIDocShellTreeItem> navItem(do_QueryInterface(navNav));
      if (navItem) {
        nsCOMPtr<nsIDocShellTreeItem> rootItem;
        navItem->GetRootTreeItem(getter_AddRefs(rootItem));
        nsCOMPtr<nsIDOMWindow> rootWin(do_GetInterface(rootItem));
        nsCOMPtr<nsIDOMChromeWindow> chromeWin(do_QueryInterface(rootWin));
        if (chromeWin)
          chromeWin->GetBrowserDOMWindow(getter_AddRefs(bwin));
      }
    }
    if (!finalWindow || !bwin)
      newWindow = PR_TRUE;
  }

  
  if (!MayOpenURL(aArgument))
    return NS_ERROR_ABORT;

  nsresult rv = NS_OK;

  
  nsString url;
  url.AssignWithConversion(aArgument.get());

  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), url);

  if (newWindow) {
    nsXPIDLCString urlString;
    GetBrowserLocation(getter_Copies(urlString));
    if (!urlString)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupportsString> arg;
    arg = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    if (!arg)
      return NS_ERROR_FAILURE;
    
    
    arg->SetData(url);
    
    nsCOMPtr<nsIDOMWindow> window;
    rv = OpenChromeWindow(finalWindow, urlString, "chrome,all,dialog=no",
			  arg, getter_AddRefs(window));
  }

  
  
  else if (!finalWindow) {
    nsCOMPtr<nsIURILoader> loader;
    loader = do_GetService(NS_URI_LOADER_CONTRACTID);
    if (!loader)
      return NS_ERROR_FAILURE;
    
    XRemoteContentListener *listener;
    listener = new XRemoteContentListener();
    if (!listener)
      return NS_ERROR_FAILURE;

    
    NS_ADDREF(listener);
    nsCOMPtr<nsISupports> listenerRef;
    listenerRef = do_QueryInterface(NS_STATIC_CAST(nsIURIContentListener *,
						   listener));
    
    NS_RELEASE(listener);

    
    if (!uri)
      return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewChannel(getter_AddRefs(channel), uri);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

    
    rv = loader->OpenURI(channel, PR_TRUE, listener);
  }

  else if (newTab && aOpenBrowser) {
    if (bwin && uri) {
      nsCOMPtr<nsIDOMWindow> container;
      rv = bwin->OpenURI(uri, 0,
                         nsIBrowserDOMWindow::OPEN_NEWTAB,
                         nsIBrowserDOMWindow::OPEN_EXTERNAL,
                         getter_AddRefs(container));
    }
    else {
      NS_ERROR("failed to open remote URL in new tab");
      return NS_ERROR_FAILURE;
    }
  }

  else if (bwin && uri) { 
    nsCOMPtr<nsIDOMWindow> container;
    rv = bwin->OpenURI(uri, 0,
                       nsIBrowserDOMWindow::OPEN_DEFAULTWINDOW,
                       nsIBrowserDOMWindow::OPEN_EXTERNAL,
                       getter_AddRefs(container));
    if (NS_SUCCEEDED(rv))
      return NS_OK;
  }

  else { 
    
    
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(finalWindow));
    if (!win) {
      NS_WARNING("Failed to get script object for browser instance");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();
    if (!docShell) {
      NS_WARNING("Failed to get docshell object for browser instance");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(docShell));
    if (!item) {
      NS_WARNING("failed to get doc shell tree item for browser instance");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    item->GetTreeOwner(getter_AddRefs(treeOwner));
    if (!treeOwner) {
      NS_WARNING("failed to get tree owner");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDocShellTreeItem> primaryContent;
    treeOwner->GetPrimaryContentShell(getter_AddRefs(primaryContent));

    docShell = do_QueryInterface(primaryContent);
    if (!docShell) {
      NS_WARNING("failed to get docshell from primary content item");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIWebNavigation> webNav;
    webNav = do_GetInterface(docShell);
    if (!webNav) {
      NS_WARNING("failed to get web nav from inner docshell");
      return NS_ERROR_FAILURE;
    }

    rv = webNav->LoadURI(url.get(),
                         nsIWebNavigation::LOAD_FLAGS_NONE,
                         nsnull,
                         nsnull,
                         nsnull);

  }

  return rv;
}

nsresult
XRemoteService::XfeDoCommand(nsCString &aArgument,
                             nsIDOMWindow *aParent)
{
  nsresult rv = NS_OK;
  
  
  nsCString restArgument;
  PRUint32  index;
  FindRestInList(aArgument, restArgument, &index);

  if (!restArgument.IsEmpty())
    aArgument.Truncate(index);
  nsCOMPtr<nsISupportsString> arg;
  arg = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  
  if (NS_FAILED(rv))
    return rv;
  
  
  arg->SetData(NS_ConvertUTF8toUTF16(restArgument));

  
  if (aArgument.LowerCaseEqualsLiteral("openinbox")) {

    
    nsCOMPtr<nsIDOMWindowInternal> domWindow;

    rv = FindWindow(NS_LITERAL_STRING("mail:3pane").get(),
                    getter_AddRefs(domWindow));

    if (NS_FAILED(rv))
      return rv;

    
    if (domWindow) {
      domWindow->Focus();
    }

    
    else {
      
      nsXPIDLCString mailLocation;
      GetMailLocation(getter_Copies(mailLocation));
      if (!mailLocation)
	return NS_ERROR_FAILURE;

      nsCOMPtr<nsIDOMWindow> newWindow;
      rv = OpenChromeWindow(0, mailLocation, "chrome,all,dialog=no",
                            arg, getter_AddRefs(newWindow));
    }
  }

  
  else if (aArgument.LowerCaseEqualsLiteral("openbrowser")) {
    
    nsCOMPtr<nsICmdLineHandler> browserHandler =
        do_GetService("@mozilla.org/commandlinehandler/general-startup;1?type=browser");

    if (!browserHandler)
        return NS_ERROR_FAILURE;

    nsXPIDLCString browserLocation;
    browserHandler->GetChromeUrlForTask(getter_Copies(browserLocation));

    nsXPIDLString startPage;
    browserHandler->GetDefaultArgs(getter_Copies(startPage));

    arg->SetData(startPage);

    nsCOMPtr<nsIDOMWindow> newWindow;
    rv = OpenChromeWindow(0, browserLocation, "chrome,all,dialog=no",
                          arg, getter_AddRefs(newWindow));
  }

  
  else if (aArgument.LowerCaseEqualsLiteral("composemessage")) {
    



    const char * composeLocation;
    rv = GetComposeLocation(&composeLocation);
    if (rv != NS_OK)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMWindow> newWindow;
    rv = OpenChromeWindow(0, composeLocation, "chrome,all,dialog=no",
                          arg, getter_AddRefs(newWindow));
  }

  
  else if (aArgument.LowerCaseEqualsLiteral("opencalendar")) {

    
    nsCOMPtr<nsIDOMWindowInternal> aWindow;

    rv = FindWindow(NS_LITERAL_STRING("calendarMainWindow").get(),
		    getter_AddRefs(aWindow));

    if (NS_FAILED(rv))
      return rv;

    
    if (aWindow) {
      aWindow->Focus();
    }

    
    else {
      nsXPIDLCString calendarChrome;
      rv = GetCalendarLocation(getter_Copies(calendarChrome));
      if (NS_FAILED(rv))
        return rv;

      nsCOMPtr<nsIDOMWindow> newWindow;
      rv = OpenChromeWindow(0, calendarChrome, "chrome,all,dialog=no",
                            arg, getter_AddRefs(newWindow));
    }
  }

  return rv;
}

nsresult
XRemoteService::FindWindow(const PRUnichar *aType,
			   nsIDOMWindowInternal **_retval)
{
  nsCOMPtr<nsIWindowMediator> mediator;
  mediator = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);

  if (!mediator)
    return NS_ERROR_FAILURE;

  return mediator->GetMostRecentWindow(aType, _retval);
}

NS_GENERIC_FACTORY_CONSTRUCTOR(XRemoteService)

static const nsModuleComponentInfo components[] = {
  { "XRemoteService",
    NS_XREMOTESERVICE_CID,
    "@mozilla.org/browser/xremoteservice;2",
    XRemoteServiceConstructor }
};

NS_IMPL_NSGETMODULE(XRemoteServiceModule, components)

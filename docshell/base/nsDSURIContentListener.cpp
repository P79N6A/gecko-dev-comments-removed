





#include "nsDocShell.h"
#include "nsDSURIContentListener.h"
#include "nsIChannel.h"
#include "nsServiceManagerUtils.h"
#include "nsDocShellCID.h"
#include "nsIWebNavigationInfo.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsQueryObject.h"
#include "nsIHttpChannel.h"
#include "nsIScriptSecurityManager.h"
#include "nsError.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsDocShellLoadTypes.h"
#include "nsIMultiPartChannel.h"

using namespace mozilla;





nsDSURIContentListener::nsDSURIContentListener(nsDocShell* aDocShell)
  : mDocShell(aDocShell)
  , mExistingJPEGRequest(nullptr)
  , mParentContentListener(nullptr)
{
}

nsDSURIContentListener::~nsDSURIContentListener()
{
}

nsresult
nsDSURIContentListener::Init()
{
  nsresult rv;
  mNavInfo = do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get webnav info");
  return rv;
}





NS_IMPL_ADDREF(nsDSURIContentListener)
NS_IMPL_RELEASE(nsDSURIContentListener)

NS_INTERFACE_MAP_BEGIN(nsDSURIContentListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIURIContentListener)
  NS_INTERFACE_MAP_ENTRY(nsIURIContentListener)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsDSURIContentListener::OnStartURIOpen(nsIURI* aURI, bool* aAbortOpen)
{
  
  
  
  if (!mDocShell) {
    *aAbortOpen = true;
    return NS_OK;
  }

  nsCOMPtr<nsIURIContentListener> parentListener;
  GetParentContentListener(getter_AddRefs(parentListener));
  if (parentListener) {
    return parentListener->OnStartURIOpen(aURI, aAbortOpen);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::DoContent(const nsACString& aContentType,
                                  bool aIsContentPreferred,
                                  nsIRequest* aRequest,
                                  nsIStreamListener** aContentHandler,
                                  bool* aAbortProcess)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aContentHandler);
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  
  
  
  if (!CheckFrameOptions(aRequest)) {
    *aAbortProcess = true;
    return NS_OK;
  }

  *aAbortProcess = false;

  
  nsLoadFlags loadFlags = 0;
  nsCOMPtr<nsIChannel> aOpenedChannel = do_QueryInterface(aRequest);

  if (aOpenedChannel) {
    aOpenedChannel->GetLoadFlags(&loadFlags);
  }

  if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI) {
    
    mDocShell->Stop(nsIWebNavigation::STOP_NETWORK);

    mDocShell->SetLoadType(aIsContentPreferred ? LOAD_LINK : LOAD_NORMAL);
  }

  
  
  
  nsCOMPtr<nsIChannel> baseChannel;
  if (nsCOMPtr<nsIMultiPartChannel> mpchan = do_QueryInterface(aRequest)) {
    mpchan->GetBaseChannel(getter_AddRefs(baseChannel));
  }

  bool reuseCV = baseChannel && baseChannel == mExistingJPEGRequest &&
                 aContentType.EqualsLiteral("image/jpeg");

  if (mExistingJPEGStreamListener && reuseCV) {
    nsRefPtr<nsIStreamListener> copy(mExistingJPEGStreamListener);
    copy.forget(aContentHandler);
    rv = NS_OK;
  } else {
    rv = mDocShell->CreateContentViewer(aContentType, aRequest, aContentHandler);
    if (NS_SUCCEEDED(rv) && reuseCV) {
      mExistingJPEGStreamListener = *aContentHandler;
    } else {
      mExistingJPEGStreamListener = nullptr;
    }
    mExistingJPEGRequest = baseChannel;
  }

  if (rv == NS_ERROR_REMOTE_XUL) {
    aRequest->Cancel(rv);
    *aAbortProcess = true;
    return NS_OK;
  }

  if (NS_FAILED(rv)) {
    
    *aContentHandler = nullptr;
    return rv;
  }

  if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI) {
    nsCOMPtr<nsIDOMWindow> domWindow =
      mDocShell ? mDocShell->GetWindow() : nullptr;
    NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);
    domWindow->Focus();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::IsPreferred(const char* aContentType,
                                    char** aDesiredContentType,
                                    bool* aCanHandle)
{
  NS_ENSURE_ARG_POINTER(aCanHandle);
  NS_ENSURE_ARG_POINTER(aDesiredContentType);

  
  

  nsCOMPtr<nsIURIContentListener> parentListener;
  GetParentContentListener(getter_AddRefs(parentListener));
  if (parentListener) {
    return parentListener->IsPreferred(aContentType,
                                       aDesiredContentType,
                                       aCanHandle);
  }
  
  
  
  
  
  
  
  
  
  
  
  return CanHandleContent(aContentType, true, aDesiredContentType, aCanHandle);
}

NS_IMETHODIMP
nsDSURIContentListener::CanHandleContent(const char* aContentType,
                                         bool aIsContentPreferred,
                                         char** aDesiredContentType,
                                         bool* aCanHandleContent)
{
  NS_PRECONDITION(aCanHandleContent, "Null out param?");
  NS_ENSURE_ARG_POINTER(aDesiredContentType);

  *aCanHandleContent = false;
  *aDesiredContentType = nullptr;

  nsresult rv = NS_OK;
  if (aContentType) {
    uint32_t canHandle = nsIWebNavigationInfo::UNSUPPORTED;
    rv = mNavInfo->IsTypeSupported(nsDependentCString(aContentType),
                                   mDocShell,
                                   &canHandle);
    *aCanHandleContent = (canHandle != nsIWebNavigationInfo::UNSUPPORTED);
  }

  return rv;
}

NS_IMETHODIMP
nsDSURIContentListener::GetLoadCookie(nsISupports** aLoadCookie)
{
  NS_IF_ADDREF(*aLoadCookie = nsDocShell::GetAsSupports(mDocShell));
  return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::SetLoadCookie(nsISupports* aLoadCookie)
{
#ifdef DEBUG
  nsRefPtr<nsDocLoader> cookieAsDocLoader =
    nsDocLoader::GetAsDocLoader(aLoadCookie);
  NS_ASSERTION(cookieAsDocLoader && cookieAsDocLoader == mDocShell,
               "Invalid load cookie being set!");
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::GetParentContentListener(
    nsIURIContentListener** aParentListener)
{
  if (mWeakParentContentListener) {
    nsCOMPtr<nsIURIContentListener> tempListener =
      do_QueryReferent(mWeakParentContentListener);
    *aParentListener = tempListener;
    NS_IF_ADDREF(*aParentListener);
  } else {
    *aParentListener = mParentContentListener;
    NS_IF_ADDREF(*aParentListener);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::SetParentContentListener(
    nsIURIContentListener* aParentListener)
{
  if (aParentListener) {
    
    
    mParentContentListener = nullptr;
    mWeakParentContentListener = do_GetWeakReference(aParentListener);
    if (!mWeakParentContentListener) {
      mParentContentListener = aParentListener;
    }
  } else {
    mWeakParentContentListener = nullptr;
    mParentContentListener = nullptr;
  }
  return NS_OK;
}

bool
nsDSURIContentListener::CheckOneFrameOptionsPolicy(nsIHttpChannel* aHttpChannel,
                                                   const nsAString& aPolicy)
{
  static const char allowFrom[] = "allow-from";
  const uint32_t allowFromLen = ArrayLength(allowFrom) - 1;
  bool isAllowFrom =
    StringHead(aPolicy, allowFromLen).LowerCaseEqualsLiteral(allowFrom);

  
  if (!aPolicy.LowerCaseEqualsLiteral("deny") &&
      !aPolicy.LowerCaseEqualsLiteral("sameorigin") &&
      !isAllowFrom) {
    return true;
  }

  nsCOMPtr<nsIURI> uri;
  aHttpChannel->GetURI(getter_AddRefs(uri));

  
  if (!mDocShell) {
    return true;
  }

  
  
  
  
  nsCOMPtr<nsIDOMWindow> thisWindow = mDocShell->GetWindow();
  
  if (!thisWindow) {
    return true;
  }

  
  
  nsCOMPtr<nsIDOMWindow> topWindow;
  thisWindow->GetScriptableTop(getter_AddRefs(topWindow));

  
  if (thisWindow == topWindow) {
    return true;
  }

  
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> thisDocShellItem(
    do_QueryInterface(static_cast<nsIDocShell*>(mDocShell)));
  nsCOMPtr<nsIDocShellTreeItem> parentDocShellItem;
  nsCOMPtr<nsIDocShellTreeItem> curDocShellItem = thisDocShellItem;
  nsCOMPtr<nsIDocument> topDoc;
  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> ssm =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  if (!ssm) {
    MOZ_CRASH();
  }

  
  
  
  while (NS_SUCCEEDED(
           curDocShellItem->GetParent(getter_AddRefs(parentDocShellItem))) &&
         parentDocShellItem) {
    nsCOMPtr<nsIDocShell> curDocShell = do_QueryInterface(curDocShellItem);
    if (curDocShell && curDocShell->GetIsBrowserOrApp()) {
      break;
    }

    bool system = false;
    topDoc = parentDocShellItem->GetDocument();
    if (topDoc) {
      if (NS_SUCCEEDED(
            ssm->IsSystemPrincipal(topDoc->NodePrincipal(), &system)) &&
          system) {
        
        break;
      }
    } else {
      return false;
    }
    curDocShellItem = parentDocShellItem;
  }

  
  
  if (curDocShellItem == thisDocShellItem) {
    return true;
  }

  
  
  
  if (aPolicy.LowerCaseEqualsLiteral("deny")) {
    ReportXFOViolation(curDocShellItem, uri, eDENY);
    return false;
  }

  topDoc = curDocShellItem->GetDocument();
  nsCOMPtr<nsIURI> topUri;
  topDoc->NodePrincipal()->GetURI(getter_AddRefs(topUri));

  
  
  if (aPolicy.LowerCaseEqualsLiteral("sameorigin")) {
    rv = ssm->CheckSameOriginURI(uri, topUri, true);
    if (NS_FAILED(rv)) {
      ReportXFOViolation(curDocShellItem, uri, eSAMEORIGIN);
      return false; 
    }
  }

  
  
  if (isAllowFrom) {
    if (aPolicy.Length() == allowFromLen ||
        (aPolicy[allowFromLen] != ' ' &&
         aPolicy[allowFromLen] != '\t')) {
      ReportXFOViolation(curDocShellItem, uri, eALLOWFROM);
      return false;
    }
    rv = NS_NewURI(getter_AddRefs(uri), Substring(aPolicy, allowFromLen));
    if (NS_FAILED(rv)) {
      return false;
    }

    rv = ssm->CheckSameOriginURI(uri, topUri, true);
    if (NS_FAILED(rv)) {
      ReportXFOViolation(curDocShellItem, uri, eALLOWFROM);
      return false;
    }
  }

  return true;
}




bool
nsDSURIContentListener::CheckFrameOptions(nsIRequest* aRequest)
{
  nsresult rv;
  nsCOMPtr<nsIChannel> chan = do_QueryInterface(aRequest);
  if (!chan) {
    return true;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(chan);
  if (!httpChannel) {
    
    rv = mDocShell->GetHttpChannel(chan, getter_AddRefs(httpChannel));
    if (NS_FAILED(rv)) {
      return false;
    }
  }

  if (!httpChannel) {
    return true;
  }

  nsAutoCString xfoHeaderCValue;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("X-Frame-Options"),
                                 xfoHeaderCValue);
  NS_ConvertUTF8toUTF16 xfoHeaderValue(xfoHeaderCValue);

  
  if (xfoHeaderValue.IsEmpty()) {
    return true;
  }

  
  
  nsCharSeparatedTokenizer tokenizer(xfoHeaderValue, ',');
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& tok = tokenizer.nextToken();
    if (!CheckOneFrameOptionsPolicy(httpChannel, tok)) {
      
      httpChannel->Cancel(NS_BINDING_ABORTED);
      if (mDocShell) {
        nsCOMPtr<nsIWebNavigation> webNav(do_QueryObject(mDocShell));
        if (webNav) {
          webNav->LoadURI(MOZ_UTF16("about:blank"),
                          0, nullptr, nullptr, nullptr);
        }
      }
      return false;
    }
  }

  return true;
}

void
nsDSURIContentListener::ReportXFOViolation(nsIDocShellTreeItem* aTopDocShellItem,
                                           nsIURI* aThisURI,
                                           XFOHeader aHeader)
{
  MOZ_ASSERT(aTopDocShellItem, "Need a top docshell");

  nsCOMPtr<nsPIDOMWindow> topOuterWindow = aTopDocShellItem->GetWindow();
  if (!topOuterWindow) {
    return;
  }

  NS_ASSERTION(topOuterWindow->IsOuterWindow(), "Huh?");
  nsPIDOMWindow* topInnerWindow = topOuterWindow->GetCurrentInnerWindow();
  if (!topInnerWindow) {
    return;
  }

  nsCOMPtr<nsIURI> topURI;

  nsCOMPtr<nsIDocument> document = aTopDocShellItem->GetDocument();
  nsresult rv = document->NodePrincipal()->GetURI(getter_AddRefs(topURI));
  if (NS_FAILED(rv)) {
    return;
  }

  if (!topURI) {
    return;
  }

  nsCString topURIString;
  nsCString thisURIString;

  rv = topURI->GetSpec(topURIString);
  if (NS_FAILED(rv)) {
    return;
  }

  rv = aThisURI->GetSpec(thisURIString);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIConsoleService> consoleService =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID);
  nsCOMPtr<nsIScriptError> errorObject =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);

  if (!consoleService || !errorObject) {
    return;
  }

  nsString msg = NS_LITERAL_STRING("Load denied by X-Frame-Options: ");
  msg.Append(NS_ConvertUTF8toUTF16(thisURIString));

  switch (aHeader) {
    case eDENY:
      msg.AppendLiteral(" does not permit framing.");
      break;
    case eSAMEORIGIN:
      msg.AppendLiteral(" does not permit cross-origin framing.");
      break;
    case eALLOWFROM:
      msg.AppendLiteral(" does not permit framing by ");
      msg.Append(NS_ConvertUTF8toUTF16(topURIString));
      msg.Append('.');
      break;
  }

  rv = errorObject->InitWithWindowID(msg, EmptyString(), EmptyString(), 0, 0,
                                     nsIScriptError::errorFlag,
                                     "X-Frame-Options",
                                     topInnerWindow->WindowID());
  if (NS_FAILED(rv)) {
    return;
  }

  consoleService->LogMessage(errorObject);
}

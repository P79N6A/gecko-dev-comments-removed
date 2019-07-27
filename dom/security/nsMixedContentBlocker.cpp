




#include "nsMixedContentBlocker.h"

#include "nsContentPolicyUtils.h"
#include "nsThreadUtils.h"
#include "nsINode.h"
#include "nsCOMPtr.h"
#include "nsIDocShell.h"
#include "nsISecurityEventSink.h"
#include "nsIWebProgressListener.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsIRequest.h"
#include "nsIDocument.h"
#include "nsIContentViewer.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIParentChannel.h"
#include "mozilla/Preferences.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsISecureBrowserUI.h"
#include "nsIDocumentLoader.h"
#include "nsIWebNavigation.h"
#include "nsLoadGroup.h"
#include "nsIScriptError.h"
#include "nsIURI.h"
#include "nsIChannelEventSink.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "mozilla/LoadInfo.h"

#include "prlog.h"

using namespace mozilla;

enum nsMixedContentBlockerMessageType {
  eBlocked = 0x00,
  eUserOverride = 0x01
};



bool nsMixedContentBlocker::sBlockMixedScript = false;


bool nsMixedContentBlocker::sBlockMixedDisplay = false;




class nsMixedContentEvent : public nsRunnable
{
public:
  nsMixedContentEvent(nsISupports *aContext, MixedContentTypes aType)
    : mContext(aContext), mType(aType)
  {}

  NS_IMETHOD Run()
  {
    NS_ASSERTION(mContext,
                 "You can't call this runnable without a requesting context");

    
    
    
    


    
    
    
    nsCOMPtr<nsIDocShell> docShell = NS_CP_GetDocShellFromContext(mContext);
    if (!docShell) {
        return NS_OK;
    }
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShell->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    NS_ASSERTION(sameTypeRoot, "No document shell root tree item from document shell tree item!");

    
    nsCOMPtr<nsIDocument> rootDoc = do_GetInterface(sameTypeRoot);
    NS_ASSERTION(rootDoc, "No root document from document shell root tree item.");


    if (mType == eMixedScript) {
       
       if (rootDoc->GetHasMixedActiveContentLoaded()) {
         return NS_OK;
       }
       rootDoc->SetHasMixedActiveContentLoaded(true);

      
      nsCOMPtr<nsISecurityEventSink> eventSink = do_QueryInterface(docShell);
      if (eventSink) {
        
        if (rootDoc->GetHasMixedDisplayContentLoaded()) {
          eventSink->OnSecurityChange(mContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
        } else {
          eventSink->OnSecurityChange(mContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT));
        }
      }

    } else if (mType == eMixedDisplay) {
      
      if (rootDoc->GetHasMixedDisplayContentLoaded()) {
        return NS_OK;
      }
      rootDoc->SetHasMixedDisplayContentLoaded(true);

      
      nsCOMPtr<nsISecurityEventSink> eventSink = do_QueryInterface(docShell);
      if (eventSink) {
        
        if (rootDoc->GetHasMixedActiveContentLoaded()) {
          eventSink->OnSecurityChange(mContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT));
        } else {
          eventSink->OnSecurityChange(mContext, (nsIWebProgressListener::STATE_IS_BROKEN |
          nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
        }
      }
    }

    return NS_OK;
  }
private:
  
  
  nsCOMPtr<nsISupports> mContext;

  
  const MixedContentTypes mType;
};


nsMixedContentBlocker::nsMixedContentBlocker()
{
  
  Preferences::AddBoolVarCache(&sBlockMixedScript,
                               "security.mixed_content.block_active_content");

  
  Preferences::AddBoolVarCache(&sBlockMixedDisplay,
                               "security.mixed_content.block_display_content");
}

nsMixedContentBlocker::~nsMixedContentBlocker()
{
}

NS_IMPL_ISUPPORTS(nsMixedContentBlocker, nsIContentPolicy, nsIChannelEventSink)

static void
LogMixedContentMessage(MixedContentTypes aClassification,
                       nsIURI* aContentLocation,
                       nsIDocument* aRootDoc,
                       nsMixedContentBlockerMessageType aMessageType)
{
  nsAutoCString messageCategory;
  uint32_t severityFlag;
  nsAutoCString messageLookupKey;

  if (aMessageType == eBlocked) {
    severityFlag = nsIScriptError::errorFlag;
    messageCategory.AssignLiteral("Mixed Content Blocker");
    if (aClassification == eMixedDisplay) {
      messageLookupKey.AssignLiteral("BlockMixedDisplayContent");
    } else {
      messageLookupKey.AssignLiteral("BlockMixedActiveContent");
    }
  } else {
    severityFlag = nsIScriptError::warningFlag;
    messageCategory.AssignLiteral("Mixed Content Message");
    if (aClassification == eMixedDisplay) {
      messageLookupKey.AssignLiteral("LoadingMixedDisplayContent2");
    } else {
      messageLookupKey.AssignLiteral("LoadingMixedActiveContent2");
    }
  }

  nsAutoCString locationSpec;
  aContentLocation->GetSpec(locationSpec);
  NS_ConvertUTF8toUTF16 locationSpecUTF16(locationSpec);

  const char16_t* strings[] = { locationSpecUTF16.get() };
  nsContentUtils::ReportToConsole(severityFlag, messageCategory, aRootDoc,
                                  nsContentUtils::eSECURITY_PROPERTIES,
                                  messageLookupKey.get(), strings, ArrayLength(strings));
}








NS_IMETHODIMP
nsMixedContentBlocker::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                              nsIChannel* aNewChannel,
                                              uint32_t aFlags,
                                              nsIAsyncVerifyRedirectCallback* aCallback)
{
  nsAsyncRedirectAutoCallback autoCallback(aCallback);

  if (!aOldChannel) {
    NS_ERROR("No channel when evaluating mixed content!");
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  nsCOMPtr<nsIParentChannel> is_ipc_channel;
  NS_QueryNotificationCallbacks(aNewChannel, is_ipc_channel);
  if (is_ipc_channel) {
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIURI> oldUri;
  rv = aOldChannel->GetURI(getter_AddRefs(oldUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> newUri;
  rv = aNewChannel->GetURI(getter_AddRefs(newUri));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsILoadInfo> loadInfo;
  rv = aOldChannel->GetLoadInfo(getter_AddRefs(loadInfo));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!loadInfo) {
    
    
    
    
    
    
    return NS_OK;
  }

  uint32_t contentPolicyType = loadInfo->GetContentPolicyType();
  nsCOMPtr<nsIPrincipal> requestingPrincipal = loadInfo->LoadingPrincipal();

  
  
  
  nsCOMPtr<nsIURI> requestingLocation;
  if (requestingPrincipal) {
    
    
    if (nsContentUtils::IsSystemPrincipal(requestingPrincipal)) {
      return NS_OK;
    }
    
    rv = requestingPrincipal->GetURI(getter_AddRefs(requestingLocation));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  int16_t decision = REJECT_REQUEST;
  rv = ShouldLoad(contentPolicyType,
                  newUri,
                  requestingLocation,
                  loadInfo->LoadingNode(),
                  EmptyCString(),       
                  nullptr,              
                  requestingPrincipal,
                  &decision);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!NS_CP_ACCEPTED(decision)) {
    autoCallback.DontCallback();
    return NS_BINDING_FAILED;
  }

  return NS_OK;
}





NS_IMETHODIMP
nsMixedContentBlocker::ShouldLoad(uint32_t aContentType,
                                  nsIURI* aContentLocation,
                                  nsIURI* aRequestingLocation,
                                  nsISupports* aRequestingContext,
                                  const nsACString& aMimeGuess,
                                  nsISupports* aExtra,
                                  nsIPrincipal* aRequestPrincipal,
                                  int16_t* aDecision)
{
  
  
  
  
  nsresult rv = ShouldLoad(false,   
                           aContentType,
                           aContentLocation,
                           aRequestingLocation,
                           aRequestingContext,
                           aMimeGuess,
                           aExtra,
                           aRequestPrincipal,
                           aDecision);
  return rv;
}




nsresult
nsMixedContentBlocker::ShouldLoad(bool aHadInsecureImageRedirect,
                                  uint32_t aContentType,
                                  nsIURI* aContentLocation,
                                  nsIURI* aRequestingLocation,
                                  nsISupports* aRequestingContext,
                                  const nsACString& aMimeGuess,
                                  nsISupports* aExtra,
                                  nsIPrincipal* aRequestPrincipal,
                                  int16_t* aDecision)
{
  
  
  
  MOZ_ASSERT(NS_IsMainThread());

  
  MixedContentTypes classification = eMixedScript;
  
  *aDecision = REJECT_REQUEST;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  static_assert(TYPE_DATAREQUEST == TYPE_XMLHTTPREQUEST,
                "TYPE_DATAREQUEST is not a synonym for "
                "TYPE_XMLHTTPREQUEST");

  switch (aContentType) {
    
    case TYPE_DOCUMENT:
      *aDecision = ACCEPT;
      return NS_OK;
    
    
    
    case TYPE_WEBSOCKET:
      *aDecision = ACCEPT;
      return NS_OK;


    
    
    case TYPE_IMAGE:
    case TYPE_MEDIA:
    case TYPE_OBJECT_SUBREQUEST:
    case TYPE_PING:
    case TYPE_BEACON:
      classification = eMixedDisplay;
      break;

    
    
    
    case TYPE_IMAGESET:
    case TYPE_CSP_REPORT:
    case TYPE_DTD:
    case TYPE_FETCH:
    case TYPE_FONT:
    case TYPE_OBJECT:
    case TYPE_SCRIPT:
    case TYPE_STYLESHEET:
    case TYPE_SUBDOCUMENT:
    case TYPE_XBL:
    case TYPE_XMLHTTPREQUEST:
    case TYPE_XSLT:
    case TYPE_OTHER:
      break;


    
    default:
      MOZ_ASSERT(false, "Mixed content of unknown type");
      break;
  }

 
















  bool schemeLocal = false;
  bool schemeNoReturnData = false;
  bool schemeInherits = false;
  bool schemeSecure = false;
  if (NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_IS_LOCAL_RESOURCE , &schemeLocal))  ||
      NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA, &schemeNoReturnData)) ||
      NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT, &schemeInherits)) ||
      NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_SAFE_TO_LOAD_IN_SECURE_CONTEXT, &schemeSecure))) {
    *aDecision = REJECT_REQUEST;
    return NS_ERROR_FAILURE;
  }
  
  
  
  
  if (!aHadInsecureImageRedirect &&
      (schemeLocal || schemeNoReturnData || schemeInherits || schemeSecure)) {
    *aDecision = ACCEPT;
     return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIPrincipal> principal;
  
  nsCOMPtr<nsINode> node = do_QueryInterface(aRequestingContext);
  if (node) {
    principal = node->NodePrincipal();
  }

  
  if (!principal) {
    nsCOMPtr<nsIScriptObjectPrincipal> scriptObjPrin = do_QueryInterface(aRequestingContext);
    if (scriptObjPrin) {
      principal = scriptObjPrin->GetPrincipal();
    }
  }

  nsCOMPtr<nsIURI> requestingLocation;
  if (principal) {
    principal->GetURI(getter_AddRefs(requestingLocation));
  }

  
  if (principal && !requestingLocation) {
    if (nsContentUtils::IsSystemPrincipal(principal)) {
      *aDecision = ACCEPT;
      return NS_OK;
    }
  }

  
  
  
  
  if (!requestingLocation) {
    requestingLocation = aRequestingLocation;
  }

  
  
  
  if (!principal && !requestingLocation && aRequestPrincipal) {
    nsCOMPtr<nsIExpandedPrincipal> expanded = do_QueryInterface(aRequestPrincipal);
    if (expanded) {
      *aDecision = ACCEPT;
      return NS_OK;
    }
  }

  
  
  if (!requestingLocation) {
    *aDecision = REJECT_REQUEST;
    return NS_OK;
  }

  
  
  bool parentIsHttps;
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(requestingLocation);
  if (!innerURI) {
    NS_ERROR("Can't get innerURI from requestingLocation");
    *aDecision = REJECT_REQUEST;
    return NS_OK;
  }

  nsresult rv = innerURI->SchemeIs("https", &parentIsHttps);
  if (NS_FAILED(rv)) {
    NS_ERROR("requestingLocation->SchemeIs failed");
    *aDecision = REJECT_REQUEST;
    return NS_OK;
  }
  if (!parentIsHttps) {
    *aDecision = ACCEPT;
    return NS_OK;
  }

  
  nsCOMPtr<nsIDocShell> docShell = NS_CP_GetDocShellFromContext(aRequestingContext);
  NS_ENSURE_TRUE(docShell, NS_OK);
  bool rootHasSecureConnection = false;
  bool allowMixedContent = false;
  bool isRootDocShell = false;
  rv = docShell->GetAllowMixedContentAndConnectionData(&rootHasSecureConnection, &allowMixedContent, &isRootDocShell);
  if (NS_FAILED(rv)) {
    *aDecision = REJECT_REQUEST;
    return rv;
  }


  
  nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
  docShell->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
  NS_ASSERTION(sameTypeRoot, "No root tree item from docshell!");

  
  
  
  if (aContentType == TYPE_SUBDOCUMENT && !rootHasSecureConnection) {

    bool httpsParentExists = false;

    nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
    parentTreeItem = docShell;

    while(!httpsParentExists && parentTreeItem) {
      nsCOMPtr<nsIWebNavigation> parentAsNav(do_QueryInterface(parentTreeItem));
      NS_ASSERTION(parentAsNav, "No web navigation object from parent's docshell tree item");
      nsCOMPtr<nsIURI> parentURI;

      parentAsNav->GetCurrentURI(getter_AddRefs(parentURI));
      if (!parentURI || NS_FAILED(parentURI->SchemeIs("https", &httpsParentExists))) {
        
        httpsParentExists = true;
        break;
      }

      
      
      if(sameTypeRoot == parentTreeItem) {
        break;
      }

      
      nsCOMPtr<nsIDocShellTreeItem> newParentTreeItem;
      parentTreeItem->GetSameTypeParent(getter_AddRefs(newParentTreeItem));
      parentTreeItem = newParentTreeItem;
    } 

    if (!httpsParentExists) {
      *aDecision = nsIContentPolicy::ACCEPT;
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIDocument> rootDoc = do_GetInterface(sameTypeRoot);
  NS_ASSERTION(rootDoc, "No root document from document shell root tree item.");

  
  nsCOMPtr<nsISecurityEventSink> eventSink = do_QueryInterface(docShell);
  NS_ASSERTION(eventSink, "No eventSink from docShell.");
  nsCOMPtr<nsIDocShell> rootShell = do_GetInterface(sameTypeRoot);
  NS_ASSERTION(rootShell, "No root docshell from document shell root tree item.");
  uint32_t State = nsIWebProgressListener::STATE_IS_BROKEN;
  nsCOMPtr<nsISecureBrowserUI> securityUI;
  rootShell->GetSecurityUI(getter_AddRefs(securityUI));
  
  
  if (!securityUI) {
    *aDecision = nsIContentPolicy::ACCEPT;
    return NS_OK;
  }
  nsresult stateRV = securityUI->GetState(&State);

  
  if (sBlockMixedDisplay && classification == eMixedDisplay) {
    if (allowMixedContent) {
      LogMixedContentMessage(classification, aContentLocation, rootDoc, eUserOverride);
      *aDecision = nsIContentPolicy::ACCEPT;
      
      
      if (rootDoc->GetHasMixedDisplayContentLoaded()) {
        return NS_OK;
      }
      rootDoc->SetHasMixedDisplayContentLoaded(true);

      if (rootHasSecureConnection) {
        if (rootDoc->GetHasMixedActiveContentLoaded()) {
          
          eventSink->OnSecurityChange(aRequestingContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT));
        } else {
          eventSink->OnSecurityChange(aRequestingContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
        }
      } else {
        
        
        if (NS_SUCCEEDED(stateRV)) {
          eventSink->OnSecurityChange(aRequestingContext, (State | nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
         }
      }
    } else {
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
      LogMixedContentMessage(classification, aContentLocation, rootDoc, eBlocked);
      if (!rootDoc->GetHasMixedDisplayContentBlocked() && NS_SUCCEEDED(stateRV)) {
        rootDoc->SetHasMixedDisplayContentBlocked(true);
        eventSink->OnSecurityChange(aRequestingContext, (State | nsIWebProgressListener::STATE_BLOCKED_MIXED_DISPLAY_CONTENT));
      }
    }
    return NS_OK;

  } else if (sBlockMixedScript && classification == eMixedScript) {
    
    
    if (allowMixedContent) {
      LogMixedContentMessage(classification, aContentLocation, rootDoc, eUserOverride);
      *aDecision = nsIContentPolicy::ACCEPT;
      
      if (rootDoc->GetHasMixedActiveContentLoaded()) {
        return NS_OK;
      }
      rootDoc->SetHasMixedActiveContentLoaded(true);

      if (rootHasSecureConnection) {
        
        if (rootDoc->GetHasMixedDisplayContentLoaded()) {
          
          eventSink->OnSecurityChange(aRequestingContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
        } else {
          eventSink->OnSecurityChange(aRequestingContext,
                                      (nsIWebProgressListener::STATE_IS_BROKEN |
                                       nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT));
        }
        return NS_OK;
      } else {
        
        
        if (NS_SUCCEEDED(stateRV)) {
          eventSink->OnSecurityChange(aRequestingContext, (State | nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT));
        }
        return NS_OK;
      }
    } else {
      
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
      LogMixedContentMessage(classification, aContentLocation, rootDoc, eBlocked);
      
      if (rootDoc->GetHasMixedActiveContentBlocked()) {
        return NS_OK;
      }
      rootDoc->SetHasMixedActiveContentBlocked(true);

      
      
      if (NS_SUCCEEDED(stateRV)) {
         eventSink->OnSecurityChange(aRequestingContext, (State | nsIWebProgressListener::STATE_BLOCKED_MIXED_ACTIVE_CONTENT));
      }
      return NS_OK;
    }

  } else {
    

    
    LogMixedContentMessage(classification, aContentLocation, rootDoc, eUserOverride);

    
    
    nsContentUtils::AddScriptRunner(
      new nsMixedContentEvent(aRequestingContext, classification));
    *aDecision = ACCEPT;
    return NS_OK;
  }

  *aDecision = REJECT_REQUEST;
  return NS_OK;
}

NS_IMETHODIMP
nsMixedContentBlocker::ShouldProcess(uint32_t aContentType,
                                     nsIURI* aContentLocation,
                                     nsIURI* aRequestingLocation,
                                     nsISupports* aRequestingContext,
                                     const nsACString& aMimeGuess,
                                     nsISupports* aExtra,
                                     nsIPrincipal* aRequestPrincipal,
                                     int16_t* aDecision)
{
  if (!aContentLocation) {
    
    if (aContentType == TYPE_OBJECT) {
       *aDecision = ACCEPT;
       return NS_OK;
    } else {
       *aDecision = REJECT_REQUEST;
       return NS_ERROR_FAILURE;
    }
  }

  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aRequestPrincipal,
                    aDecision);
}

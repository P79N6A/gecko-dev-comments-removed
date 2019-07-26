




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
#include "mozilla/Preferences.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsISecureBrowserUI.h"
#include "nsIDocumentLoader.h"
#include "nsLoadGroup.h"

#include "prlog.h"

using namespace mozilla;



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
          eventSink->OnSecurityChange(mContext, (nsIWebProgressListener::STATE_IS_BROKEN |
          nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT |
          nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
        } else {
          eventSink->OnSecurityChange(mContext, (nsIWebProgressListener::STATE_IS_BROKEN |
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
          eventSink->OnSecurityChange(mContext, (nsIWebProgressListener::STATE_IS_BROKEN |
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

NS_IMPL_ISUPPORTS1(nsMixedContentBlocker, nsIContentPolicy)

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
  
  
  
  MOZ_ASSERT(NS_IsMainThread());

  
  MixedContentTypes classification = eMixedScript;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  MOZ_STATIC_ASSERT(TYPE_DATAREQUEST == TYPE_XMLHTTPREQUEST,
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
      classification = eMixedDisplay;
      break;

    
    
    
    case TYPE_CSP_REPORT:
    case TYPE_DTD:
    case TYPE_FONT:
    case TYPE_OBJECT:
    case TYPE_SCRIPT:
    case TYPE_STYLESHEET:
    case TYPE_SUBDOCUMENT:
    case TYPE_XBL:
    case TYPE_XMLHTTPREQUEST:
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
    return NS_ERROR_FAILURE;
  }

  if (schemeLocal || schemeNoReturnData || schemeInherits || schemeSecure) {
     return NS_OK;
  }

  
  
  if (!aRequestingLocation) {
    if (!aRequestPrincipal) {
      
      
      nsCOMPtr<nsINode> node = do_QueryInterface(aRequestingContext);
      if (node) {
        aRequestPrincipal = node->NodePrincipal();
      } else {
        
        nsCOMPtr<nsIScriptObjectPrincipal> scriptObjPrin = do_QueryInterface(aRequestingContext);
        if (scriptObjPrin) {
          aRequestPrincipal = scriptObjPrin->GetPrincipal();
        }
      }
    }
    if (aRequestPrincipal) {
      nsCOMPtr<nsIURI> principalUri;
      nsresult rvalue = aRequestPrincipal->GetURI(getter_AddRefs(principalUri));
      if (NS_SUCCEEDED(rvalue)) {
        aRequestingLocation = principalUri;
      }
    }

    if (!aRequestingLocation) {
      
      
      
      nsCOMPtr<nsIExpandedPrincipal> expanded = do_QueryInterface(aRequestPrincipal);
      if (expanded || (aRequestPrincipal && nsContentUtils::IsSystemPrincipal(aRequestPrincipal))) {
        *aDecision = ACCEPT;
        return NS_OK;
      } else {
        
        
        *aDecision = REJECT_REQUEST;
        return NS_OK;
      }
    }
  }

  
  
  bool parentIsHttps;
  nsresult rv = aRequestingLocation->SchemeIs("https", &parentIsHttps);
  if (NS_FAILED(rv)) {
    NS_ERROR("aRequestingLocation->SchemeIs failed");
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
     return rv;
  }

  
  nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
  docShell->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
  NS_ASSERTION(sameTypeRoot, "No document shell root tree item from document shell tree item!");
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
      *aDecision = nsIContentPolicy::ACCEPT;
      rootDoc->SetHasMixedActiveContentLoaded(true);
      if (!rootDoc->GetHasMixedDisplayContentLoaded() && NS_SUCCEEDED(stateRV)) {
        eventSink->OnSecurityChange(aRequestingContext, (State | nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
      }
    } else {
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
      if (!rootDoc->GetHasMixedDisplayContentBlocked() && NS_SUCCEEDED(stateRV)) {
        eventSink->OnSecurityChange(aRequestingContext, (State | nsIWebProgressListener::STATE_BLOCKED_MIXED_DISPLAY_CONTENT));
      }
    }
    return NS_OK;

  } else if (sBlockMixedScript && classification == eMixedScript) {
    
    
    if (allowMixedContent) {
       *aDecision = nsIContentPolicy::ACCEPT;
       
       if (rootDoc->GetHasMixedActiveContentLoaded()) {
         return NS_OK;
       }
       rootDoc->SetHasMixedActiveContentLoaded(true);

       if (rootHasSecureConnection) {
         
         if (rootDoc->GetHasMixedDisplayContentLoaded()) {
           
           eventSink->OnSecurityChange(aRequestingContext, (nsIWebProgressListener::STATE_IS_BROKEN |
           nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT |
           nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT));
         } else {
           eventSink->OnSecurityChange(aRequestingContext, (nsIWebProgressListener::STATE_IS_BROKEN |
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
    

    
    
    nsContentUtils::AddScriptRunner(
      new nsMixedContentEvent(aRequestingContext, classification));
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
       return NS_OK;
    } else {
       return NS_ERROR_FAILURE;
    }
  }

  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aRequestPrincipal,
                    aDecision);
}

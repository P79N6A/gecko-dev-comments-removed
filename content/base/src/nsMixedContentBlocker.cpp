




#include "nsMixedContentBlocker.h"
#include "nsContentPolicyUtils.h"

#include "nsINode.h"
#include "nsCOMPtr.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
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
    nsCOMPtr<nsIDocShellTreeItem> currentDocShellTreeItem(do_QueryInterface(docShell));
    if(!currentDocShellTreeItem) {
        return NS_OK;
    }
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    currentDocShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    NS_ASSERTION(sameTypeRoot, "No document shell root tree item from document shell tree item!");

    
    nsCOMPtr<nsIDocument> rootDoc = do_GetInterface(sameTypeRoot);
    NS_ASSERTION(rootDoc, "No root document from document shell root tree item.");


    if(mType == eMixedScript) {
      rootDoc->SetHasMixedActiveContentLoaded(true);

      
      nsCOMPtr<nsISecurityEventSink> eventSink = do_QueryInterface(docShell);
      if (eventSink) {
        eventSink->OnSecurityChange(mContext, nsIWebProgressListener::STATE_IS_BROKEN);
      }

    } else {
        if(mType == eMixedDisplay) {
          
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
  
  *aDecision = nsIContentPolicy::ACCEPT;

  
  
  if (!sBlockMixedScript && !sBlockMixedDisplay) {
    return NS_OK;
  }

  
  
  if (aContentType == nsIContentPolicy::TYPE_DOCUMENT || aContentType == nsIContentPolicy::TYPE_WEBSOCKET) {
    return NS_OK;
  }

  
  
  if (!aRequestingLocation) {
    nsCOMPtr<nsINode> node = do_QueryInterface(aRequestingContext);
    if (node) {
      nsCOMPtr<nsIURI> principalUri;
      node->NodePrincipal()->GetURI(getter_AddRefs(principalUri));
      aRequestingLocation = principalUri;
    }
    
    
    if (!aRequestingLocation) {
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
      return NS_OK;
    }
  }

  
  
  bool parentIsHttps;
  if (NS_FAILED(aRequestingLocation->SchemeIs("https", &parentIsHttps)) ||
      !parentIsHttps) {
    return NS_OK;
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

  

  
  
  switch (aContentType) {
    case nsIContentPolicy::TYPE_FONT:
    case nsIContentPolicy::TYPE_OBJECT:
    case nsIContentPolicy::TYPE_SCRIPT:
    case nsIContentPolicy::TYPE_STYLESHEET:
    case nsIContentPolicy::TYPE_SUBDOCUMENT:
    case nsIContentPolicy::TYPE_WEBSOCKET:
    case nsIContentPolicy::TYPE_XMLHTTPREQUEST:
      
      
      
      if (sBlockMixedScript) {
        *aDecision = nsIContentPolicy::REJECT_REQUEST;

        
        
        
        



      }
      break;

    case nsIContentPolicy::TYPE_IMAGE:
    case nsIContentPolicy::TYPE_MEDIA:
    case nsIContentPolicy::TYPE_PING:
      
      
      if (sBlockMixedDisplay) {
        *aDecision = nsIContentPolicy::REJECT_REQUEST;

        
        
        
        



      }
      break;

    default:
      
      break;
  }

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

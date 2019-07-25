








































#include "HttpChannelParentListener.h"
#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/unused.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsNetUtil.h"
#include "nsISupportsPriority.h"
#include "nsIAuthPromptProvider.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBadCertListener2.h"
#include "nsICacheEntryDescriptor.h"
#include "nsSerializationHelper.h"
#include "nsISerializable.h"
#include "nsIAssociatedContentSecurity.h"
#include "nsISecureBrowserUI.h"

using mozilla::unused;

namespace mozilla {
namespace net {

HttpChannelParentListener::HttpChannelParentListener(HttpChannelParent* aInitialChannel)
  : mActiveChannel(aInitialChannel)
{
}

HttpChannelParentListener::~HttpChannelParentListener()
{
}





NS_IMPL_ISUPPORTS5(HttpChannelParentListener, 
                   nsIInterfaceRequestor,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIChannelEventSink,
                   nsIRedirectResultListener)





NS_IMETHODIMP
HttpChannelParentListener::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  if (!mActiveChannel)
    return NS_ERROR_UNEXPECTED;

  LOG(("HttpChannelParentListener::OnStartRequest [this=%x]\n", this));
  return mActiveChannel->OnStartRequest(aRequest, aContext);
}

NS_IMETHODIMP
HttpChannelParentListener::OnStopRequest(nsIRequest *aRequest, 
                                          nsISupports *aContext, 
                                          nsresult aStatusCode)
{
  if (!mActiveChannel)
    return NS_ERROR_UNEXPECTED;

  LOG(("HttpChannelParentListener::OnStopRequest: [this=%x status=%ul]\n", 
       this, aStatusCode));
  nsresult rv = mActiveChannel->OnStopRequest(aRequest, aContext, aStatusCode);

  mActiveChannel = nsnull;
  return rv;
}





NS_IMETHODIMP
HttpChannelParentListener::OnDataAvailable(nsIRequest *aRequest, 
                                            nsISupports *aContext, 
                                            nsIInputStream *aInputStream, 
                                            PRUint32 aOffset, 
                                            PRUint32 aCount)
{
  if (!mActiveChannel)
    return NS_ERROR_UNEXPECTED;

  LOG(("HttpChannelParentListener::OnDataAvailable [this=%x]\n", this));
  return mActiveChannel->OnDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount);
}





NS_IMETHODIMP 
HttpChannelParentListener::GetInterface(const nsIID& aIID, void **result)
{
  if (aIID.Equals(NS_GET_IID(nsIAuthPromptProvider))) {
    if (!mActiveChannel || !mActiveChannel->mTabParent)
      return NS_NOINTERFACE;
    return mActiveChannel->mTabParent->QueryInterface(aIID, result);
  }

  if (aIID.Equals(NS_GET_IID(nsISecureBrowserUI))) {
    if (!mActiveChannel || !mActiveChannel->mTabParent)
      return NS_NOINTERFACE;
    return mActiveChannel->mTabParent->QueryInterface(aIID, result);
  }

  if (aIID.Equals(NS_GET_IID(nsIProgressEventSink))) {
    if (!mActiveChannel)
      return NS_NOINTERFACE;
    return mActiveChannel->QueryInterface(aIID, result);
  }

  
  
  if (

      
      
      
      
      
      
      aIID.Equals(NS_GET_IID(nsIAuthPrompt2)) ||
      aIID.Equals(NS_GET_IID(nsIAuthPrompt))  ||
      
      
      
      
      
      aIID.Equals(NS_GET_IID(nsIChannelEventSink)) || 
      aIID.Equals(NS_GET_IID(nsIHttpEventSink))  ||
      aIID.Equals(NS_GET_IID(nsIRedirectResultListener))  ||
      
      aIID.Equals(NS_GET_IID(nsIApplicationCacheContainer)) ||
      
      aIID.Equals(NS_GET_IID(nsIDocShellTreeItem)) ||
      
      aIID.Equals(NS_GET_IID(nsIBadCertListener2))) 
  {
    return QueryInterface(aIID, result);
  } else {
    nsPrintfCString msg(2000, 
       "HttpChannelParentListener::GetInterface: interface UUID=%s not yet supported! "
       "Use 'grep -ri UUID <mozilla_src>' to find the name of the interface, "
       "check http://tinyurl.com/255ojvu to see if a bug has already been "
       "filed, and if not, add one and make it block bug 516730. Thanks!",
       aIID.ToString());
    NECKO_MAYBE_ABORT(msg);
    return NS_NOINTERFACE;
  }
}





NS_IMETHODIMP
HttpChannelParentListener::AsyncOnChannelRedirect(
                                    nsIChannel *oldChannel,
                                    nsIChannel *newChannel,
                                    PRUint32 redirectFlags,
                                    nsIAsyncVerifyRedirectCallback* callback)
{
  if (mActiveChannel->mIPCClosed)
    return NS_BINDING_ABORTED;

  
  PBrowserParent* browser = mActiveChannel->mTabParent ?
      static_cast<TabParent*>(mActiveChannel->mTabParent.get()) : nsnull;
  mRedirectChannel = static_cast<HttpChannelParent *>
      (mActiveChannel->Manager()->SendPHttpChannelConstructor(browser));

  
  mRedirectChannel->mChannel = newChannel;

  
  
  mRedirectChannel->mChannelListener = this;

  
  mRedirectCallback = callback;

  nsCOMPtr<nsIURI> newURI;
  newChannel->GetURI(getter_AddRefs(newURI));

  nsHttpChannel *oldHttpChannel = static_cast<nsHttpChannel *>(oldChannel);
  nsHttpResponseHead *responseHead = oldHttpChannel->GetResponseHead();

  
  
  unused << mActiveChannel->SendRedirect1Begin(mRedirectChannel,
                                               IPC::URI(newURI),
                                               redirectFlags,
                                               responseHead ? *responseHead 
                                                            : nsHttpResponseHead());

  
  

  return NS_OK;
}

void
HttpChannelParentListener::OnContentRedirectResultReceived(
                                const nsresult result,
                                const RequestHeaderTuples& changedHeaders)
{
  nsHttpChannel* newHttpChannel = 
      static_cast<nsHttpChannel*>(mRedirectChannel->mChannel.get());

  if (NS_SUCCEEDED(result)) {
    for (PRUint32 i = 0; i < changedHeaders.Length(); i++) {
      newHttpChannel->SetRequestHeader(changedHeaders[i].mHeader,
                                       changedHeaders[i].mValue,
                                       changedHeaders[i].mMerge);
    }
  }

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nsnull;
}





NS_IMETHODIMP
HttpChannelParentListener::OnRedirectResult(PRBool succeeded)
{
  if (!mRedirectChannel) {
    
    return NS_OK;
  }

  if (succeeded && !mActiveChannel->mIPCClosed) {
    
    unused << mActiveChannel->SendRedirect3Complete();
  }

  HttpChannelParent* channelToDelete;
  if (succeeded) {
    
    channelToDelete = mActiveChannel;
    mActiveChannel = mRedirectChannel;
  } else {
    
    channelToDelete = mRedirectChannel;
  }

  if (!channelToDelete->mIPCClosed)
    unused << channelToDelete->SendDeleteSelf();
  mRedirectChannel = nsnull;

  return NS_OK;
}

}} 

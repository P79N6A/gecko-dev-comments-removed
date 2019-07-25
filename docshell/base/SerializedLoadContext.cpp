





#include "SerializedLoadContext.h"
#include "nsNetUtil.h"
#include "nsIChannel.h"
#include "nsIWebSocketChannel.h"

namespace IPC {

SerializedLoadContext::SerializedLoadContext(nsILoadContext* aLoadContext)
{
  Init(aLoadContext);
}

SerializedLoadContext::SerializedLoadContext(nsIChannel* aChannel)
{
  nsCOMPtr<nsILoadContext> loadContext;
  NS_QueryNotificationCallbacks(aChannel, loadContext);
  Init(loadContext);
}

SerializedLoadContext::SerializedLoadContext(nsIWebSocketChannel* aChannel)
{
  nsCOMPtr<nsILoadContext> loadContext;
  NS_QueryNotificationCallbacks(aChannel, loadContext);
  Init(loadContext);
}

void
SerializedLoadContext::Init(nsILoadContext* aLoadContext)
{
  if (aLoadContext) {
    mIsNotNull = true;
    aLoadContext->GetIsContent(&mIsContent);
    aLoadContext->GetUsePrivateBrowsing(&mUsePrivateBrowsing);
    aLoadContext->GetAppId(&mAppId);
    aLoadContext->GetIsInBrowserElement(&mIsInBrowserElement);
  } else {
    mIsNotNull = false;
    
    
    mIsContent = true;
    mUsePrivateBrowsing = false;
    mAppId = 0;
    mIsInBrowserElement = false;
  }
}

} 


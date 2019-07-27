





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
  if (!aChannel) {
    Init(nullptr);
    return;
  }

  nsCOMPtr<nsILoadContext> loadContext;
  NS_QueryNotificationCallbacks(aChannel, loadContext);
  Init(loadContext);

  if (!loadContext) {
    
    
    bool isPrivate = false;
    bool isOverriden = false;
    nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(aChannel);
    if (pbChannel &&
        NS_SUCCEEDED(pbChannel->IsPrivateModeOverriden(&isPrivate,
                                                       &isOverriden)) &&
        isOverriden) {
      mUsePrivateBrowsing = isPrivate;
      mIsPrivateBitValid = true;
    }
  }
}

SerializedLoadContext::SerializedLoadContext(nsIWebSocketChannel* aChannel)
{
  nsCOMPtr<nsILoadContext> loadContext;
  if (aChannel) {
    NS_QueryNotificationCallbacks(aChannel, loadContext);
  }
  Init(loadContext);
}

void
SerializedLoadContext::Init(nsILoadContext* aLoadContext)
{
  if (aLoadContext) {
    mIsNotNull = true;
    mIsPrivateBitValid = true;
    aLoadContext->GetIsContent(&mIsContent);
    aLoadContext->GetUsePrivateBrowsing(&mUsePrivateBrowsing);
    aLoadContext->GetUseRemoteTabs(&mUseRemoteTabs);
    aLoadContext->GetAppId(&mAppId);
    aLoadContext->GetIsInBrowserElement(&mIsInBrowserElement);
  } else {
    mIsNotNull = false;
    mIsPrivateBitValid = false;
    
    
    mIsContent = true;
    mUsePrivateBrowsing = false;
    mUseRemoteTabs = false;
    mAppId = 0;
    mIsInBrowserElement = false;
  }
}

} 


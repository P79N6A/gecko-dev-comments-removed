





#include "mozilla/dom/ChannelInfo.h"
#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsSerializationHelper.h"
#include "mozilla/net/HttpBaseChannel.h"
#include "mozilla/ipc/ChannelInfo.h"
#include "nsIJARChannel.h"
#include "nsJARChannel.h"
#include "nsNetUtil.h"

using namespace mozilla;
using namespace mozilla::dom;

void
ChannelInfo::InitFromChannel(nsIChannel* aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mInited, "Cannot initialize the object twice");

  nsCOMPtr<nsISupports> securityInfo;
  aChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
  if (securityInfo) {
    SetSecurityInfo(securityInfo);
  }

  nsLoadFlags loadFlags = 0;
  aChannel->GetLoadFlags(&loadFlags);
  mRedirected = (loadFlags & nsIChannel::LOAD_REPLACE);
  if (mRedirected) {
    
    
    
    nsCOMPtr<nsIURI> redirectedURI;
    aChannel->GetURI(getter_AddRefs(redirectedURI));
    if (redirectedURI) {
      redirectedURI->GetSpec(mRedirectedURISpec);
    }
  }

  mInited = true;
}

void
ChannelInfo::InitFromIPCChannelInfo(const ipc::IPCChannelInfo& aChannelInfo)
{
  MOZ_ASSERT(!mInited, "Cannot initialize the object twice");

  mSecurityInfo = aChannelInfo.securityInfo();
  mRedirectedURISpec = aChannelInfo.redirectedURI();
  mRedirected = aChannelInfo.redirected();

  mInited = true;
}

void
ChannelInfo::SetSecurityInfo(nsISupports* aSecurityInfo)
{
  MOZ_ASSERT(mSecurityInfo.IsEmpty(), "security info should only be set once");
  nsCOMPtr<nsISerializable> serializable = do_QueryInterface(aSecurityInfo);
  if (!serializable) {
    NS_WARNING("A non-serializable object was passed to InternalResponse::SetSecurityInfo");
    return;
  }
  NS_SerializeToString(serializable, mSecurityInfo);
}

nsresult
ChannelInfo::ResurrectInfoOnChannel(nsIChannel* aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mInited);

  
  
  nsCOMPtr<nsIHttpChannel> httpChannel =
    do_QueryInterface(aChannel);
  nsCOMPtr<nsIJARChannel> jarChannel =
    do_QueryInterface(aChannel);

  if (!mSecurityInfo.IsEmpty()) {
    nsCOMPtr<nsISupports> infoObj;
    nsresult rv = NS_DeserializeObject(mSecurityInfo, getter_AddRefs(infoObj));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    if (httpChannel) {
      net::HttpBaseChannel* httpBaseChannel =
        static_cast<net::HttpBaseChannel*>(httpChannel.get());
      rv = httpBaseChannel->OverrideSecurityInfo(infoObj);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else {
      if (NS_WARN_IF(!jarChannel)) {
        return NS_ERROR_FAILURE;
      }
      static_cast<nsJARChannel*>(jarChannel.get())->
        OverrideSecurityInfo(infoObj);
    }
  }

  if (mRedirected) {
    nsLoadFlags flags = 0;
    aChannel->GetLoadFlags(&flags);
    flags |= nsIChannel::LOAD_REPLACE;
    aChannel->SetLoadFlags(flags);

    nsCOMPtr<nsIURI> redirectedURI;
    nsresult rv = NS_NewURI(getter_AddRefs(redirectedURI),
                            mRedirectedURISpec);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    if (httpChannel) {
      net::HttpBaseChannel* httpBaseChannel =
        static_cast<net::HttpBaseChannel*>(httpChannel.get());
      httpBaseChannel->OverrideURI(redirectedURI);
    } else {
      if (NS_WARN_IF(!jarChannel)) {
        return NS_ERROR_FAILURE;
      }
      static_cast<nsJARChannel*>(jarChannel.get())->OverrideURI(redirectedURI);
    }
  }

  return NS_OK;
}

ipc::IPCChannelInfo
ChannelInfo::AsIPCChannelInfo() const
{
  
  
  
  

  IPCChannelInfo ipcInfo;

  ipcInfo.securityInfo() = mSecurityInfo;
  ipcInfo.redirectedURI() = mRedirectedURISpec;
  ipcInfo.redirected() = mRedirected;

  return ipcInfo;
}

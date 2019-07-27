



#include "LoadContextInfo.h"

#include "nsIChannel.h"
#include "nsILoadContext.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(LoadContextInfo, nsILoadContextInfo)

LoadContextInfo::LoadContextInfo(bool aIsPrivate, uint32_t aAppId, bool aIsInBrowser, bool aIsAnonymous)
  : mAppId(aAppId)
  , mIsPrivate(aIsPrivate)
  , mIsInBrowser(aIsInBrowser)
  , mIsAnonymous(aIsAnonymous)
{
}

LoadContextInfo::~LoadContextInfo()
{
}

NS_IMETHODIMP LoadContextInfo::GetIsPrivate(bool *aIsPrivate)
{
  *aIsPrivate = mIsPrivate;
  return NS_OK;
}

NS_IMETHODIMP LoadContextInfo::GetAppId(uint32_t *aAppId)
{
  *aAppId = mAppId;
  return NS_OK;
}

NS_IMETHODIMP LoadContextInfo::GetIsInBrowserElement(bool *aIsInBrowser)
{
  *aIsInBrowser = mIsInBrowser;
  return NS_OK;
}

NS_IMETHODIMP LoadContextInfo::GetIsAnonymous(bool *aIsAnonymous)
{
  *aIsAnonymous = mIsAnonymous;
  return NS_OK;
}

LoadContextInfo *
GetLoadContextInfo(nsIChannel * aChannel)
{
  bool pb = NS_UsePrivateBrowsing(aChannel);
  uint32_t appId;
  bool ib;
  if (!NS_GetAppInfo(aChannel, &appId, &ib)) {
    appId = nsILoadContextInfo::NO_APP_ID;
    ib = false;
  }

  bool anon = false;
  nsLoadFlags loadFlags;
  nsresult rv = aChannel->GetLoadFlags(&loadFlags);
  if (NS_SUCCEEDED(rv))
    anon = !!(loadFlags & nsIChannel::LOAD_ANONYMOUS);

  return new LoadContextInfo(pb, appId, ib, anon);
}

LoadContextInfo *
GetLoadContextInfo(nsILoadContext * aLoadContext, bool aIsAnonymous)
{
  if (!aLoadContext)
    return new LoadContextInfo(false, nsILoadContextInfo::NO_APP_ID, false, aIsAnonymous); 

  bool pb = aLoadContext->UsePrivateBrowsing();

  bool ib;
  nsresult rv = aLoadContext->GetIsInBrowserElement(&ib);
  if (NS_FAILED(rv))
    ib = false; 

  uint32_t appId;
  rv = aLoadContext->GetAppId(&appId);
  if (NS_FAILED(rv))
    appId = nsILoadContextInfo::NO_APP_ID;

  return new LoadContextInfo(pb, appId, ib, aIsAnonymous);
}

LoadContextInfo *
GetLoadContextInfo(nsILoadContextInfo* aInfo)
{
  return new LoadContextInfo(aInfo->IsPrivate(),
                             aInfo->AppId(),
                             aInfo->IsInBrowserElement(),
                             aInfo->IsAnonymous());
}

LoadContextInfo *
GetLoadContextInfo(bool const aIsPrivate,
                   uint32_t const aAppId,
                   bool const aIsInBrowserElement,
                   bool const aIsAnonymous)
{
  return new LoadContextInfo(aIsPrivate,
                             aAppId,
                             aIsInBrowserElement,
                             aIsAnonymous);
}

} 
} 

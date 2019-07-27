





#include "mozilla/dom/cache/ManagerId.h"
#include "nsIPrincipal.h"
#include "nsProxyRelease.h"
#include "nsRefPtr.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {
namespace cache {


nsresult
ManagerId::Create(nsIPrincipal* aPrincipal, ManagerId** aManagerIdOut)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  
  
  
  
  
  
  

  nsAutoCString origin;
  nsresult rv = aPrincipal->GetOrigin(getter_Copies(origin));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  uint32_t appId;
  rv = aPrincipal->GetAppId(&appId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool inBrowserElement;
  rv = aPrincipal->GetIsInBrowserElement(&inBrowserElement);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsRefPtr<ManagerId> ref = new ManagerId(aPrincipal, origin, appId,
                                          inBrowserElement);
  ref.forget(aManagerIdOut);

  return NS_OK;
}

already_AddRefed<nsIPrincipal>
ManagerId::Principal() const
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIPrincipal> ref = mPrincipal;
  return ref.forget();
}

ManagerId::ManagerId(nsIPrincipal* aPrincipal, const nsACString& aOrigin,
                     uint32_t aAppId, bool aInBrowserElement)
    : mPrincipal(aPrincipal)
    , mOrigin(aOrigin)
    , mAppId(aAppId)
    , mInBrowserElement(aInBrowserElement)
{
  MOZ_ASSERT(mPrincipal);
}

ManagerId::~ManagerId()
{
  
  if (NS_IsMainThread()) {
    return;
  }

  

  
  
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  MOZ_ASSERT(mainThread);

  NS_ProxyRelease(mainThread, mPrincipal.forget().take());
}

} 
} 
} 

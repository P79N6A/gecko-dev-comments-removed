





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

  
  
  
  
  
  
  
  
  
  
  

  nsCString origin;
  nsresult rv = aPrincipal->GetOriginNoSuffix(origin);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCString jarPrefix;
  rv = aPrincipal->GetJarPrefix(jarPrefix);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsRefPtr<ManagerId> ref = new ManagerId(aPrincipal, origin, jarPrefix);
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
                     const nsACString& aJarPrefix)
    : mPrincipal(aPrincipal)
    , mExtendedOrigin(aJarPrefix + aOrigin)
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

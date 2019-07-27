





#include "mozilla/LoadInfo.h"

#include "mozilla/Assertions.h"
#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"

namespace mozilla {

LoadInfo::LoadInfo(nsIPrincipal* aPrincipal,
                   InheritType aInheritPrincipal,
                   SandboxType aSandboxed)
  : mPrincipal(aPrincipal)
  , mInheritPrincipal(aInheritPrincipal == eInheritPrincipal &&
                      aSandboxed != eSandboxed)
  , mSandboxed(aSandboxed == eSandboxed)
{
  MOZ_ASSERT(aPrincipal);
}

LoadInfo::~LoadInfo()
{
}

NS_IMPL_ISUPPORTS(LoadInfo, nsILoadInfo)

NS_IMETHODIMP
LoadInfo::GetLoadingPrincipal(nsIPrincipal** aPrincipal)
{
  NS_ADDREF(*aPrincipal = mPrincipal);
  return NS_OK;
}

nsIPrincipal*
LoadInfo::LoadingPrincipal()
{
  return mPrincipal;
}

NS_IMETHODIMP
LoadInfo::GetForceInheritPrincipal(bool* aInheritPrincipal)
{
  *aInheritPrincipal = mInheritPrincipal;
  return NS_OK;
}

NS_IMETHODIMP
LoadInfo::GetLoadingSandboxed(bool* aLoadingSandboxed)
{
  *aLoadingSandboxed = mSandboxed;
  return NS_OK;
}

} 

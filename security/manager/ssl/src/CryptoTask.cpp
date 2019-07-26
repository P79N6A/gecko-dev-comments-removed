





#include "CryptoTask.h"

namespace mozilla {

CryptoTask::~CryptoTask()
{
  MOZ_ASSERT(mReleasedNSSResources);

  nsNSSShutDownPreventionLock lock;
  if (!isAlreadyShutDown()) {
    shutdown(calledFromObject);
  }
}

nsresult
CryptoTask::Dispatch(const nsACString & taskThreadName)
{
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_NewThread(getter_AddRefs(thread), this);
  if (thread) {
    NS_SetThreadName(thread, taskThreadName);
  }
  return rv;
}

NS_IMETHODIMP
CryptoTask::Run()
{
  if (!NS_IsMainThread()) {
    nsNSSShutDownPreventionLock locker;
    if (isAlreadyShutDown()) {
      mRv = NS_ERROR_NOT_AVAILABLE;
    } else {
      mRv = CalculateResult();
    }
    NS_DispatchToMainThread(this);
  } else {
    

    
    
    
    if (!mReleasedNSSResources) {
      mReleasedNSSResources = true;
      ReleaseNSSResources();
    }

    CallCallback(mRv);
  }

  return NS_OK;
}

void
CryptoTask::virtualDestroyNSSReference()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(),
                    "virtualDestroyNSSReference called off the main thread");
  if (!mReleasedNSSResources) {
    mReleasedNSSResources = true;
    ReleaseNSSResources();
  }
}

} 

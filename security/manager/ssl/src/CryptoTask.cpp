





#include "CryptoTask.h"
#include "nsNSSComponent.h"

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
CryptoTask::Dispatch(const nsACString& taskThreadName)
{
  MOZ_ASSERT(taskThreadName.Length() <= 15);

  
  
  if (!EnsureNSSInitializedChromeOrContent()) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv = NS_NewThread(getter_AddRefs(mThread), nullptr,
                             nsIThreadManager::DEFAULT_STACK_SIZE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_SetThreadName(mThread, taskThreadName);
  
  return mThread->Dispatch(this, NS_DISPATCH_NORMAL);
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

    
    if (mThread) {
      
      mThread->Shutdown(); 
      
      
      
      
      
    }
  }

  return NS_OK;
}

void
CryptoTask::virtualDestroyNSSReference()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "virtualDestroyNSSReference called off the main thread");
  if (!mReleasedNSSResources) {
    mReleasedNSSResources = true;
    ReleaseNSSResources();
  }
}

} 

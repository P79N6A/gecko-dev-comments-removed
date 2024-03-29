



#include "nsVolumeMountLock.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/Services.h"

#include "nsIObserverService.h"
#include "nsIPowerManagerService.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "nsString.h"
#include "nsXULAppAPI.h"

#undef VOLUME_MANAGER_LOG_TAG
#define VOLUME_MANAGER_LOG_TAG  "nsVolumeMountLock"
#include "VolumeManagerLog.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/dom/power/PowerManagerService.h"

using namespace mozilla::dom;
using namespace mozilla::services;

namespace mozilla {
namespace system {

NS_IMPL_ISUPPORTS(nsVolumeMountLock, nsIVolumeMountLock,
                  nsIObserver, nsISupportsWeakReference)


already_AddRefed<nsVolumeMountLock>
nsVolumeMountLock::Create(const nsAString& aVolumeName)
{
  DBG("nsVolumeMountLock::Create called");

  nsRefPtr<nsVolumeMountLock> mountLock = new nsVolumeMountLock(aVolumeName);
  nsresult rv = mountLock->Init();
  NS_ENSURE_SUCCESS(rv, nullptr);

  return mountLock.forget();
}

nsVolumeMountLock::nsVolumeMountLock(const nsAString& aVolumeName)
  : mVolumeName(aVolumeName),
    mVolumeGeneration(-1),
    mUnlocked(false)
{
}


nsVolumeMountLock::~nsVolumeMountLock()
{
  Unlock();
}

nsresult nsVolumeMountLock::Init()
{
  LOG("nsVolumeMountLock created for '%s'",
      NS_LossyConvertUTF16toASCII(mVolumeName).get());

  
  
  
  nsCOMPtr<nsIObserverService> obs = GetObserverService();
  obs->AddObserver(this, NS_VOLUME_STATE_CHANGED, true );

  
  nsCOMPtr<nsIVolumeService> vs = do_GetService(NS_VOLUMESERVICE_CONTRACTID);
  NS_ENSURE_TRUE(vs, NS_ERROR_FAILURE);

  nsCOMPtr<nsIVolume> vol;
  nsresult rv = vs->GetVolumeByName(mVolumeName, getter_AddRefs(vol));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  rv = vol->GetMountGeneration(&mVolumeGeneration);
  NS_ENSURE_SUCCESS(rv, rv);

  return Lock(vol);
}


NS_IMETHODIMP nsVolumeMountLock::Unlock()
{
  LOG("nsVolumeMountLock released for '%s'",
      NS_LossyConvertUTF16toASCII(mVolumeName).get());

  mUnlocked = true;
  mWakeLock = nullptr;

  
  
  nsCOMPtr<nsIObserverService> obs = GetObserverService();
  obs->RemoveObserver(this, NS_VOLUME_STATE_CHANGED);
  return NS_OK;
}

NS_IMETHODIMP nsVolumeMountLock::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* aData)
{
  if (strcmp(aTopic, NS_VOLUME_STATE_CHANGED) != 0) {
    return NS_OK;
  }
  if (mUnlocked) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIVolume> vol = do_QueryInterface(aSubject);
  if (!vol) {
    return NS_OK;
  }
  nsString volName;
  vol->GetName(volName);
  if (!volName.Equals(mVolumeName)) {
    return NS_OK;
  }
  int32_t state;
  nsresult rv = vol->GetState(&state);
  NS_ENSURE_SUCCESS(rv, rv);

  if (state != nsIVolume::STATE_MOUNTED) {
    mWakeLock = nullptr;
    mVolumeGeneration = -1;
    return NS_OK;
  }

  int32_t   mountGeneration;
  rv = vol->GetMountGeneration(&mountGeneration);
  NS_ENSURE_SUCCESS(rv, rv);

  DBG("nsVolumeMountLock::Observe mountGeneration = %d mVolumeGeneration = %d",
       mountGeneration, mVolumeGeneration);

  if (mVolumeGeneration == mountGeneration) {
    return NS_OK;
  }

  
  
  

  mWakeLock = nullptr;
  mVolumeGeneration = mountGeneration;

  return Lock(vol);
}

nsresult
nsVolumeMountLock::Lock(nsIVolume* aVolume)
{
  nsRefPtr<power::PowerManagerService> pmService =
    power::PowerManagerService::GetInstance();
  NS_ENSURE_TRUE(pmService, NS_ERROR_FAILURE);

  nsString mountLockName;
  aVolume->GetMountLockName(mountLockName);

  ErrorResult err;
  mWakeLock = pmService->NewWakeLock(mountLockName, nullptr, err);
  if (err.Failed()) {
    return err.StealNSResult();
  }

  LOG("nsVolumeMountLock acquired for '%s' gen %d",
      NS_LossyConvertUTF16toASCII(mVolumeName).get(), mVolumeGeneration);
  return NS_OK;
}

} 
} 





#include "nsVolumeService.h"

#include "Volume.h"
#include "VolumeManager.h"
#include "VolumeServiceIOThread.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDependentSubstring.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPowerManagerService.h"
#include "nsISupportsUtils.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "nsLocalFile.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsVolumeMountLock.h"
#include "nsXULAppAPI.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/Services.h"

#define VOLUME_MANAGER_LOG_TAG  "nsVolumeService"
#include "VolumeManagerLog.h"

#include <stdlib.h>

using namespace mozilla::dom;
using namespace mozilla::services;

namespace mozilla {
namespace system {

NS_IMPL_THREADSAFE_ISUPPORTS2(nsVolumeService,
                              nsIVolumeService,
                              nsIDOMMozWakeLockListener)

StaticRefPtr<nsVolumeService> nsVolumeService::sSingleton;


already_AddRefed<nsVolumeService>
nsVolumeService::GetSingleton()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!sSingleton) {
    sSingleton = new nsVolumeService();
  }
  nsRefPtr<nsVolumeService> volumeService = sSingleton.get();
  return volumeService.forget();
}


void
nsVolumeService::Shutdown()
{
  if (!sSingleton) {
    return;
  }
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    obs->RemoveObserver(sSingleton.get(), NS_VOLUME_STATE_CHANGED);
    sSingleton = nullptr;
    return;
  }

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (pmService) {
    pmService->RemoveWakeLockListener(sSingleton.get());
  }

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(ShutdownVolumeServiceIOThread));

  sSingleton = nullptr;
}

nsVolumeService::nsVolumeService()
  : mArrayMonitor("nsVolumeServiceArray")
{
  sSingleton = this;

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    obs->AddObserver(this, NS_VOLUME_STATE_CHANGED, false );

    
    ContentChild::GetSingleton()->SendBroadcastVolume(NS_LITERAL_STRING(""));
    return;
  }

  
  
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(InitVolumeServiceIOThread, this));

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  if (!pmService) {
    return;
  }
  pmService->AddWakeLockListener(this);
}

nsVolumeService::~nsVolumeService()
{
}


NS_IMETHODIMP
nsVolumeService::Callback(const nsAString& aTopic, const nsAString& aState)
{
  CheckMountLock(aTopic, aState);
  return NS_OK;
}

NS_IMETHODIMP
nsVolumeService::BroadcastVolume(const nsAString& aVolName)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  if (aVolName.EqualsLiteral("")) {
    nsVolume::Array volumeArray;
    {
      
      
      MonitorAutoLock autoLock(mArrayMonitor);
      volumeArray = mVolumeArray;
    }

    
    nsVolume::Array::size_type numVolumes = volumeArray.Length();
    nsVolume::Array::index_type volIndex;
    for (volIndex = 0; volIndex < numVolumes; volIndex++) {
      const nsString& volName(volumeArray[volIndex]->Name());
      if (!volName.EqualsLiteral("")) {
        
        
        
        
        
        nsresult rv = BroadcastVolume(volName);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    return NS_OK;
  }
  nsRefPtr<nsVolume> vol;
  {
    MonitorAutoLock autoLock(mArrayMonitor);
    vol = FindVolumeByName(aVolName);
  }
  if (!vol) {
    ERR("BroadcastVolume: Unable to locate volume '%s'",
        NS_LossyConvertUTF16toASCII(aVolName).get());
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIObserverService> obs = GetObserverService();
  NS_ENSURE_TRUE(obs, NS_NOINTERFACE);

  DBG("nsVolumeService::BroadcastVolume for '%s'", vol->NameStr().get());
  NS_ConvertUTF8toUTF16 stateStr(vol->StateStr());
  obs->NotifyObservers(vol, NS_VOLUME_STATE_CHANGED, stateStr.get());
  return NS_OK;
}

NS_IMETHODIMP nsVolumeService::GetVolumeByName(const nsAString& aVolName, nsIVolume **aResult)
{
  MonitorAutoLock autoLock(mArrayMonitor);

  nsRefPtr<nsVolume> vol = FindVolumeByName(aVolName);
  if (!vol) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  vol.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsVolumeService::GetVolumeByPath(const nsAString& aPath, nsIVolume **aResult)
{
  nsCString utf8Path = NS_ConvertUTF16toUTF8(aPath);
  char realPathBuf[PATH_MAX];

  if (!realpath(utf8Path.get(), realPathBuf)) {
    ERR("GetVolumeByPath: realpath on '%s' failed: %d", utf8Path.get(), errno);
    return NSRESULT_FOR_ERRNO();
  }

  
  
  
  
  
  
  

  strlcat(realPathBuf, "/", sizeof(realPathBuf));

  MonitorAutoLock autoLock(mArrayMonitor);

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    nsAutoCString volMountPointSlash = NS_ConvertUTF16toUTF8(vol->MountPoint());
    volMountPointSlash.Append(NS_LITERAL_CSTRING("/"));
    nsDependentCSubstring testStr(realPathBuf, volMountPointSlash.Length());
    if (volMountPointSlash.Equals(testStr)) {
      vol.forget(aResult);
      return NS_OK;
    }
  }
  return NS_ERROR_FILE_NOT_FOUND;
}

NS_IMETHODIMP
nsVolumeService::CreateOrGetVolumeByPath(const nsAString& aPath, nsIVolume** aResult)
{
  nsresult rv = GetVolumeByPath(aPath, aResult);
  if (rv == NS_OK) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIVolume> vol = new nsVolume(NS_LITERAL_STRING("fake"),
                                         aPath, nsIVolume::STATE_MOUNTED,
                                         -1 );
  vol.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsVolumeService::GetVolumeNames(nsTArray<nsString>& aVolNames)
{
  MonitorAutoLock autoLock(mArrayMonitor);

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    aVolNames.AppendElement(vol->Name());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsVolumeService::CreateMountLock(const nsAString& aVolumeName, nsIVolumeMountLock **aResult)
{
  nsCOMPtr<nsIVolumeMountLock> mountLock = nsVolumeMountLock::Create(aVolumeName);
  if (!mountLock) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  mountLock.forget(aResult);
  return NS_OK;
}

void
nsVolumeService::CheckMountLock(const nsAString& aMountLockName,
                                const nsAString& aMountLockState)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<nsVolume> vol = FindVolumeByMountLockName(aMountLockName);
  if (vol) {
    vol->UpdateMountLock(aMountLockState);
  }
}

already_AddRefed<nsVolume>
nsVolumeService::FindVolumeByMountLockName(const nsAString& aMountLockName)
{
  MonitorAutoLock autoLock(mArrayMonitor);

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    nsString mountLockName;
    vol->GetMountLockName(mountLockName);
    if (mountLockName.Equals(aMountLockName)) {
      return vol.forget();
    }
  }
  return nullptr;
}

already_AddRefed<nsVolume>
nsVolumeService::FindVolumeByName(const nsAString& aName)
{
  mArrayMonitor.AssertCurrentThreadOwns();

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    if (vol->Name().Equals(aName)) {
      return vol.forget();
    }
  }
  return nullptr;
}


already_AddRefed<nsVolume>
nsVolumeService::CreateOrFindVolumeByName(const nsAString& aName)
{
  MonitorAutoLock autoLock(mArrayMonitor);

  nsRefPtr<nsVolume> vol;
  vol = FindVolumeByName(aName);
  if (vol) {
    return vol.forget();
  }
  
  vol = new nsVolume(aName);
  mVolumeArray.AppendElement(vol);
  return vol.forget();
}

NS_IMETHODIMP
nsVolumeService::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  if (strcmp(aTopic, NS_VOLUME_STATE_CHANGED) != 0) {
    return NS_OK;
  }
  MOZ_ASSERT(XRE_GetProcessType() != GeckoProcessType_Default);
  nsCOMPtr<nsIVolume> vol = do_QueryInterface(aSubject);
  if (!vol) {
    return NS_OK;
  }
  UpdateVolume(vol);
  return NS_OK;
}

void
nsVolumeService::UpdateVolume(nsIVolume* aVolume)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsString volName;
  aVolume->GetName(volName);
  nsRefPtr<nsVolume> vol = CreateOrFindVolumeByName(volName);
  if (vol->Equals(aVolume)) {
    
    return;
  }
  vol->Set(aVolume);
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    
    return;
  }
  nsCOMPtr<nsIObserverService> obs = GetObserverService();
  if (!obs) {
    return;
  }
  NS_ConvertUTF8toUTF16 stateStr(vol->StateStr());
  obs->NotifyObservers(vol, NS_VOLUME_STATE_CHANGED, stateStr.get());
}





class UpdateVolumeRunnable : public nsRunnable
{
public:
  UpdateVolumeRunnable(nsVolumeService* aVolumeService, const Volume* aVolume)
    : mVolumeService(aVolumeService),
      mVolume(new nsVolume(aVolume))
  {
    MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    DBG("UpdateVolumeRunnable::Run '%s' state %s gen %d locked %d",
        mVolume->NameStr().get(), mVolume->StateStr(),
        mVolume->MountGeneration(), (int)mVolume->IsMountLocked());

    mVolumeService->UpdateVolume(mVolume);
    mVolumeService = nullptr;
    mVolume = nullptr;
    return NS_OK;
  }

private:
  nsRefPtr<nsVolumeService> mVolumeService;
  nsRefPtr<nsVolume>        mVolume;
};

void
nsVolumeService::UpdateVolumeIOThread(const Volume* aVolume)
{
  DBG("UpdateVolumeIOThread: Volume '%s' state %s mount '%s' gen %d locked %d",
      aVolume->NameStr(), aVolume->StateStr(), aVolume->MountPoint().get(),
      aVolume->MountGeneration(), (int)aVolume->IsMountLocked());
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  NS_DispatchToMainThread(new UpdateVolumeRunnable(this, aVolume));
}

} 
} 

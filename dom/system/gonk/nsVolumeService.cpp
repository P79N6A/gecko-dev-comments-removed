



#include "nsVolumeService.h"

#include "Volume.h"
#include "VolumeManager.h"
#include "VolumeServiceIOThread.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDependentSubstring.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIMutableArray.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPowerManagerService.h"
#include "nsISupportsPrimitives.h"
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

#undef VOLUME_MANAGER_LOG_TAG
#define VOLUME_MANAGER_LOG_TAG  "nsVolumeService"
#include "VolumeManagerLog.h"

#include <stdlib.h>

using namespace mozilla::dom;
using namespace mozilla::services;

namespace mozilla {
namespace system {

NS_IMPL_ISUPPORTS(nsVolumeService,
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
  : mArrayMonitor("nsVolumeServiceArray"),
    mGotVolumesFromParent(false)
{
  sSingleton = this;

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    
    
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

void nsVolumeService::DumpNoLock(const char* aLabel)
{
  mArrayMonitor.AssertCurrentThreadOwns();

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();

  if (numVolumes == 0) {
    LOG("%s: No Volumes!", aLabel);
    return;
  }
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    vol->Dump(aLabel);
  }
}

NS_IMETHODIMP
nsVolumeService::Dump(const nsAString& aLabel)
{
  MonitorAutoLock autoLock(mArrayMonitor);
  DumpNoLock(NS_LossyConvertUTF16toASCII(aLabel).get());
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
  NS_ConvertUTF16toUTF8 utf8Path(aPath);
  char realPathBuf[PATH_MAX];

  while (realpath(utf8Path.get(), realPathBuf) < 0) {
    if (errno != ENOENT) {
      ERR("GetVolumeByPath: realpath on '%s' failed: %d", utf8Path.get(), errno);
      return NSRESULT_FOR_ERRNO();
    }
    
    
    
    int32_t slashIndex = utf8Path.RFindChar('/');
    if ((slashIndex == kNotFound) || (slashIndex == 0)) {
      errno = ENOENT;
      ERR("GetVolumeByPath: realpath on '%s' failed.", utf8Path.get());
      return NSRESULT_FOR_ERRNO();
    }
    utf8Path.Assign(Substring(utf8Path, 0, slashIndex));
  }

  
  
  
  
  
  
  

  strlcat(realPathBuf, "/", sizeof(realPathBuf));

  MonitorAutoLock autoLock(mArrayMonitor);

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    NS_ConvertUTF16toUTF8 volMountPointSlash(vol->MountPoint());
    volMountPointSlash.Append('/');
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
                                         -1    ,
                                         true  ,
                                         false ,
                                         false ,
                                         true  ,
                                         false ,
                                         false ,
                                         false );
  vol.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsVolumeService::GetVolumeNames(nsIArray** aVolNames)
{
  NS_ENSURE_ARG_POINTER(aVolNames);
  MonitorAutoLock autoLock(mArrayMonitor);

  *aVolNames = nullptr;

  nsresult rv;
  nsCOMPtr<nsIMutableArray> volNames =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    nsCOMPtr<nsISupportsString> isupportsString =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = isupportsString->SetData(vol->Name());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = volNames->AppendElement(isupportsString, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  volNames.forget(aVolNames);
  return NS_OK;
}

void
nsVolumeService::GetVolumesForIPC(nsTArray<VolumeInfo>* aResult)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(NS_IsMainThread());

  MonitorAutoLock autoLock(mArrayMonitor);

  nsVolume::Array::size_type numVolumes = mVolumeArray.Length();
  nsVolume::Array::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    nsRefPtr<nsVolume> vol = mVolumeArray[volIndex];
    VolumeInfo* volInfo = aResult->AppendElement();

    volInfo->name()             = vol->mName;
    volInfo->mountPoint()       = vol->mMountPoint;
    volInfo->volState()         = vol->mState;
    volInfo->mountGeneration()  = vol->mMountGeneration;
    volInfo->isMediaPresent()   = vol->mIsMediaPresent;
    volInfo->isSharing()        = vol->mIsSharing;
    volInfo->isFormatting()     = vol->mIsFormatting;
    volInfo->isFake()           = vol->mIsFake;
    volInfo->isUnmounting()     = vol->mIsUnmounting;
    volInfo->isRemovable()      = vol->mIsRemovable;
    volInfo->isHotSwappable()   = vol->mIsHotSwappable;
  }
}

void
nsVolumeService::RecvVolumesFromParent(const nsTArray<VolumeInfo>& aVolumes)
{
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    return;
  }
  if (mGotVolumesFromParent) {
    
    return;
  }

  for (uint32_t i = 0; i < aVolumes.Length(); i++) {
    const VolumeInfo& volInfo(aVolumes[i]);
    nsRefPtr<nsVolume> vol = new nsVolume(volInfo.name(),
                                          volInfo.mountPoint(),
                                          volInfo.volState(),
                                          volInfo.mountGeneration(),
                                          volInfo.isMediaPresent(),
                                          volInfo.isSharing(),
                                          volInfo.isFormatting(),
                                          volInfo.isFake(),
                                          volInfo.isUnmounting(),
                                          volInfo.isRemovable(),
                                          volInfo.isHotSwappable());
    UpdateVolume(vol, false);
  }
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
nsVolumeService::CreateOrFindVolumeByName(const nsAString& aName, bool aIsFake )
{
  MonitorAutoLock autoLock(mArrayMonitor);

  nsRefPtr<nsVolume> vol;
  vol = FindVolumeByName(aName);
  if (vol) {
    return vol.forget();
  }
  
  vol = new nsVolume(aName);
  vol->SetIsFake(aIsFake);
  mVolumeArray.AppendElement(vol);
  return vol.forget();
}

void
nsVolumeService::UpdateVolume(nsIVolume* aVolume, bool aNotifyObservers)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsString volName;
  aVolume->GetName(volName);
  bool aIsFake;
  aVolume->GetIsFake(&aIsFake);
  nsRefPtr<nsVolume> vol = CreateOrFindVolumeByName(volName, aIsFake);
  if (vol->Equals(aVolume)) {
    
    return;
  }

  if (!vol->IsFake() && aIsFake) {
    
    return;
  }

  vol->Set(aVolume);

  if (!aNotifyObservers) {
    return;
  }

  nsCOMPtr<nsIObserverService> obs = GetObserverService();
  if (!obs) {
    return;
  }
  NS_ConvertUTF8toUTF16 stateStr(vol->StateStr());
  obs->NotifyObservers(vol, NS_VOLUME_STATE_CHANGED, stateStr.get());
}

NS_IMETHODIMP
nsVolumeService::CreateFakeVolume(const nsAString& name, const nsAString& path)
{
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsRefPtr<nsVolume> vol = new nsVolume(name, path, nsIVolume::STATE_INIT,
                                          -1    ,
                                          true  ,
                                          false ,
                                          false ,
                                          true  ,
                                          false ,
                                          false ,
                                          false );
    vol->LogState();
    UpdateVolume(vol.get());
    return NS_OK;
  }

  ContentChild::GetSingleton()->SendCreateFakeVolume(nsString(name), nsString(path));
  return NS_OK;
}

NS_IMETHODIMP
nsVolumeService::SetFakeVolumeState(const nsAString& name, int32_t state)
{
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsRefPtr<nsVolume> vol;
    {
      MonitorAutoLock autoLock(mArrayMonitor);
      vol = FindVolumeByName(name);
    }
    if (!vol || !vol->IsFake()) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    vol->SetState(state);
    vol->LogState();
    UpdateVolume(vol.get());
    return NS_OK;
  }

  ContentChild::GetSingleton()->SendSetFakeVolumeState(nsString(name), state);
  return NS_OK;
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
    DBG("UpdateVolumeRunnable::Run '%s' state %s gen %d locked %d "
        "media %d sharing %d formatting %d unmounting %d removable %d hotswappable %d",
        mVolume->NameStr().get(), mVolume->StateStr(),
        mVolume->MountGeneration(), (int)mVolume->IsMountLocked(),
        (int)mVolume->IsMediaPresent(), mVolume->IsSharing(),
        mVolume->IsFormatting(), mVolume->IsUnmounting(),
        (int)mVolume->IsRemovable(), (int)mVolume->IsHotSwappable());

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
  DBG("UpdateVolumeIOThread: Volume '%s' state %s mount '%s' gen %d locked %d "
      "media %d sharing %d formatting %d unmounting %d removable %d hotswappable %d",
      aVolume->NameStr(), aVolume->StateStr(), aVolume->MountPoint().get(),
      aVolume->MountGeneration(), (int)aVolume->IsMountLocked(),
      (int)aVolume->MediaPresent(), (int)aVolume->IsSharing(),
      (int)aVolume->IsFormatting(), (int)aVolume->IsUnmounting(),
      (int)aVolume->IsRemovable(), (int)aVolume->IsHotSwappable());
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  NS_DispatchToMainThread(new UpdateVolumeRunnable(this, aVolume));
}

} 
} 

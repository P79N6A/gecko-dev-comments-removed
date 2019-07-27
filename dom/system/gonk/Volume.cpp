



#include "Volume.h"
#include "VolumeCommand.h"
#include "VolumeManager.h"
#include "VolumeManagerLog.h"
#include "nsIVolume.h"
#include "nsXULAppAPI.h"

#include <vold/ResponseCode.h>

namespace mozilla {
namespace system {

#if DEBUG_VOLUME_OBSERVER
void
VolumeObserverList::Broadcast(Volume* const& aVolume)
{
  uint32_t size = mObservers.Length();
  for (uint32_t i = 0; i < size; ++i) {
    LOG("VolumeObserverList::Broadcast to [%u] %p volume '%s'",
        i, mObservers[i], aVolume->NameStr());
    mObservers[i]->Notify(aVolume);
  }
}
#endif

VolumeObserverList Volume::sEventObserverList;

































static int32_t sMountGeneration = 0;

static uint32_t sNextId = 1;



Volume::Volume(const nsCSubstring& aName)
  : mMediaPresent(true),
    mState(nsIVolume::STATE_INIT),
    mName(aName),
    mMountGeneration(-1),
    mMountLocked(true),  
    mSharingEnabled(false),
    mFormatRequested(false),
    mMountRequested(false),
    mUnmountRequested(false),
    mCanBeShared(true),
    mIsSharing(false),
    mIsFormatting(false),
    mIsUnmounting(false),
    mIsRemovable(false),
    mIsHotSwappable(false),
    mId(sNextId++)
{
  DBG("Volume %s: created", NameStr());
}

void
Volume::Dump(const char* aLabel) const
{
  LOG("%s: Volume: %s (%d) is %s and %s @ %s gen %d locked %d",
      aLabel,
      NameStr(),
      Id(),
      StateStr(),
      MediaPresent() ? "inserted" : "missing",
      MountPoint().get(),
      MountGeneration(),
      (int)IsMountLocked());
  LOG("%s:   Sharing %s Mounting %s Formating %s Unmounting %s",
      aLabel,
      CanBeShared() ? (IsSharingEnabled() ? (IsSharing() ? "en-y" : "en-n")
                                          : "dis")
                    : "x",
      IsMountRequested() ? "req" : "n",
      IsFormatRequested() ? (IsFormatting() ? "req-y" : "req-n")
                          : (IsFormatting() ? "y" : "n"),
      IsUnmountRequested() ? (IsUnmounting() ? "req-y" : "req-n")
                           : (IsUnmounting() ? "y" : "n"));
}

void Volume::SetFakeVolume(const nsACString& aMountPoint)
{
  this->mMountLocked = false;
  this->mCanBeShared = false;
  this->mMountPoint = aMountPoint;
  SetState(nsIVolume::STATE_MOUNTED);
}

void
Volume::SetIsSharing(bool aIsSharing)
{
  if (aIsSharing == mIsSharing) {
    return;
  }
  mIsSharing = aIsSharing;
  LOG("Volume %s: IsSharing set to %d state %s",
      NameStr(), (int)mIsSharing, StateStr(mState));
  sEventObserverList.Broadcast(this);
}

void
Volume::SetIsFormatting(bool aIsFormatting)
{
  if (aIsFormatting == mIsFormatting) {
    return;
  }
  mIsFormatting = aIsFormatting;
  LOG("Volume %s: IsFormatting set to %d state %s",
      NameStr(), (int)mIsFormatting, StateStr(mState));
  if (mIsFormatting) {
    sEventObserverList.Broadcast(this);
  }
}

void
Volume::SetIsUnmounting(bool aIsUnmounting)
{
  if (aIsUnmounting == mIsUnmounting) {
    return;
  }
  mIsUnmounting = aIsUnmounting;
  LOG("Volume %s: IsUnmounting set to %d state %s",
      NameStr(), (int)mIsUnmounting, StateStr(mState));
  sEventObserverList.Broadcast(this);
}

void
Volume::SetIsRemovable(bool aIsRemovable)
{
  if (aIsRemovable == mIsRemovable) {
    return;
  }
  mIsRemovable = aIsRemovable;
  if (!mIsRemovable) {
    mIsHotSwappable = false;
  }
  LOG("Volume %s: IsRemovable set to %d state %s",
      NameStr(), (int)mIsRemovable, StateStr(mState));
  sEventObserverList.Broadcast(this);
}

void
Volume::SetIsHotSwappable(bool aIsHotSwappable)
{
  if (aIsHotSwappable == mIsHotSwappable) {
    return;
  }
  mIsHotSwappable = aIsHotSwappable;
  if (mIsHotSwappable) {
    mIsRemovable = true;
  }
  LOG("Volume %s: IsHotSwappable set to %d state %s",
      NameStr(), (int)mIsHotSwappable, StateStr(mState));
  sEventObserverList.Broadcast(this);
}

bool
Volume::BoolConfigValue(const nsCString& aConfigValue, bool& aBoolValue)
{
  if (aConfigValue.EqualsLiteral("1") ||
      aConfigValue.LowerCaseEqualsLiteral("true")) {
    aBoolValue = true;
    return true;
  }
  if (aConfigValue.EqualsLiteral("0") ||
      aConfigValue.LowerCaseEqualsLiteral("false")) {
    aBoolValue = false;
    return true;
  }
  return false;
}

void
Volume::SetConfig(const nsCString& aConfigName, const nsCString& aConfigValue)
{
  if (aConfigName.LowerCaseEqualsLiteral("removable")) {
    bool value = false;
    if (BoolConfigValue(aConfigValue, value)) {
      SetIsRemovable(value);
    } else {
      ERR("Volume %s: invalid value '%s' for configuration '%s'",
          NameStr(), aConfigValue.get(), aConfigName.get());
    }
    return;
  }
  if (aConfigName.LowerCaseEqualsLiteral("hotswappable")) {
    bool value = false;
    if (BoolConfigValue(aConfigValue, value)) {
      SetIsHotSwappable(value);
    } else {
      ERR("Volume %s: invalid value '%s' for configuration '%s'",
          NameStr(), aConfigValue.get(), aConfigName.get());
    }
    return;
  }
  ERR("Volume %s: invalid config '%s'", NameStr(), aConfigName.get());
}

void
Volume::SetMediaPresent(bool aMediaPresent)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (mMediaPresent == aMediaPresent) {
    return;
  }

  LOG("Volume: %s media %s", NameStr(), aMediaPresent ? "inserted" : "removed");
  mMediaPresent = aMediaPresent;
  sEventObserverList.Broadcast(this);
}

void
Volume::SetSharingEnabled(bool aSharingEnabled)
{
  mSharingEnabled = aSharingEnabled;

  LOG("SetSharingMode for volume %s to %d canBeShared = %d",
      NameStr(), (int)mSharingEnabled, (int)mCanBeShared);
  sEventObserverList.Broadcast(this);
}

void
Volume::SetFormatRequested(bool aFormatRequested)
{
  mFormatRequested = aFormatRequested;

  LOG("SetFormatRequested for volume %s to %d CanBeFormatted = %d",
      NameStr(), (int)mFormatRequested, (int)CanBeFormatted());
}

void
Volume::SetMountRequested(bool aMountRequested)
{
  mMountRequested = aMountRequested;

  LOG("SetMountRequested for volume %s to %d CanBeMounted = %d",
      NameStr(), (int)mMountRequested, (int)CanBeMounted());
}

void
Volume::SetUnmountRequested(bool aUnmountRequested)
{
  mUnmountRequested = aUnmountRequested;

  LOG("SetUnmountRequested for volume %s to %d CanBeMounted = %d",
      NameStr(), (int)mUnmountRequested, (int)CanBeMounted());
}

void
Volume::SetState(Volume::STATE aNewState)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  if (aNewState == mState) {
    return;
  }
  if (aNewState == nsIVolume::STATE_MOUNTED) {
    mMountGeneration = ++sMountGeneration;
    LOG("Volume %s (%u): changing state from %s to %s @ '%s' (%d observers) "
        "mountGeneration = %d, locked = %d",
        NameStr(), mId, StateStr(mState),
        StateStr(aNewState), mMountPoint.get(), sEventObserverList.Length(),
        mMountGeneration, (int)mMountLocked);
  } else {
    LOG("Volume %s (%u): changing state from %s to %s (%d observers)",
        NameStr(), mId, StateStr(mState),
        StateStr(aNewState), sEventObserverList.Length());
  }

  switch (aNewState) {
     case nsIVolume::STATE_NOMEDIA:
       
       mMediaPresent = false;
       mIsSharing = false;
       mUnmountRequested = false;
       mMountRequested = false;
       mIsUnmounting = false;
       break;

     case nsIVolume::STATE_MOUNTED:
     case nsIVolume::STATE_MOUNT_FAIL:
       mMountRequested = false;
       mIsFormatting = false;
       mIsSharing = false;
       mIsUnmounting = false;
       break;

     case nsIVolume::STATE_FORMATTING:
       mFormatRequested = false;
       mIsFormatting = true;
       mIsSharing = false;
       mIsUnmounting = false;
       break;

     case nsIVolume::STATE_SHARED:
     case nsIVolume::STATE_SHAREDMNT:
       
       
       
       
       mIsSharing = true;
       mIsUnmounting = false;
       mIsFormatting = false;
       break;

     case nsIVolume::STATE_UNMOUNTING:
       mIsUnmounting = true;
       mIsFormatting = false;
       mIsSharing = false;
       break;

     case nsIVolume::STATE_IDLE: 
     case nsIVolume::STATE_CHECKMNT: 
     default:
       break;
  }
  mState = aNewState;
  sEventObserverList.Broadcast(this);
}

void
Volume::SetMountPoint(const nsCSubstring& aMountPoint)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  if (mMountPoint.Equals(aMountPoint)) {
    return;
  }
  mMountPoint = aMountPoint;
  DBG("Volume %s: Setting mountpoint to '%s'", NameStr(), mMountPoint.get());
}

void
Volume::StartMount(VolumeResponseCallback* aCallback)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  StartCommand(new VolumeActionCommand(this, "mount", "", aCallback));
}

void
Volume::StartUnmount(VolumeResponseCallback* aCallback)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  StartCommand(new VolumeActionCommand(this, "unmount", "force", aCallback));
}

void
Volume::StartFormat(VolumeResponseCallback* aCallback)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  StartCommand(new VolumeActionCommand(this, "format", "", aCallback));
}

void
Volume::StartShare(VolumeResponseCallback* aCallback)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  StartCommand(new VolumeActionCommand(this, "share", "ums", aCallback));
}

void
Volume::StartUnshare(VolumeResponseCallback* aCallback)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  StartCommand(new VolumeActionCommand(this, "unshare", "ums", aCallback));
}

void
Volume::StartCommand(VolumeCommand* aCommand)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  VolumeManager::PostCommand(aCommand);
}


void
Volume::RegisterVolumeObserver(Volume::EventObserver* aObserver, const char* aName)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  sEventObserverList.AddObserver(aObserver);

  DBG("Added Volume Observer '%s' @%p, length = %u",
      aName, aObserver, sEventObserverList.Length());

  
  size_t numVolumes = VolumeManager::NumVolumes();
  for (size_t volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = VolumeManager::GetVolume(volIndex);
    aObserver->Notify(vol);
  }
}


void
Volume::UnregisterVolumeObserver(Volume::EventObserver* aObserver, const char* aName)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  sEventObserverList.RemoveObserver(aObserver);

  DBG("Removed Volume Observer '%s' @%p, length = %u",
      aName, aObserver, sEventObserverList.Length());
}


void
Volume::UpdateMountLock(const nsACString& aVolumeName,
                        const int32_t& aMountGeneration,
                        const bool& aMountLocked)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  RefPtr<Volume> vol = VolumeManager::FindVolumeByName(aVolumeName);
  if (!vol || (vol->mMountGeneration != aMountGeneration)) {
    return;
  }
  if (vol->mMountLocked != aMountLocked) {
    vol->mMountLocked = aMountLocked;
    DBG("Volume::UpdateMountLock for '%s' to %d\n", vol->NameStr(), (int)aMountLocked);
    sEventObserverList.Broadcast(vol);
  }
}

void
Volume::HandleVoldResponse(int aResponseCode, nsCWhitespaceTokenizer& aTokenizer)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  switch (aResponseCode) {
    case ::ResponseCode::VolumeListResult: {
      
      
      
      
      nsDependentCSubstring mntPoint(aTokenizer.nextToken());
      SetMountPoint(mntPoint);
      nsresult errCode;
      nsCString state(aTokenizer.nextToken());
      if (state.EqualsLiteral("X")) {
        
        mCanBeShared = false;
        SetState(nsIVolume::STATE_MOUNTED);
      } else {
        SetState((STATE)state.ToInteger(&errCode));
      }
      break;
    }

    case ::ResponseCode::VolumeStateChange: {
      
      
      
      
      
      while (aTokenizer.hasMoreTokens()) {
        nsAutoCString token(aTokenizer.nextToken());
        if (token.EqualsLiteral("to")) {
          nsresult errCode;
          token = aTokenizer.nextToken();
          STATE newState = (STATE)(token.ToInteger(&errCode));
          if (newState == nsIVolume::STATE_MOUNTED) {
            
            
            
            SetState(nsIVolume::STATE_CHECKMNT);
          } else {
            if (State() == nsIVolume::STATE_CHECKING && newState == nsIVolume::STATE_IDLE) {
              LOG("Mount of volume '%s' failed", NameStr());
              SetState(nsIVolume::STATE_MOUNT_FAIL);
            } else {
              SetState(newState);
            }
          }
          break;
        }
      }
      break;
    }

    case ::ResponseCode::VolumeDiskInserted:
      SetMediaPresent(true);
      break;

    case ::ResponseCode::VolumeDiskRemoved: 
    case ::ResponseCode::VolumeBadRemoval:
      SetMediaPresent(false);
      break;

    default:
      LOG("Volume: %s unrecognized reponse code (ignored)", NameStr());
      break;
  }
}

} 
} 

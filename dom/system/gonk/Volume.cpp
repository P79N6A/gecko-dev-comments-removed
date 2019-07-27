



#include "Volume.h"
#include "VolumeCommand.h"
#include "VolumeManager.h"
#include "VolumeManagerLog.h"
#include "nsIVolume.h"
#include "nsXULAppAPI.h"

#include <vold/ResponseCode.h>

namespace mozilla {
namespace system {

Volume::EventObserverList Volume::mEventObserverList;

































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
    mId(sNextId++)
{
  DBG("Volume %s: created", NameStr());
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
  mEventObserverList.Broadcast(this);
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
    mEventObserverList.Broadcast(this);
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
  mEventObserverList.Broadcast(this);
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
  mEventObserverList.Broadcast(this);
}

void
Volume::SetSharingEnabled(bool aSharingEnabled)
{
  mSharingEnabled = aSharingEnabled;

  LOG("SetSharingMode for volume %s to %d canBeShared = %d",
      NameStr(), (int)mSharingEnabled, (int)mCanBeShared);
  mEventObserverList.Broadcast(this);
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
        StateStr(aNewState), mMountPoint.get(), mEventObserverList.Length(),
        mMountGeneration, (int)mMountLocked);
  } else {
    LOG("Volume %s (%u): changing state from %s to %s (%d observers)",
        NameStr(), mId, StateStr(mState),
        StateStr(aNewState), mEventObserverList.Length());
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
       break;
     default:
       break;
  }
  mState = aNewState;
  mEventObserverList.Broadcast(this);
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
Volume::RegisterObserver(Volume::EventObserver* aObserver)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  mEventObserverList.AddObserver(aObserver);
  
  size_t numVolumes = VolumeManager::NumVolumes();
  for (size_t volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = VolumeManager::GetVolume(volIndex);
    aObserver->Notify(vol);
  }
}


void
Volume::UnregisterObserver(Volume::EventObserver* aObserver)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  mEventObserverList.RemoveObserver(aObserver);
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
    mEventObserverList.Broadcast(vol);
  }
}

void
Volume::HandleVoldResponse(int aResponseCode, nsCWhitespaceTokenizer& aTokenizer)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  switch (aResponseCode) {
    case ResponseCode::VolumeListResult: {
      
      
      
      
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

    case ResponseCode::VolumeStateChange: {
      
      
      
      
      
      while (aTokenizer.hasMoreTokens()) {
        nsAutoCString token(aTokenizer.nextToken());
        if (token.EqualsLiteral("to")) {
          nsresult errCode;
          token = aTokenizer.nextToken();
          SetState((STATE)token.ToInteger(&errCode));
          break;
        }
      }
      break;
    }

    case ResponseCode::VolumeDiskInserted:
      SetMediaPresent(true);
      break;

    case ResponseCode::VolumeDiskRemoved: 
    case ResponseCode::VolumeBadRemoval:
      SetMediaPresent(false);
      break;

    default:
      LOG("Volume: %s unrecognized reponse code (ignored)", NameStr());
      break;
  }
}

} 
} 

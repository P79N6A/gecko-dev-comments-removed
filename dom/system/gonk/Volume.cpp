



#include "Volume.h"
#include "VolumeCommand.h"
#include "VolumeManager.h"
#include "VolumeManagerLog.h"
#include "nsXULAppAPI.h"

#include <vold/ResponseCode.h>

namespace mozilla {
namespace system {



Volume::Volume(const nsCSubstring &aName)
  : mMediaPresent(true),
    mState(STATE_INIT),
    mName(aName)
{
  DBG("Volume %s: created", NameStr());
}

void
Volume::SetMediaPresent(bool aMediaPresent)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (mMediaPresent != aMediaPresent) {
    LOG("Volume: %s media %s", NameStr(), aMediaPresent ? "inserted" : "removed");
    mMediaPresent = aMediaPresent;
    
    
  }
}

void
Volume::SetState(Volume::STATE aNewState)
{
  if (aNewState == mState) {
    return;
  }
  LOG("Volume %s: changing state from %s to %s (%d observers)",
      NameStr(), StateStr(mState),
      StateStr(aNewState), mEventObserverList.Length());

  if (aNewState == STATE_NOMEDIA) {
    
    mMediaPresent = false;
  }
  mState = aNewState;
  mEventObserverList.Broadcast(this);
}

void
Volume::SetMountPoint(const nsCSubstring &aMountPoint)
{
  if (mMountPoint.Equals(aMountPoint)) {
    return;
  }
  mMountPoint = aMountPoint;
  DBG("Volume %s: Setting mountpoint to '%s'", NameStr(), mMountPoint.get());
}

void
Volume::StartMount(VolumeResponseCallback *aCallback)
{
  StartCommand(new VolumeActionCommand(this, "mount", "", aCallback));
}

void
Volume::StartUnmount(VolumeResponseCallback *aCallback)
{
  StartCommand(new VolumeActionCommand(this, "unmount", "force", aCallback));
}

void
Volume::StartShare(VolumeResponseCallback *aCallback)
{
  StartCommand(new VolumeActionCommand(this, "share", "ums", aCallback));
}

void
Volume::StartUnshare(VolumeResponseCallback *aCallback)
{
  StartCommand(new VolumeActionCommand(this, "unshare", "ums", aCallback));
}

void
Volume::StartCommand(VolumeCommand *aCommand)
{
  VolumeManager::PostCommand(aCommand);
}

void
Volume::RegisterObserver(Volume::EventObserver *aObserver)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  mEventObserverList.AddObserver(aObserver);
  
  aObserver->Notify(this);
}

void
Volume::UnregisterObserver(Volume::EventObserver *aObserver)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  mEventObserverList.RemoveObserver(aObserver);
}

void
Volume::HandleVoldResponse(int aResponseCode, nsCWhitespaceTokenizer &aTokenizer)
{
  
  
  switch (aResponseCode) {
    case ResponseCode::VolumeListResult: {
      
      
      
      
      nsDependentCSubstring mntPoint(aTokenizer.nextToken());
      SetMountPoint(mntPoint);
      PRInt32 errCode;
      nsCString state(aTokenizer.nextToken());
      SetState((STATE)state.ToInteger(&errCode));
      break;
    }

    case ResponseCode::VolumeStateChange: {
      
      
      
      
      
      while (aTokenizer.hasMoreTokens()) {
        nsCAutoString token(aTokenizer.nextToken());
        if (token.Equals("to")) {
          PRInt32 errCode;
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

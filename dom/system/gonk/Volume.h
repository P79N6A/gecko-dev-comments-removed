



#ifndef mozilla_system_volume_h__
#define mozilla_system_volume_h__

#include "mozilla/RefPtr.h"
#include "nsString.h"
#include "VolumeCommand.h"

namespace mozilla {
namespace system {










class Volume : public RefCounted<Volume>
{
public:
  
  enum STATE
  {
    STATE_INIT        = -1,
    STATE_NOMEDIA     = 0,
    STATE_IDLE        = 1,
    STATE_PENDING     = 2,
    STATE_CHECKING    = 3,
    STATE_MOUNTED     = 4,
    STATE_UNMOUNTING  = 5,
    STATE_FORMATTING  = 6,
    STATE_SHARED      = 7,
    STATE_SHAREDMNT   = 8
  };

  Volume(const nsCSubstring &aVolumeName);

  const char *StateStr(STATE aState) const
  {
    switch (aState) {
      case STATE_INIT:        return "Init";
      case STATE_NOMEDIA:     return "NoMedia";
      case STATE_IDLE:        return "Idle";
      case STATE_PENDING:     return "Pending";
      case STATE_CHECKING:    return "Checking";
      case STATE_MOUNTED:     return "Mounted";
      case STATE_UNMOUNTING:  return "Unmounting";
      case STATE_FORMATTING:  return "Formatting";
      case STATE_SHARED:      return "Shared";
      case STATE_SHAREDMNT:   return "Shared-Mounted";
    }
    return "???";
  }
  const char *StateStr() const  { return StateStr(mState); }
  STATE State() const           { return mState; }

  const nsCString &Name() const { return mName; }
  const char *NameStr() const   { return mName.get(); }

  
  
  const nsCString &MountPoint() const { return mMountPoint; }

  
  
  
  void StartMount(VolumeResponseCallback *aCallback);
  void StartUnmount(VolumeResponseCallback *aCallback);
  void StartShare(VolumeResponseCallback *aCallback);
  void StartUnshare(VolumeResponseCallback *aCallback);

private:
  friend class VolumeManager;       
  friend class VolumeListCallback;  

  void SetState(STATE aNewState);
  void SetMountPoint(const nsCSubstring &aMountPoint);
  void StartCommand(VolumeCommand *aCommand);

  STATE             mState;
  const nsCString   mName;

  nsCString         mMountPoint;
};

} 
} 

#endif

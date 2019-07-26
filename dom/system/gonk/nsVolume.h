



#ifndef mozilla_system_nsvolume_h__
#define mozilla_system_nsvolume_h__

#include "nsCOMPtr.h"
#include "nsIVolume.h"
#include "nsString.h"
#include "nsTArray.h"

namespace mozilla {
namespace system {

class Volume;
class VolumeMountLock;

class nsVolume : public nsIVolume
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOLUME

  
  nsVolume(const Volume *aVolume);

  
  nsVolume(const nsAString &aName, const nsAString &aMountPoint,
           const int32_t &aState, const int32_t &aMountGeneration)
    : mName(aName),
      mMountPoint(aMountPoint),
      mState(aState),
      mMountGeneration(aMountGeneration),
      mMountLocked(false)
  {
  }

  
  
  nsVolume(const nsAString &aName)
    : mName(aName),
      mState(STATE_INIT),
      mMountGeneration(-1),
      mMountLocked(true)  
  {
  }

  bool Equals(const nsVolume *aVolume)
  {
    return mName.Equals(aVolume->mName)
        && mMountPoint.Equals(aVolume->mMountPoint)
        && (mState == aVolume->mState)
        && (mMountGeneration == aVolume->mMountGeneration)
        && (mMountLocked == aVolume->mMountLocked);
  }

  void Set(const nsVolume *aVolume);

  void LogState() const;

  const nsString &Name() const        { return mName; }
  const char *NameStr() const         { return NS_LossyConvertUTF16toASCII(mName).get(); }

  int32_t MountGeneration() const     { return mMountGeneration; }
  bool IsMountLocked() const          { return mMountLocked; }

  const nsString &MountPoint() const  { return mMountPoint; }
  const char *MountPointStr() const   { return NS_LossyConvertUTF16toASCII(mMountPoint).get(); }

  int32_t State() const               { return mState; }
  const char *StateStr() const        { return NS_VolumeStateStr(mState); }

  typedef nsTArray<nsRefPtr<nsVolume> > Array;

private:
  ~nsVolume() {}

  friend class nsVolumeService; 
  void UpdateMountLock(const nsAString &aMountLockState);
  void UpdateMountLock(bool aMountLocked);

  nsString mName;
  nsString mMountPoint;
  int32_t  mState;
  int32_t  mMountGeneration;
  bool     mMountLocked;
};

} 
} 

#endif  

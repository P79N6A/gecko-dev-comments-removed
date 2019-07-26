



#ifndef mozilla_system_volume_h__
#define mozilla_system_volume_h__

#include "VolumeCommand.h"
#include "nsIVolume.h"
#include "nsString.h"
#include "mozilla/Observer.h"
#include "mozilla/RefPtr.h"
#include "nsWhitespaceTokenizer.h"

namespace mozilla {
namespace system {










class Volume : public RefCounted<Volume>
{
public:
  Volume(const nsCSubstring &aVolumeName);

  typedef long STATE; 

  static const char *StateStr(STATE aState) { return NS_VolumeStateStr(aState); }
  const char *StateStr() const  { return StateStr(mState); }
  STATE State() const           { return mState; }

  const nsCString &Name() const { return mName; }
  const char *NameStr() const   { return mName.get(); }

  
  
  const nsCString &MountPoint() const { return mMountPoint; }

  bool MediaPresent() const     { return mMediaPresent; }

  typedef mozilla::Observer<Volume *>     EventObserver;
  typedef mozilla::ObserverList<Volume *> EventObserverList;

  
  static void RegisterObserver(EventObserver *aObserver);
  static void UnregisterObserver(EventObserver *aObserver);

private:
  friend class AutoMounter;         
  friend class VolumeManager;       
  friend class VolumeListCallback;  

  
  
  
  void StartMount(VolumeResponseCallback *aCallback);
  void StartUnmount(VolumeResponseCallback *aCallback);
  void StartShare(VolumeResponseCallback *aCallback);
  void StartUnshare(VolumeResponseCallback *aCallback);

  void SetState(STATE aNewState);
  void SetMediaPresent(bool aMediaPresent);
  void SetMountPoint(const nsCSubstring &aMountPoint);
  void StartCommand(VolumeCommand *aCommand);

  void HandleVoldResponse(int aResponseCode, nsCWhitespaceTokenizer &aTokenizer);

  bool              mMediaPresent;
  STATE             mState;
  const nsCString   mName;
  nsCString         mMountPoint;

  static EventObserverList mEventObserverList;
};

} 
} 

#endif  





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
  Volume(const nsCSubstring& aVolumeName);

  typedef long STATE; 

  static const char* StateStr(STATE aState) { return NS_VolumeStateStr(aState); }
  const char* StateStr() const  { return StateStr(mState); }
  STATE State() const           { return mState; }

  const nsCString& Name() const { return mName; }
  const char* NameStr() const   { return mName.get(); }

  
  
  const nsCString& MountPoint() const { return mMountPoint; }

  int32_t MountGeneration() const     { return mMountGeneration; }
  bool IsMountLocked() const          { return mMountLocked; }
  bool MediaPresent() const           { return mMediaPresent; }
  bool CanBeShared() const            { return mCanBeShared; }
  bool CanBeFormatted() const         { return CanBeShared(); }
  bool IsSharingEnabled() const       { return mCanBeShared && mSharingEnabled; }
  bool IsFormatRequested() const      { return CanBeFormatted() && mFormatRequested; }
  bool IsSharing() const              { return mIsSharing; }
  bool IsFormatting() const           { return mIsFormatting; }

  void SetSharingEnabled(bool aSharingEnabled);
  void SetFormatRequested(bool aFormatRequested);

  typedef mozilla::Observer<Volume *>     EventObserver;
  typedef mozilla::ObserverList<Volume *> EventObserverList;

  
  static void RegisterObserver(EventObserver* aObserver);
  static void UnregisterObserver(EventObserver* aObserver);

private:
  friend class AutoMounter;         
  friend class nsVolume;            
  friend class VolumeManager;       
  friend class VolumeListCallback;  

  
  
  
  void StartMount(VolumeResponseCallback* aCallback);
  void StartUnmount(VolumeResponseCallback* aCallback);
  void StartFormat(VolumeResponseCallback* aCallback);
  void StartShare(VolumeResponseCallback* aCallback);
  void StartUnshare(VolumeResponseCallback* aCallback);

  void SetIsSharing(bool aIsSharing);
  void SetIsFormatting(bool aIsFormatting);
  void SetState(STATE aNewState);
  void SetMediaPresent(bool aMediaPresent);
  void SetMountPoint(const nsCSubstring& aMountPoint);
  void StartCommand(VolumeCommand* aCommand);

  void HandleVoldResponse(int aResponseCode, nsCWhitespaceTokenizer& aTokenizer);

  static void UpdateMountLock(const nsACString& aVolumeName,
                              const int32_t& aMountGeneration,
                              const bool& aMountLocked);

  bool              mMediaPresent;
  STATE             mState;
  const nsCString   mName;
  nsCString         mMountPoint;
  int32_t           mMountGeneration;
  bool              mMountLocked;
  bool              mSharingEnabled;
  bool              mFormatRequested;
  bool              mCanBeShared;
  bool              mIsSharing;
  bool              mIsFormatting;

  static EventObserverList mEventObserverList;
};

} 
} 

#endif  

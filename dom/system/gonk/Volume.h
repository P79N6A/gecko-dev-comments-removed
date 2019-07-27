



#ifndef mozilla_system_volume_h__
#define mozilla_system_volume_h__

#include "VolumeCommand.h"
#include "nsIVolume.h"
#include "nsString.h"
#include "mozilla/Observer.h"
#include "nsISupportsImpl.h"
#include "nsWhitespaceTokenizer.h"

namespace mozilla {
namespace system {










class Volume MOZ_FINAL
{
public:
  NS_INLINE_DECL_REFCOUNTING(Volume)

  Volume(const nsCSubstring& aVolumeName);

  typedef long STATE; 

  static const char* StateStr(STATE aState) { return NS_VolumeStateStr(aState); }
  const char* StateStr() const  { return StateStr(mState); }
  STATE State() const           { return mState; }

  const nsCString& Name() const { return mName; }
  const char* NameStr() const   { return mName.get(); }

  
  
  const nsCString& MountPoint() const { return mMountPoint; }

  uint32_t Id() const                 { return mId; }

  int32_t MountGeneration() const     { return mMountGeneration; }
  bool IsMountLocked() const          { return mMountLocked; }
  bool MediaPresent() const           { return mMediaPresent; }
  bool CanBeShared() const            { return mCanBeShared; }
  bool CanBeFormatted() const         { return CanBeShared(); }
  bool CanBeMounted() const           { return CanBeShared(); }
  bool IsSharingEnabled() const       { return mCanBeShared && mSharingEnabled; }
  bool IsFormatRequested() const      { return CanBeFormatted() && mFormatRequested; }
  bool IsMountRequested() const       { return CanBeMounted() && mMountRequested; }
  bool IsUnmountRequested() const     { return CanBeMounted() && mUnmountRequested; }
  bool IsSharing() const              { return mIsSharing; }
  bool IsFormatting() const           { return mIsFormatting; }
  bool IsUnmounting() const           { return mIsUnmounting; }

  void SetSharingEnabled(bool aSharingEnabled);
  void SetFormatRequested(bool aFormatRequested);
  void SetMountRequested(bool aMountRequested);
  void SetUnmountRequested(bool aUnmountRequested);

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
  void SetIsUnmounting(bool aIsUnmounting);
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
  bool              mMountRequested;
  bool              mUnmountRequested;
  bool              mCanBeShared;
  bool              mIsSharing;
  bool              mIsFormatting;
  bool              mIsUnmounting;
  uint32_t          mId;                

  static EventObserverList mEventObserverList;
};

} 
} 

#endif  

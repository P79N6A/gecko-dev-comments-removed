



#ifndef mozilla_system_nsvolumeservice_h__
#define mozilla_system_nsvolumeservice_h__

#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "nsVolume.h"

namespace mozilla {

namespace dom {
class VolumeInfo;
} 

namespace system {

class WakeLockCallback;
class Volume;







class nsVolumeService final : public nsIVolumeService,
                              public nsIDOMMozWakeLockListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIVOLUMESERVICE
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER

  nsVolumeService();

  static already_AddRefed<nsVolumeService> GetSingleton();
  
  static void Shutdown();

  void DumpNoLock(const char* aLabel);

  void UpdateVolume(nsIVolume* aVolume, bool aNotifyObservers = true);
  void UpdateVolumeIOThread(const Volume* aVolume);

  void RecvVolumesFromParent(const nsTArray<dom::VolumeInfo>& aVolumes);
  void GetVolumesForIPC(nsTArray<dom::VolumeInfo>* aResult);

private:
  ~nsVolumeService();

  void CheckMountLock(const nsAString& aMountLockName,
                      const nsAString& aMountLockState);
  already_AddRefed<nsVolume> FindVolumeByMountLockName(const nsAString& aMountLockName);
  already_AddRefed<nsVolume> FindVolumeByName(const nsAString& aName);
  already_AddRefed<nsVolume> CreateOrFindVolumeByName(const nsAString& aName, bool aIsFake = false);

  Monitor mArrayMonitor;
  nsVolume::Array mVolumeArray;

  static StaticRefPtr<nsVolumeService> sSingleton;
  bool mGotVolumesFromParent;
};

} 
} 

#endif  

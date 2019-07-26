



#ifndef mozilla_system_nsvolumeservice_h__
#define mozilla_system_nsvolumeservice_h__

#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIObserver.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "nsVolume.h"
#include "Volume.h"

namespace mozilla {
namespace system {

class WakeLockCallback;







class nsVolumeService MOZ_FINAL : public nsIVolumeService,
                                  public nsIObserver,
                                  public nsIDOMMozWakeLockListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIVOLUMESERVICE
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER

  nsVolumeService();

  static already_AddRefed<nsVolumeService> GetSingleton();
  
  static void Shutdown();

  void UpdateVolume(nsIVolume* aVolume);
  void UpdateVolumeIOThread(const Volume* aVolume);

private:
  ~nsVolumeService();

  void CheckMountLock(const nsAString& aMountLockName,
                      const nsAString& aMountLockState);
  already_AddRefed<nsVolume> FindVolumeByMountLockName(const nsAString& aMountLockName);
  already_AddRefed<nsVolume> FindVolumeByName(const nsAString& aName);
  already_AddRefed<nsVolume> CreateOrFindVolumeByName(const nsAString& aName);

  Monitor mArrayMonitor;
  nsVolume::Array mVolumeArray;

  static StaticRefPtr<nsVolumeService> sSingleton;
};

} 
} 

#endif  

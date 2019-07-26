



#ifndef mozilla_system_nsvolumeservice_h__
#define mozilla_system_nsvolumeservice_h__

#include "nsCOMPtr.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "nsVolume.h"
#include "Volume.h"

namespace mozilla {
namespace system {







class nsVolumeService : public nsIVolumeService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOLUMESERVICE

  nsVolumeService();

  already_AddRefed<nsVolume> FindVolumeByName(const nsAString &aName);
  already_AddRefed<nsVolume> FindAddVolumeByName(const nsAString &aName);
  void UpdateVolume(const nsVolume *aVolume);
  static void UpdateVolumeIOThread(const Volume *aVolume);

private:
  ~nsVolumeService();

  nsVolume::Array  mVolumeArray;
};

} 
} 

#endif  

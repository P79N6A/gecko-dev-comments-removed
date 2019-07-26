



#ifndef mozilla_system_nsvolumemountlock_h__
#define mozilla_system_nsvolumemountlock_h__

#include "nsIVolumeMountLock.h"

#include "nsIDOMWakeLock.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace system {







class nsVolumeMountLock MOZ_FINAL : public nsIVolumeMountLock,
                                    public nsIObserver,
                                    public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIVOLUMEMOUNTLOCK

  static already_AddRefed<nsVolumeMountLock> Create(const nsAString& volumeName);

  const nsString& VolumeName() const  { return mVolumeName; }

private:
  nsVolumeMountLock(const nsAString& aVolumeName);
  ~nsVolumeMountLock();

  nsresult Init();

  nsString                    mVolumeName;
  int32_t                     mVolumeGeneration;
  nsCOMPtr<nsIDOMMozWakeLock> mWakeLock;
  bool                        mUnlocked;
};

} 
} 

#endif  

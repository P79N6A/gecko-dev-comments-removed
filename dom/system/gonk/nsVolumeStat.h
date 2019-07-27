



#ifndef mozilla_system_nsvolumestat_h__
#define mozilla_system_nsvolumestat_h__

#include "nsIVolumeStat.h"
#include "nsString.h"
#include <sys/statfs.h>

namespace mozilla {
namespace system {

class nsVolumeStat final : public nsIVolumeStat
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOLUMESTAT

  nsVolumeStat(const nsAString& aPath);

protected:
  ~nsVolumeStat() {}

private:
  struct statfs mStat;
};

} 
} 

#endif  

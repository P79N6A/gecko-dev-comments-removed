



#include "nsVolumeStat.h"
#include "nsString.h"

namespace mozilla {
namespace system {

NS_IMPL_ISUPPORTS(nsVolumeStat, nsIVolumeStat)

nsVolumeStat::nsVolumeStat(const nsAString& aPath)
{
  if (statfs(NS_ConvertUTF16toUTF8(aPath).get(), &mStat) != 0) {
    memset(&mStat, 0, sizeof(mStat));
  }
}


NS_IMETHODIMP nsVolumeStat::GetTotalBytes(int64_t* aTotalBytes)
{
  *aTotalBytes = mStat.f_blocks * mStat.f_bsize;
  return NS_OK;
}


NS_IMETHODIMP nsVolumeStat::GetFreeBytes(int64_t* aFreeBytes)
{
  *aFreeBytes = mStat.f_bfree * mStat.f_bsize;
  return NS_OK;
}

} 
} 

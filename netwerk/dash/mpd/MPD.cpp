









































#include "nsTArray.h"
#include "MPD.h"


namespace mozilla {
namespace net {

void
MPD::AddPeriod(Period* aPeriod)
{
  NS_ENSURE_TRUE(aPeriod,);
  
  if (!mPeriods.Contains(aPeriod)) {
    mPeriods.AppendElement(aPeriod);
  }
}

Period const *
MPD::GetPeriod(uint32_t aIndex) const
{
  NS_ENSURE_TRUE(aIndex < mPeriods.Length(), nullptr);
  return mPeriods[aIndex];
}

uint32_t const
MPD::GetNumPeriods() const
{
  return mPeriods.Length();
}

void
MPD::AddBaseUrl(nsAString const& aUrl)
{
  NS_ENSURE_FALSE(aUrl.IsEmpty(),);
  
  if (!mBaseUrls.Contains(aUrl)) {
    mBaseUrls.AppendElement(aUrl);
  }
}

nsAString const&
MPD::GetBaseUrl(uint32_t aIndex) const
{
  NS_ENSURE_TRUE(aIndex < mBaseUrls.Length(), NS_LITERAL_STRING(""));
  return mBaseUrls[aIndex];
}

}
}

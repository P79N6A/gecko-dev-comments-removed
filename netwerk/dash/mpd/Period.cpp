









































#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "Period.h"

namespace mozilla {
namespace net {

AdaptationSet const *
Period::GetAdaptationSet(uint32_t aIndex) const
{
  NS_ENSURE_TRUE(aIndex < mAdaptationSets.Length(), nullptr);
  return mAdaptationSets[aIndex];
}

void
Period::AddAdaptationSet(AdaptationSet* aAdaptationSet)
{
  NS_ENSURE_TRUE(aAdaptationSet,);
  
  if (!mAdaptationSets.Contains(aAdaptationSet)) {
    mAdaptationSets.AppendElement(aAdaptationSet);
  }
}

uint16_t const
Period::GetNumAdaptationSets() const
{
  return mAdaptationSets.Length();
}

double const
Period::GetStart() const
{
  return mStart;
}

double const
Period::GetDuration() const
{
  return mDuration;
}

void
Period::SetStart(double const aStart)
{
  mStart = aStart;
}

void
Period::SetDuration(double const aDuration)
{
  mDuration = aDuration;
}

}
}












































#include "AdaptationSet.h"

namespace mozilla {
namespace net {

int32_t
AdaptationSet::GetWidth() const
{
  return mWidth;
}

void
AdaptationSet::SetWidth(int32_t const aWidth)
{
  mWidth = aWidth;
}

int32_t
AdaptationSet::GetHeight() const
{
  return mHeight;
}

void
AdaptationSet::SetHeight(int32_t const aHeight)
{
  mHeight = aHeight;
}

void
AdaptationSet::GetMIMEType(nsAString& aMIMEType) const
{
  aMIMEType = mMIMEType;
}

void
AdaptationSet::SetMIMEType(nsAString const &aMIMEType)
{
  NS_ENSURE_FALSE(aMIMEType.IsEmpty(),);
  mMIMEType = aMIMEType;
}

Representation const *
AdaptationSet::GetRepresentation(uint32_t aIndex) const
{
  NS_ENSURE_TRUE(aIndex < mRepresentations.Length(), nullptr);
  return mRepresentations[aIndex];
}

void
AdaptationSet::AddRepresentation(Representation* aRep)
{
  NS_ENSURE_TRUE(aRep,);
  
  if (!mRepresentations.Contains(aRep)) {
    mRepresentations.AppendElement(aRep);
  }
}

uint16_t
AdaptationSet::GetNumRepresentations() const
{
  return mRepresentations.Length();
}

void
AdaptationSet::EnableBitstreamSwitching(bool aEnable)
{
  mIsBitstreamSwitching = aEnable;
}

bool
AdaptationSet::IsBitstreamSwitchingEnabled() const
{
  return mIsBitstreamSwitching;
}

}
}

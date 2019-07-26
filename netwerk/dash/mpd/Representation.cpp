









































#include "nsTArray.h"
#include "Representation.h"

namespace mozilla {
namespace net {

int64_t const
Representation::GetBitrate() const
{
  return mBitrate;
}

void
Representation::SetBitrate(int64_t aBitrate)
{
  mBitrate = aBitrate;
}

void
Representation::SetWidth(int32_t const aWidth)
{
  mWidth = aWidth;
}

int32_t const
Representation::GetWidth() const
{
  return mWidth;
}

void
Representation::SetHeight(int32_t aHeight)
{
  mHeight = aHeight;
}

int32_t const
Representation::GetHeight() const
{
  return mHeight;
}

void
Representation::AddBaseUrl(nsAString const& aUrl)
{
  NS_ENSURE_FALSE(aUrl.IsEmpty(),);
  
  if (!mBaseUrls.Contains(aUrl)) {
    mBaseUrls.AppendElement(aUrl);
  }
}

nsAString const &
Representation::GetBaseUrl(uint32_t aIndex) const
{
  NS_ENSURE_TRUE(aIndex < mBaseUrls.Length(), NS_LITERAL_STRING(""));
  return mBaseUrls[aIndex];
}

SegmentBase const*
Representation::GetSegmentBase() const
{
  return mSegmentBase;
}

void
Representation::SetSegmentBase(SegmentBase* aBase)
{
  NS_ENSURE_TRUE(aBase,);
  
  if (mSegmentBase != aBase
      || (mSegmentBase && (*mSegmentBase != *aBase))) {
    mSegmentBase = aBase;
  }
}

}
}

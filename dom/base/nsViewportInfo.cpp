



#include "nsViewportInfo.h"
#include "mozilla/Assertions.h"
#include <algorithm>

using namespace mozilla;

void
nsViewportInfo::SetDefaultZoom(const CSSToScreenScale& aDefaultZoom)
{
  MOZ_ASSERT(aDefaultZoom.scale >= 0.0f);
  mDefaultZoom = aDefaultZoom;
}

void
nsViewportInfo::ConstrainViewportValues()
{
  
  
  mMaxZoom = std::max(mMinZoom, mMaxZoom);

  mDefaultZoom = mDefaultZoom < mMaxZoom ? mDefaultZoom : mMaxZoom;
  mDefaultZoom = mDefaultZoom > mMinZoom ? mDefaultZoom : mMinZoom;
}

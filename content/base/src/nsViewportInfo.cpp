



#include "nsViewportInfo.h"
#include "mozilla/Assertions.h"
#include <algorithm>

void
nsViewportInfo::SetDefaultZoom(const double aDefaultZoom)
{
  MOZ_ASSERT(aDefaultZoom >= 0.0f);
  mDefaultZoom = aDefaultZoom;
}

void
nsViewportInfo::ConstrainViewportValues()
{
  
  
  mMaxZoom = std::max(mMinZoom, mMaxZoom);

  mDefaultZoom = std::min(mDefaultZoom, mMaxZoom);
  mDefaultZoom = std::max(mDefaultZoom, mMinZoom);
}





#include "nsViewportInfo.h"

void
nsViewportInfo::SetDefaultZoom(const double aDefaultZoom)
{
  MOZ_ASSERT(aDefaultZoom >= 0.0f);
  mDefaultZoom = aDefaultZoom;
}

void
nsViewportInfo::ConstrainViewportValues()
{
  
  
  mMaxZoom = NS_MAX(mMinZoom, mMaxZoom);

  mDefaultZoom = NS_MIN(mDefaultZoom, mMaxZoom);
  mDefaultZoom = NS_MAX(mDefaultZoom, mMinZoom);
}

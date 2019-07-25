






































#include "nsFontInflationData.h"
#include "FramePropertyTable.h"

using namespace mozilla;

static void
DestroyFontInflationData(void *aPropertyValue)
{
  delete static_cast<nsFontInflationData*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(FontInflationDataProperty, DestroyFontInflationData);

 nsFontInflationData*
nsFontInflationData::FindFontInflationDataFor(const nsIFrame *aFrame)
{
  
  const nsIFrame *bfc = FlowRootFor(aFrame);

  return static_cast<nsFontInflationData*>(
             bfc->Properties().Get(FontInflationDataProperty()));
}

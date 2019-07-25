






































#ifndef nsFontInflationData_h_
#define nsFontInflationData_h_

#include "nsIFrame.h"
#include "nsLayoutUtils.h"
#include "nsBlockFrame.h"

class nsFontInflationData
{
public:

  static nsFontInflationData* FindFontInflationDataFor(const nsIFrame *aFrame);

private:

  static const nsIFrame* FlowRootFor(const nsIFrame *aFrame)
  {
    while (!(aFrame->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT)) {
      aFrame = aFrame->GetParent();
    }
    return aFrame;
  }

};

#endif 








#ifndef nsFontInflationData_h_
#define nsFontInflationData_h_

#include "nsContainerFrame.h"

struct nsHTMLReflowState;

class nsFontInflationData
{
public:

  static nsFontInflationData* FindFontInflationDataFor(const nsIFrame *aFrame);

  
  
  static bool
    UpdateFontInflationDataWidthFor(const nsHTMLReflowState& aReflowState);

  static void MarkFontInflationDataTextDirty(nsIFrame *aFrame);

  bool InflationEnabled() {
    if (mTextDirty) {
      ScanText();
    }
    return mInflationEnabled;
  }

  nscoord EffectiveWidth() const {
    return mNCAWidth;
  }

private:

  explicit nsFontInflationData(nsIFrame* aBFCFrame);

  nsFontInflationData(const nsFontInflationData&) MOZ_DELETE;
  void operator=(const nsFontInflationData&) MOZ_DELETE;

  void UpdateWidth(const nsHTMLReflowState &aReflowState);
  enum SearchDirection { eFromStart, eFromEnd };
  static nsIFrame* FindEdgeInflatableFrameIn(nsIFrame *aFrame,
                                             SearchDirection aDirection);

  void MarkTextDirty() { mTextDirty = true; }
  void ScanText();
  
  
  
  
  
  void ScanTextIn(nsIFrame *aFrame);

  static const nsIFrame* FlowRootFor(const nsIFrame *aFrame)
  {
    while (!(aFrame->GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT)) {
      aFrame = aFrame->GetParent();
    }
    return aFrame;
  }

  nsIFrame *mBFCFrame;
  nscoord mNCAWidth;
  nscoord mTextAmount, mTextThreshold;
  bool mInflationEnabled; 
  bool mTextDirty;
};

#endif 

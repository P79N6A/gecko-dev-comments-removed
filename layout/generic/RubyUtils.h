





#ifndef mozilla_RubyUtils_h_
#define mozilla_RubyUtils_h_

#include "nsGkAtoms.h"
#include "nsIFrame.h"

namespace mozilla {






























class RubyUtils
{
public:
  static inline bool IsExpandableRubyBox(nsIFrame* aFrame)
  {
    nsIAtom* type = aFrame->GetType();
    return type == nsGkAtoms::rubyBaseFrame ||
           type == nsGkAtoms::rubyTextFrame ||
           type == nsGkAtoms::rubyBaseContainerFrame ||
           type == nsGkAtoms::rubyTextContainerFrame;
  }

  static void SetReservedISize(nsIFrame* aFrame, nscoord aISize);
  static void ClearReservedISize(nsIFrame* aFrame);
  static nscoord GetReservedISize(nsIFrame* aFrame);
};

}

#endif 

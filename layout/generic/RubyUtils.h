





#ifndef mozilla_RubyUtils_h_
#define mozilla_RubyUtils_h_

#include "nsGkAtoms.h"
#include "nsRubyTextContainerFrame.h"

class nsRubyBaseContainerFrame;

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





class MOZ_STACK_CLASS RubyTextContainerIterator
{
public:
  explicit RubyTextContainerIterator(nsRubyBaseContainerFrame* aBaseContainer);

  void Next();
  bool AtEnd() const { return !mFrame; }
  nsRubyTextContainerFrame* GetTextContainer() const
  {
    return static_cast<nsRubyTextContainerFrame*>(mFrame);
  }

private:
  nsIFrame* mFrame;
};

}

#endif 

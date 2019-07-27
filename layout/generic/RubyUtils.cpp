





#include "RubyUtils.h"
#include "nsRubyBaseContainerFrame.h"

using namespace mozilla;

NS_DECLARE_FRAME_PROPERTY(ReservedISize, nullptr);

union NSCoordValue
{
  nscoord mCoord;
  void* mPointer;
  static_assert(sizeof(nscoord) <= sizeof(void*),
                "Cannot store nscoord in pointer");
};

 void
RubyUtils::SetReservedISize(nsIFrame* aFrame, nscoord aISize)
{
  MOZ_ASSERT(IsExpandableRubyBox(aFrame));
  NSCoordValue value = { aISize };
  aFrame->Properties().Set(ReservedISize(), value.mPointer);
}

 void
RubyUtils::ClearReservedISize(nsIFrame* aFrame)
{
  MOZ_ASSERT(IsExpandableRubyBox(aFrame));
  aFrame->Properties().Remove(ReservedISize());
}

 nscoord
RubyUtils::GetReservedISize(nsIFrame* aFrame)
{
  MOZ_ASSERT(IsExpandableRubyBox(aFrame));
  NSCoordValue value;
  value.mPointer = aFrame->Properties().Get(ReservedISize());
  return value.mCoord;
}

RubyTextContainerIterator::RubyTextContainerIterator(
  nsRubyBaseContainerFrame* aBaseContainer)
{
  mFrame = aBaseContainer;
  Next();
}

void
RubyTextContainerIterator::Next()
{
  MOZ_ASSERT(mFrame, "Should have checked AtEnd()");
  mFrame = mFrame->GetNextSibling();
  if (mFrame && mFrame->GetType() != nsGkAtoms::rubyTextContainerFrame) {
    mFrame = nullptr;
  }
}

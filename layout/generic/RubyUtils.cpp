





#include "RubyUtils.h"
#include "nsRubyFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"
#include "nsRubyBaseContainerFrame.h"
#include "nsRubyTextContainerFrame.h"

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

AutoRubyTextContainerArray::AutoRubyTextContainerArray(
  nsRubyBaseContainerFrame* aBaseContainer)
{
  for (nsIFrame* frame = aBaseContainer->GetNextSibling();
       frame && frame->GetType() == nsGkAtoms::rubyTextContainerFrame;
       frame = frame->GetNextSibling()) {
    AppendElement(static_cast<nsRubyTextContainerFrame*>(frame));
  }
}

RubySegmentEnumerator::RubySegmentEnumerator(nsRubyFrame* aRubyFrame)
{
  nsIFrame* frame = aRubyFrame->GetFirstPrincipalChild();
  MOZ_ASSERT(!frame ||
             frame->GetType() == nsGkAtoms::rubyBaseContainerFrame);
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

void
RubySegmentEnumerator::Next()
{
  MOZ_ASSERT(mBaseContainer);
  nsIFrame* frame = mBaseContainer->GetNextSibling();
  while (frame && frame->GetType() != nsGkAtoms::rubyBaseContainerFrame) {
    frame = frame->GetNextSibling();
  }
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

RubyColumnEnumerator::RubyColumnEnumerator(
  nsRubyBaseContainerFrame* aBaseContainer,
  const AutoRubyTextContainerArray& aTextContainers)
  : mAtIntraLevelWhitespace(false)
{
  const uint32_t rtcCount = aTextContainers.Length();
  mFrames.SetCapacity(rtcCount + 1);

  nsIFrame* rbFrame = aBaseContainer->GetFirstPrincipalChild();
  MOZ_ASSERT(!rbFrame || rbFrame->GetType() == nsGkAtoms::rubyBaseFrame);
  mFrames.AppendElement(static_cast<nsRubyContentFrame*>(rbFrame));
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* container = aTextContainers[i];
    
    
    nsIFrame* rtFrame = !container->IsSpanContainer() ?
      container->GetFirstPrincipalChild() : nullptr;
    MOZ_ASSERT(!rtFrame || rtFrame->GetType() == nsGkAtoms::rubyTextFrame);
    mFrames.AppendElement(static_cast<nsRubyContentFrame*>(rtFrame));
  }

  
  
  
  
  
  
  
  
  
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    nsRubyContentFrame* frame = mFrames[i];
    if (frame && frame->IsIntraLevelWhitespace()) {
      mAtIntraLevelWhitespace = true;
      break;
    }
  }
}

void
RubyColumnEnumerator::Next()
{
  bool advancingToIntraLevelWhitespace = false;
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    nsRubyContentFrame* frame = mFrames[i];
    
    
    
    
    
    if (frame && (!mAtIntraLevelWhitespace ||
                  frame->IsIntraLevelWhitespace())) {
      nsIFrame* nextSibling = frame->GetNextSibling();
      MOZ_ASSERT(!nextSibling || nextSibling->GetType() == frame->GetType(),
                 "Frame type should be identical among a level");
      mFrames[i] = frame = static_cast<nsRubyContentFrame*>(nextSibling);
      if (!advancingToIntraLevelWhitespace &&
          frame && frame->IsIntraLevelWhitespace()) {
        advancingToIntraLevelWhitespace = true;
      }
    }
  }
  MOZ_ASSERT(!advancingToIntraLevelWhitespace || !mAtIntraLevelWhitespace,
             "Should never have adjacent intra-level whitespace columns");
  mAtIntraLevelWhitespace = advancingToIntraLevelWhitespace;
}

bool
RubyColumnEnumerator::AtEnd() const
{
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    if (mFrames[i]) {
      return false;
    }
  }
  return true;
}

nsRubyContentFrame*
RubyColumnEnumerator::GetFrameAtLevel(uint32_t aIndex) const
{
  
  
  
  
  
  
  nsRubyContentFrame* frame = mFrames[aIndex];
  return !mAtIntraLevelWhitespace ||
         (frame && frame->IsIntraLevelWhitespace()) ? frame : nullptr;
}

void
RubyColumnEnumerator::GetColumn(RubyColumn& aColumn) const
{
  nsRubyContentFrame* rbFrame = GetFrameAtLevel(0);
  MOZ_ASSERT(!rbFrame || rbFrame->GetType() == nsGkAtoms::rubyBaseFrame);
  aColumn.mBaseFrame = static_cast<nsRubyBaseFrame*>(rbFrame);
  aColumn.mTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 1, iend = mFrames.Length(); i < iend; i++) {
    nsRubyContentFrame* rtFrame = GetFrameAtLevel(i);
    MOZ_ASSERT(!rtFrame || rtFrame->GetType() == nsGkAtoms::rubyTextFrame);
    aColumn.mTextFrames.AppendElement(static_cast<nsRubyTextFrame*>(rtFrame));
  }
  aColumn.mIsIntraLevelWhitespace = mAtIntraLevelWhitespace;
}

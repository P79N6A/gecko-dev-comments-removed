




#include "DisplayListClipState.h"

#include "nsDisplayList.h"

namespace mozilla {

const DisplayItemClip*
DisplayListClipState::GetCurrentCombinedClip(nsDisplayListBuilder* aBuilder)
{
  if (mCurrentCombinedClip) {
    return mCurrentCombinedClip;
  }
  if (!mClipContentDescendants && !mClipContainingBlockDescendants) {
    return nullptr;
  }
  void* mem = aBuilder->Allocate(sizeof(DisplayItemClip));
  if (mClipContentDescendants) {
    DisplayItemClip* newClip =
      new (mem) DisplayItemClip(*mClipContentDescendants);
    if (mClipContainingBlockDescendants) {
      newClip->IntersectWith(*mClipContainingBlockDescendants);
    }
    mCurrentCombinedClip = newClip;
  } else {
    mCurrentCombinedClip =
      new (mem) DisplayItemClip(*mClipContainingBlockDescendants);
  }
  return mCurrentCombinedClip;
}

void
DisplayListClipState::ClipContainingBlockDescendants(const nsRect& aRect,
                                                     const nscoord* aRadii,
                                                     DisplayItemClip& aClipOnStack)
{
  if (aRadii) {
    aClipOnStack.SetTo(aRect, aRadii);
  } else {
    aClipOnStack.SetTo(aRect);
  }
  if (mClipContainingBlockDescendants) {
    aClipOnStack.IntersectWith(*mClipContainingBlockDescendants);
  }
  mClipContainingBlockDescendants = &aClipOnStack;
  mCurrentCombinedClip = nullptr;
}

void
DisplayListClipState::ClipContentDescendants(const nsRect& aRect,
                                             DisplayItemClip& aClipOnStack)
{
  aClipOnStack.SetTo(aRect);
  if (mClipContentDescendants) {
    aClipOnStack.IntersectWith(*mClipContentDescendants);
  }
  mClipContentDescendants = &aClipOnStack;
  mCurrentCombinedClip = nullptr;
}

void
DisplayListClipState::ClipContainingBlockDescendantsToContentBox(nsDisplayListBuilder* aBuilder,
                                                                 nsIFrame* aFrame,
                                                                 DisplayItemClip& aClipOnStack,
                                                                 uint32_t aFlags)
{
  nscoord radii[8];
  bool hasBorderRadius = aFrame->GetContentBoxBorderRadii(radii);
  if (!hasBorderRadius && (aFlags & ASSUME_DRAWING_RESTRICTED_TO_CONTENT_RECT)) {
    return;
  }

  nsRect clipRect = aFrame->GetContentRectRelativeToSelf() +
    aBuilder->ToReferenceFrame(aFrame);
  
  
  ClipContainingBlockDescendants(clipRect, hasBorderRadius ? radii : nullptr,
                                 aClipOnStack);
}

DisplayListClipState::AutoSaveRestore::AutoSaveRestore(nsDisplayListBuilder* aBuilder)
  : mState(aBuilder->ClipState())
  , mSavedState(aBuilder->ClipState())
  , mClipUsed(false)
  , mRestored(false)
{}

}

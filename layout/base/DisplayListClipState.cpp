




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
    mCurrentCombinedClip =
      new (mem) DisplayItemClip(*mClipContentDescendants);
    if (mClipContainingBlockDescendants) {
      mCurrentCombinedClip->IntersectWith(*mClipContainingBlockDescendants);
    }
  } else {
    mCurrentCombinedClip =
      new (mem) DisplayItemClip(*mClipContainingBlockDescendants);
  }
  return mCurrentCombinedClip;
}

}






#ifndef DISPLAYLISTCLIPSTATE_H_
#define DISPLAYLISTCLIPSTATE_H_

#include "DisplayItemClip.h"

class nsIFrame;
class nsDisplayListBuilder;

namespace mozilla {





class DisplayListClipState {
public:
  DisplayListClipState()
    : mClipContentDescendants(nullptr)
    , mClipContainingBlockDescendants(nullptr)
    , mCurrentCombinedClip(nullptr)
  {}

  const DisplayItemClip* GetCurrentCombinedClip(nsDisplayListBuilder* aBuilder);

  const DisplayItemClip* GetClipForContainingBlockDescendants()
  {
    return mClipContainingBlockDescendants;
  }

  const DisplayItemClip* GetClipForContentDescendants()
  {
    return mClipContentDescendants;
  }

  void SetClipForContainingBlockDescendants(const DisplayItemClip* aClip)
  {
    mClipContainingBlockDescendants = aClip;
    mCurrentCombinedClip = nullptr;
  }
  void SetClipForContentDescendants(const DisplayItemClip* aClip)
  {
    mClipContentDescendants = aClip;
    mCurrentCombinedClip = nullptr;
  }

private:
  




  const DisplayItemClip* mClipContentDescendants;
  




  const DisplayItemClip* mClipContainingBlockDescendants;
  






  const DisplayItemClip* mCurrentCombinedClip;
};

}

#endif 

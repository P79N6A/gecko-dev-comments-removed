




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

  const DisplayItemClip* GetClipForContainingBlockDescendants() const
  {
    return mClipContainingBlockDescendants;
  }
  const DisplayItemClip* GetClipForContentDescendants() const
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

  void Clear()
  {
    mClipContentDescendants = nullptr;
    mClipContainingBlockDescendants = nullptr;
    mCurrentCombinedClip = nullptr;
  }

  




  void ClipContainingBlockDescendants(const nsRect& aRect,
                                      const nscoord* aRadii,
                                      DisplayItemClip& aClipOnStack);

  void ClipContentDescendants(const nsRect& aRect,
                              DisplayItemClip& aClipOnStack);

  enum {
    ASSUME_DRAWING_RESTRICTED_TO_CONTENT_RECT = 0x01
  };
  







  void ClipContainingBlockDescendantsToContentBox(nsDisplayListBuilder* aBuilder,
                                                  nsIFrame* aFrame,
                                                  DisplayItemClip& aClipOnStack,
                                                  uint32_t aFlags = 0);

  class AutoSaveRestore;
  friend class AutoSaveRestore;

  class AutoClipContainingBlockDescendantsToContentBox;

private:
  




  const DisplayItemClip* mClipContentDescendants;
  





  const DisplayItemClip* mClipContainingBlockDescendants;
  






  const DisplayItemClip* mCurrentCombinedClip;
};

class DisplayListClipState::AutoSaveRestore {
public:
  AutoSaveRestore(DisplayListClipState& aState)
    : mState(aState)
    , mSavedState(aState)
  {}
  void Restore()
  {
    mState = mSavedState;
  }
  ~AutoSaveRestore()
  {
    mState = mSavedState;
  }
protected:
  DisplayListClipState& mState;
  DisplayListClipState mSavedState;
};

class DisplayListClipState::AutoClipContainingBlockDescendantsToContentBox : public AutoSaveRestore {
public:
  AutoClipContainingBlockDescendantsToContentBox(nsDisplayListBuilder* aBuilder,
                                                 nsIFrame* aFrame,
                                                 uint32_t aFlags = 0);
protected:
  DisplayItemClip mClip;
};

}

#endif 

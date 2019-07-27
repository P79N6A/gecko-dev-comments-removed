




#ifndef DISPLAYLISTCLIPSTATE_H_
#define DISPLAYLISTCLIPSTATE_H_

#include "DisplayItemClip.h"

#include "mozilla/DebugOnly.h"

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

  class AutoSaveRestore;
  friend class AutoSaveRestore;

  class AutoClipContainingBlockDescendantsToContentBox;
  friend class AutoClipContainingBlockDescendantsToContentBox;

  class AutoClipMultiple;
  friend class AutoClipMultiple;

  enum {
    ASSUME_DRAWING_RESTRICTED_TO_CONTENT_RECT = 0x01
  };

private:
  void SetClipForContainingBlockDescendants(const DisplayItemClip* aClip)
  {
    mClipContainingBlockDescendants = aClip;
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
                              const nscoord* aRadii,
                              DisplayItemClip& aClipOnStack);
  void ClipContentDescendants(const nsRect& aRect,
                              const nsRect& aRoundedRect,
                              const nscoord* aRadii,
                              DisplayItemClip& aClipOnStack);

  







  void ClipContainingBlockDescendantsToContentBox(nsDisplayListBuilder* aBuilder,
                                                  nsIFrame* aFrame,
                                                  DisplayItemClip& aClipOnStack,
                                                  uint32_t aFlags);

  




  const DisplayItemClip* mClipContentDescendants;
  





  const DisplayItemClip* mClipContainingBlockDescendants;
  






  const DisplayItemClip* mCurrentCombinedClip;
};








class DisplayListClipState::AutoSaveRestore {
public:
  explicit AutoSaveRestore(nsDisplayListBuilder* aBuilder);
  void Restore()
  {
    mState = mSavedState;
    mRestored = true;
  }
  ~AutoSaveRestore()
  {
    mState = mSavedState;
  }

  void Clear()
  {
    NS_ASSERTION(!mRestored, "Already restored!");
    mState.Clear();
    mClipUsed = false;
  }

  




  void ClipContainingBlockDescendants(const nsRect& aRect,
                                      const nscoord* aRadii = nullptr)
  {
    NS_ASSERTION(!mRestored, "Already restored!");
    NS_ASSERTION(!mClipUsed, "mClip already used");
    mClipUsed = true;
    mState.ClipContainingBlockDescendants(aRect, aRadii, mClip);
  }

  void ClipContentDescendants(const nsRect& aRect,
                              const nscoord* aRadii = nullptr)
  {
    NS_ASSERTION(!mRestored, "Already restored!");
    NS_ASSERTION(!mClipUsed, "mClip already used");
    mClipUsed = true;
    mState.ClipContentDescendants(aRect, aRadii, mClip);
  }

  void ClipContentDescendants(const nsRect& aRect,
                              const nsRect& aRoundedRect,
                              const nscoord* aRadii = nullptr)
  {
    NS_ASSERTION(!mRestored, "Already restored!");
    NS_ASSERTION(!mClipUsed, "mClip already used");
    mClipUsed = true;
    mState.ClipContentDescendants(aRect, aRoundedRect, aRadii, mClip);
  }

  







  void ClipContainingBlockDescendantsToContentBox(nsDisplayListBuilder* aBuilder,
                                                  nsIFrame* aFrame,
                                                  uint32_t aFlags = 0)
  {
    NS_ASSERTION(!mRestored, "Already restored!");
    NS_ASSERTION(!mClipUsed, "mClip already used");
    mClipUsed = true;
    mState.ClipContainingBlockDescendantsToContentBox(aBuilder, aFrame, mClip, aFlags);
  }

protected:
  DisplayListClipState& mState;
  DisplayListClipState mSavedState;
  DisplayItemClip mClip;
  DebugOnly<bool> mClipUsed;
  DebugOnly<bool> mRestored;
};

class DisplayListClipState::AutoClipContainingBlockDescendantsToContentBox : public AutoSaveRestore {
public:
  AutoClipContainingBlockDescendantsToContentBox(nsDisplayListBuilder* aBuilder,
                                                 nsIFrame* aFrame,
                                                 uint32_t aFlags = 0)
    : AutoSaveRestore(aBuilder)
  {
    mClipUsed = true;
    mState.ClipContainingBlockDescendantsToContentBox(aBuilder, aFrame, mClip, aFlags);
  }
};






class DisplayListClipState::AutoClipMultiple : public AutoSaveRestore {
public:
  explicit AutoClipMultiple(nsDisplayListBuilder* aBuilder)
    : AutoSaveRestore(aBuilder)
    , mExtraClipUsed(false)
  {}

  


  void SetClipForContainingBlockDescendants(const DisplayItemClip* aClip)
  {
    mState.SetClipForContainingBlockDescendants(aClip);
  }

  




  void ClipContainingBlockDescendantsExtra(const nsRect& aRect,
                                           const nscoord* aRadii)
  {
    NS_ASSERTION(!mRestored, "Already restored!");
    NS_ASSERTION(!mExtraClipUsed, "mExtraClip already used");
    mExtraClipUsed = true;
    mState.ClipContainingBlockDescendants(aRect, aRadii, mExtraClip);
  }

protected:
  DisplayItemClip mExtraClip;
  DebugOnly<bool> mExtraClipUsed;
};

}

#endif 

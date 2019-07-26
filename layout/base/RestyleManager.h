









#ifndef mozilla_RestyleManager_h
#define mozilla_RestyleManager_h

#include "nsISupportsImpl.h"
#include "nsChangeHint.h"
#include "RestyleTracker.h"
#include "nsPresContext.h"

class nsRefreshDriver;
class nsIFrame;

namespace mozilla {

namespace dom {
  class Element;
} 

class RestyleManager {
public:
  friend class ::nsRefreshDriver;
  friend class RestyleTracker;

  typedef mozilla::dom::Element Element;

  RestyleManager(nsPresContext* aPresContext);

  NS_INLINE_DECL_REFCOUNTING(mozilla::RestyleManager)

  void Disconnect() {
    mPresContext = nullptr;
  }

  nsPresContext* PresContext() const {
    MOZ_ASSERT(mPresContext);
    return mPresContext;
  }

  nsCSSFrameConstructor* FrameConstructor() const
    { return PresContext()->FrameConstructor(); }

  
  
  void NotifyDestroyingFrame(nsIFrame* aFrame);

  
  
  nsresult ContentStateChanged(nsIContent*   aContent,
                               nsEventStates aStateMask);

  
  void AttributeWillChange(Element* aElement,
                           int32_t  aNameSpaceID,
                           nsIAtom* aAttribute,
                           int32_t  aModType);
  
  
  void AttributeChanged(Element* aElement,
                        int32_t  aNameSpaceID,
                        nsIAtom* aAttribute,
                        int32_t  aModType);

  
  
  uint32_t GetHoverGeneration() const { return mHoverGeneration; }

  
  
  uint64_t GetAnimationGeneration() const { return mAnimationGeneration; }

  
  
  
  
  
  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray);

private:
  void RestyleForEmptyChange(Element* aContainer);

public:
  
  
  
  void RestyleForInsertOrChange(Element* aContainer, nsIContent* aChild);

  
  
  
  
  
  void RestyleForRemove(Element* aContainer,
                        nsIContent* aOldChild,
                        nsIContent* aFollowingSibling);

  
  
  void RestyleForAppend(Element* aContainer, nsIContent* aFirstNewContent);

  
  
  
  
  
  
  
  void ProcessPendingRestyles();

  
  
  
  void RebuildAllStyleData(nsChangeHint aExtraHint);

  
  
  void DoRebuildAllStyleData(RestyleTracker& aRestyleTracker,
                             nsChangeHint aExtraHint);

  
  void PostRestyleEvent(Element* aElement,
                        nsRestyleHint aRestyleHint,
                        nsChangeHint aMinChangeHint)
  {
    if (mPresContext) {
      PostRestyleEventCommon(aElement, aRestyleHint, aMinChangeHint,
                             mPresContext->IsProcessingAnimationStyleChange());
    }
  }

  
  void PostAnimationRestyleEvent(Element* aElement,
                                 nsRestyleHint aRestyleHint,
                                 nsChangeHint aMinChangeHint)
  {
    PostRestyleEventCommon(aElement, aRestyleHint, aMinChangeHint, true);
  }

  void PostRestyleEventForLazyConstruction()
  {
    PostRestyleEventInternal(true);
  }

  void FlushOverflowChangedTracker()
  {
    mOverflowChangedTracker.Flush();
  }

private:
  














  void PostRestyleEventCommon(Element* aElement,
                              nsRestyleHint aRestyleHint,
                              nsChangeHint aMinChangeHint,
                              bool aForAnimation);
  void PostRestyleEventInternal(bool aForLazyConstruction);

public:
  









  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint);

  void SetPromoteReflowsToReframeRoot(bool aPromote)
    { mPromoteReflowsToReframeRoot = aPromote; }

private:
  
  
  void RestyleElement(Element* aElement,
                      nsIFrame*       aPrimaryFrame,
                      nsChangeHint    aMinHint,
                      RestyleTracker& aRestyleTracker,
                      bool            aRestyleDescendants);

  nsresult StyleChangeReflow(nsIFrame* aFrame, nsChangeHint aHint);

  
  
  
  bool RecomputePosition(nsIFrame* aFrame);

private:
  nsPresContext* mPresContext; 

  bool mRebuildAllStyleData : 1;
  
  bool mObservingRefreshDriver : 1;
  
  bool mInStyleRefresh : 1;
  
  
  bool mPromoteReflowsToReframeRoot : 1;
  uint32_t mHoverGeneration;
  nsChangeHint mRebuildAllExtraHint;

  OverflowChangedTracker mOverflowChangedTracker;

  
  
  uint64_t mAnimationGeneration;

  RestyleTracker mPendingRestyles;
  RestyleTracker mPendingAnimationRestyles;
};

} 

#endif 

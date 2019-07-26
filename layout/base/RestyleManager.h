









#ifndef mozilla_RestyleManager_h
#define mozilla_RestyleManager_h

#include "nsISupportsImpl.h"
#include "nsChangeHint.h"
#include "RestyleTracker.h"
#include "nsPresContext.h"

class nsRefreshDriver;
class nsIFrame;
struct TreeMatchContext;

namespace mozilla {

namespace dom {
  class Element;
} 

class RestyleManager MOZ_FINAL {
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

  







  NS_HIDDEN_(nsresult) ReparentStyleContext(nsIFrame* aFrame);

  




  NS_HIDDEN_(void)
    ComputeStyleChangeFor(nsIFrame* aFrame,
                          nsStyleChangeList* aChangeList,
                          nsChangeHint aMinChange,
                          RestyleTracker& aRestyleTracker,
                          bool aRestyleDescendants);

#ifdef DEBUG
  


  NS_HIDDEN_(void) DebugVerifyStyleTree(nsIFrame* aFrame);
#endif

  
  
  
  
  
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

  
  
  
  void BeginProcessingRestyles();
  void EndProcessingRestyles();

  
  
  
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

private:
  
  
  void RestyleElement(Element*        aElement,
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
  uint32_t mHoverGeneration;
  nsChangeHint mRebuildAllExtraHint;

  OverflowChangedTracker mOverflowChangedTracker;

  
  
  uint64_t mAnimationGeneration;

  RestyleTracker mPendingRestyles;
  RestyleTracker mPendingAnimationRestyles;
};





class ElementRestyler MOZ_FINAL {
public:
  typedef mozilla::dom::Element Element;

  
  ElementRestyler(nsPresContext* aPresContext,
                  nsIFrame* aFrame,
                  nsStyleChangeList* aChangeList,
                  nsChangeHint aHintsHandledByAncestors,
                  RestyleTracker& aRestyleTracker,
                  TreeMatchContext& aTreeMatchContext,
                  nsTArray<nsIContent*>& aVisibleKidsOfHiddenElement);

  
  enum ConstructorFlags {
    FOR_OUT_OF_FLOW_CHILD = 1<<0
  };
  ElementRestyler(const ElementRestyler& aParentRestyler,
                  nsIFrame* aFrame,
                  uint32_t aConstructorFlags);

  
  
  
  
  
  
  enum ParentContextFromChildFrame { PARENT_CONTEXT_FROM_CHILD_FRAME };
  ElementRestyler(ParentContextFromChildFrame,
                  const ElementRestyler& aParentFrameRestyler,
                  nsIFrame* aFrame);

  








  void Restyle(nsRestyleHint aRestyleHint);

  






  nsChangeHint HintsHandledForFrame() { return mHintsHandled; }

private:
  


  void RestyleSelf(nsIFrame* aSelf, nsRestyleHint aRestyleHint);

  




  void RestyleChildren(nsRestyleHint aChildRestyleHint);

  


  void CaptureChange(nsStyleContext* aOldContext,
                     nsStyleContext* aNewContext,
                     nsIFrame* aContinuation, 
                     nsChangeHint aChangeToAssume);

  


  void RestyleUndisplayedChildren(nsRestyleHint aChildRestyleHint);
  void RestyleBeforePseudo();
  void RestyleAfterPseudo(nsIFrame* aFrame);
  void RestyleContentChildren(nsIFrame* aParent,
                              nsRestyleHint aChildRestyleHint);
  void InitializeAccessibilityNotifications();
  void SendAccessibilityNotifications();

  enum DesiredA11yNotifications {
    eSkipNotifications,
    eSendAllNotifications,
    eNotifyIfShown
  };

  enum A11yNotificationType {
    eDontNotify,
    eNotifyShown,
    eNotifyHidden
  };

private:
  nsPresContext* const mPresContext;
  nsIFrame* const mFrame;
  nsIContent* const mParentContent;
  
  
  
  nsIContent* const mContent;
  nsStyleChangeList* const mChangeList;
  
  
  
  
  
  nsChangeHint mHintsHandled;
  
  nsChangeHint mParentFrameHintsNotHandledForDescendants;
  nsChangeHint mHintsNotHandledForDescendants;
  RestyleTracker& mRestyleTracker;
  TreeMatchContext& mTreeMatchContext;
  nsIFrame* mResolvedChild; 

#ifdef ACCESSIBILITY
  const DesiredA11yNotifications mDesiredA11yNotifications;
  DesiredA11yNotifications mKidsDesiredA11yNotifications;
  A11yNotificationType mOurA11yNotification;
  nsTArray<nsIContent*>& mVisibleKidsOfHiddenElement;
  bool mWasFrameVisible;
#endif
};

} 

#endif 

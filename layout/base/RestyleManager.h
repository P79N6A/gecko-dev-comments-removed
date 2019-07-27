









#ifndef mozilla_RestyleManager_h
#define mozilla_RestyleManager_h

#include "nsISupportsImpl.h"
#include "nsChangeHint.h"
#include "RestyleTracker.h"
#include "nsPresContext.h"
#include "nsRefreshDriver.h"
#include "nsRefPtrHashtable.h"
#include "nsCSSPseudoElements.h"

class nsIFrame;
class nsStyleChangeList;
struct TreeMatchContext;

namespace mozilla {
  class EventStates;

namespace dom {
  class Element;
} 

class RestyleManager MOZ_FINAL {
public:
  friend class ::nsRefreshDriver;
  friend class RestyleTracker;

  typedef mozilla::dom::Element Element;

  explicit RestyleManager(nsPresContext* aPresContext);

private:
  
  ~RestyleManager()
  {
    MOZ_ASSERT(!mReframingStyleContexts,
               "temporary member should be nulled out before destruction");
  }

public:
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
                               EventStates aStateMask);

  
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

  







  nsresult ReparentStyleContext(nsIFrame* aFrame);

private:
  void ComputeAndProcessStyleChange(nsIFrame* aFrame,
                                    nsChangeHint aMinChange,
                                    RestyleTracker& aRestyleTracker,
                                    nsRestyleHint aRestyleHint);

  




  void
    ComputeStyleChangeFor(nsIFrame* aFrame,
                          nsStyleChangeList* aChangeList,
                          nsChangeHint aMinChange,
                          RestyleTracker& aRestyleTracker,
                          nsRestyleHint aRestyleHint);

public:

#ifdef DEBUG
  


  void DebugVerifyStyleTree(nsIFrame* aFrame);
#endif

  
  
  
  
  
  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray);

  










  typedef nsRefPtrHashtable<nsRefPtrHashKey<nsIContent>, nsStyleContext>
            ReframingStyleContextTable;
  class ReframingStyleContexts {
  public:
    void Put(nsIContent* aContent, nsStyleContext* aStyleContext) {
      MOZ_ASSERT(aContent);
      nsCSSPseudoElements::Type pseudoType = aStyleContext->GetPseudoType();
      if (pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
        mElementContexts.Put(aContent, aStyleContext);
      } else if (pseudoType == nsCSSPseudoElements::ePseudo_before) {
        MOZ_ASSERT(aContent->Tag() == nsGkAtoms::mozgeneratedcontentbefore);
        mBeforePseudoContexts.Put(aContent->GetParent(), aStyleContext);
      } else if (pseudoType == nsCSSPseudoElements::ePseudo_after) {
        MOZ_ASSERT(aContent->Tag() == nsGkAtoms::mozgeneratedcontentafter);
        mAfterPseudoContexts.Put(aContent->GetParent(), aStyleContext);
      }
    }

    nsStyleContext* Get(nsIContent* aContent,
                        nsCSSPseudoElements::Type aPseudoType) {
      MOZ_ASSERT(aContent);
      if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
        return mElementContexts.GetWeak(aContent);
      }
      if (aPseudoType == nsCSSPseudoElements::ePseudo_before) {
        MOZ_ASSERT(aContent->Tag() == nsGkAtoms::mozgeneratedcontentbefore);
        return mBeforePseudoContexts.GetWeak(aContent->GetParent());
      }
      if (aPseudoType == nsCSSPseudoElements::ePseudo_after) {
        MOZ_ASSERT(aContent->Tag() == nsGkAtoms::mozgeneratedcontentafter);
        return mAfterPseudoContexts.GetWeak(aContent->GetParent());
      }
      MOZ_ASSERT(false, "unexpected aPseudoType");
      return nullptr;
    }
  private:
    ReframingStyleContextTable mElementContexts;
    ReframingStyleContextTable mBeforePseudoContexts;
    ReframingStyleContextTable mAfterPseudoContexts;
  };

  



  ReframingStyleContexts* GetReframingStyleContexts() {
    return mReframingStyleContexts;
  }

  







  static void
  TryStartingTransition(nsPresContext* aPresContext, nsIContent* aContent,
                        nsStyleContext* aOldStyleContext,
                        nsRefPtr<nsStyleContext>* aNewStyleContext );

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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void UpdateOnlyAnimationStyles();

  bool ThrottledAnimationStyleIsUpToDate() const {
    return mLastUpdateForThrottledAnimations ==
             mPresContext->RefreshDriver()->MostRecentRefresh();
  }

  
  
  
  void RebuildAllStyleData(nsChangeHint aExtraHint);

  
  
  void DoRebuildAllStyleData(RestyleTracker& aRestyleTracker,
                             nsChangeHint aExtraHint,
                             nsRestyleHint aRestyleHint);

  
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
                      nsRestyleHint   aRestyleHint);

  void StyleChangeReflow(nsIFrame* aFrame, nsChangeHint aHint);

  
  void AddSubtreeToOverflowTracker(nsIFrame* aFrame);

  
  
  
  bool RecomputePosition(nsIFrame* aFrame);

private:
  nsPresContext* mPresContext; 

  bool mRebuildAllStyleData : 1;
  
  bool mObservingRefreshDriver : 1;
  
  bool mInStyleRefresh : 1;
  uint32_t mHoverGeneration;
  nsChangeHint mRebuildAllExtraHint;

  mozilla::TimeStamp mLastUpdateForThrottledAnimations;

  OverflowChangedTracker mOverflowChangedTracker;

  
  
  uint64_t mAnimationGeneration;

  ReframingStyleContexts* mReframingStyleContexts;

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
  
  
  
  
  
  enum RestyleResult {

    
    eRestyleResult_Stop = 1,

    
    eRestyleResult_Continue,

    
    eRestyleResult_ContinueAndForceDescendants
  };

  


  RestyleResult RestyleSelf(nsIFrame* aSelf, nsRestyleHint aRestyleHint);

  




  void RestyleChildren(nsRestyleHint aChildRestyleHint);

  


  void CaptureChange(nsStyleContext* aOldContext,
                     nsStyleContext* aNewContext,
                     nsChangeHint aChangeToAssume,
                     uint32_t* aEqualStructs);

  


  void RestyleUndisplayedChildren(nsRestyleHint aChildRestyleHint);
  void MaybeReframeForBeforePseudo();
  void MaybeReframeForAfterPseudo(nsIFrame* aFrame);
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

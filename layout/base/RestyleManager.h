









#ifndef mozilla_RestyleManager_h
#define mozilla_RestyleManager_h

#include "mozilla/RestyleLogging.h"
#include "nsISupportsImpl.h"
#include "nsChangeHint.h"
#include "RestyleTracker.h"
#include "nsPresContext.h"
#include "nsRefreshDriver.h"
#include "nsRefPtrHashtable.h"
#include "nsCSSPseudoElements.h"
#include "nsTransitionManager.h"

class nsIFrame;
class nsStyleChangeList;
struct TreeMatchContext;

namespace mozilla {
  class EventStates;
  struct UndisplayedNode;

namespace dom {
  class Element;
} 

class RestyleManager MOZ_FINAL
{
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

  
  
  
  static uint64_t GetMaxAnimationGenerationForFrame(nsIFrame* aFrame);

  
  
  
  
  
  
  void IncrementAnimationGeneration() { ++mAnimationGeneration; }

  
  bool SkipAnimationRules() const {
    MOZ_ASSERT(mSkipAnimationRules || !mPostAnimationRestyles,
               "inconsistent state");
    return mSkipAnimationRules;
  }

  void SetSkipAnimationRules(bool aSkipAnimationRules) {
    mSkipAnimationRules = aSkipAnimationRules;
  }

  
  
  
  bool PostAnimationRestyles() const {
    MOZ_ASSERT(mSkipAnimationRules || !mPostAnimationRestyles,
               "inconsistent state");
    return mPostAnimationRestyles;
  }

  void SetPostAnimationRestyles(bool aPostAnimationRestyles) {
    mPostAnimationRestyles = aPostAnimationRestyles;
  }

  
  
  bool IsProcessingAnimationStyleChange() const {
    return mIsProcessingAnimationStyleChange;
  }

  







  nsresult ReparentStyleContext(nsIFrame* aFrame);

private:
  
  void ComputeAndProcessStyleChange(nsIFrame*       aFrame,
                                    nsChangeHint    aMinChange,
                                    RestyleTracker& aRestyleTracker,
                                    nsRestyleHint   aRestyleHint);
  
  void ComputeAndProcessStyleChange(nsStyleContext* aNewContext,
                                    Element*        aElement,
                                    nsChangeHint    aMinChange,
                                    RestyleTracker& aRestyleTracker,
                                    nsRestyleHint   aRestyleHint);

public:

#ifdef DEBUG
  


  void DebugVerifyStyleTree(nsIFrame* aFrame);
#endif

  
  
  
  
  
  nsresult ProcessRestyledFrames(nsStyleChangeList& aRestyleArray);

  










  typedef nsRefPtrHashtable<nsRefPtrHashKey<nsIContent>, nsStyleContext>
            ReframingStyleContextTable;
  class MOZ_STACK_CLASS ReframingStyleContexts MOZ_FINAL {
  public:
    





    explicit ReframingStyleContexts(RestyleManager* aRestyleManager);
    ~ReframingStyleContexts();

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
    RestyleManager* mRestyleManager;
    AutoRestore<ReframingStyleContexts*> mRestorePointer;
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

  
  bool HasPendingRestyles() { return mPendingRestyles.Count() != 0; }

  
  
  
  void BeginProcessingRestyles(RestyleTracker& aRestyleTracker);
  void EndProcessingRestyles();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void UpdateOnlyAnimationStyles();

  bool ThrottledAnimationStyleIsUpToDate() const {
    return mLastUpdateForThrottledAnimations ==
             mPresContext->RefreshDriver()->MostRecentRefresh();
  }

  
  
  
  
  
  
  
  
  
  
  
  
  void RebuildAllStyleData(nsChangeHint aExtraHint,
                           nsRestyleHint aRestyleHint);

  
  void PostRestyleEvent(Element* aElement,
                        nsRestyleHint aRestyleHint,
                        nsChangeHint aMinChangeHint)
  {
    if (mPresContext) {
      PostRestyleEventCommon(aElement, aRestyleHint, aMinChangeHint,
                             IsProcessingAnimationStyleChange());
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

#ifdef DEBUG
  static nsCString RestyleHintToString(nsRestyleHint aHint);
  static nsCString ChangeHintToString(nsChangeHint aHint);
#endif

private:
  














  void PostRestyleEventCommon(Element* aElement,
                              nsRestyleHint aRestyleHint,
                              nsChangeHint aMinChangeHint,
                              bool aForAnimation);
  void PostRestyleEventInternal(bool aForLazyConstruction);

public:
  











  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint,
                                    nsRestyleHint aRestyleHint);

#ifdef RESTYLE_LOGGING
  



  bool ShouldLogRestyle() {
    return ShouldLogRestyle(mPresContext);
  }

  



  static bool ShouldLogRestyle(nsPresContext* aPresContext) {
    return aPresContext->RestyleLoggingEnabled() &&
           (!aPresContext->TransitionManager()->
               InAnimationOnlyStyleUpdate() ||
            AnimationRestyleLoggingEnabled());
  }

  static bool RestyleLoggingInitiallyEnabled() {
    static bool enabled = getenv("MOZ_DEBUG_RESTYLE") != 0;
    return enabled;
  }

  static bool AnimationRestyleLoggingEnabled() {
    static bool animations = getenv("MOZ_DEBUG_RESTYLE_ANIMATIONS") != 0;
    return animations;
  }

  
  
  
  
  
  static uint32_t StructsToLog();

  static nsCString StructNamesToString(uint32_t aSIDs);
  int32_t& LoggingDepth() { return mLoggingDepth; }
#endif

private:
  
  
  void RestyleElement(Element*        aElement,
                      nsIFrame*       aPrimaryFrame,
                      nsChangeHint    aMinHint,
                      RestyleTracker& aRestyleTracker,
                      nsRestyleHint   aRestyleHint);

  void StartRebuildAllStyleData(RestyleTracker& aRestyleTracker);
  void FinishRebuildAllStyleData();

  void StyleChangeReflow(nsIFrame* aFrame, nsChangeHint aHint);

  
  void AddSubtreeToOverflowTracker(nsIFrame* aFrame);

  
  
  
  bool RecomputePosition(nsIFrame* aFrame);

  bool ShouldStartRebuildAllFor(RestyleTracker& aRestyleTracker) {
    
    
    return mDoRebuildAllStyleData &&
           &aRestyleTracker == &mPendingRestyles;
  }

  void ProcessRestyles(RestyleTracker& aRestyleTracker) {
    
    
    if (aRestyleTracker.Count() || ShouldStartRebuildAllFor(aRestyleTracker)) {
      aRestyleTracker.DoProcessRestyles();
    }
  }

private:
  nsPresContext* mPresContext; 

  
  
  bool mDoRebuildAllStyleData : 1;
  
  bool mInRebuildAllStyleData : 1;
  
  bool mObservingRefreshDriver : 1;
  
  bool mInStyleRefresh : 1;
  
  bool mSkipAnimationRules : 1;
  
  
  
  bool mPostAnimationRestyles : 1;
  
  
  bool mIsProcessingAnimationStyleChange : 1;
  bool mHavePendingNonAnimationRestyles : 1;

  uint32_t mHoverGeneration;
  nsChangeHint mRebuildAllExtraHint;
  nsRestyleHint mRebuildAllRestyleHint;

  mozilla::TimeStamp mLastUpdateForThrottledAnimations;

  OverflowChangedTracker mOverflowChangedTracker;

  
  
  uint64_t mAnimationGeneration;

  ReframingStyleContexts* mReframingStyleContexts;

  RestyleTracker mPendingRestyles;
  RestyleTracker mPendingAnimationRestyles;

#ifdef DEBUG
  bool mIsProcessingRestyles;
#endif

#ifdef RESTYLE_LOGGING
  int32_t mLoggingDepth;
#endif
};





class ElementRestyler MOZ_FINAL
{
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

  
  ElementRestyler(nsPresContext* aPresContext,
                  nsIContent* aContent,
                  nsStyleChangeList* aChangeList,
                  nsChangeHint aHintsHandledByAncestors,
                  RestyleTracker& aRestyleTracker,
                  TreeMatchContext& aTreeMatchContext,
                  nsTArray<nsIContent*>& aVisibleKidsOfHiddenElement);

  








  void Restyle(nsRestyleHint aRestyleHint);

  






  nsChangeHint HintsHandledForFrame() { return mHintsHandled; }

  



  void RestyleChildrenOfDisplayContentsElement(nsIFrame*       aParentFrame,
                                               nsStyleContext* aNewContext,
                                               nsChangeHint    aMinHint,
                                               RestyleTracker& aRestyleTracker,
                                               nsRestyleHint   aRestyleHint);

  



  static void ComputeStyleChangeFor(nsIFrame*          aFrame,
                                    nsStyleChangeList* aChangeList,
                                    nsChangeHint       aMinChange,
                                    RestyleTracker&    aRestyleTracker,
                                    nsRestyleHint      aRestyleHint);

#ifdef RESTYLE_LOGGING
  bool ShouldLogRestyle() {
    return RestyleManager::ShouldLogRestyle(mPresContext);
  }
#endif

private:
  
  
  
  
  
  enum RestyleResult {

    
    eRestyleResult_Stop = 1,

    
    eRestyleResult_Continue,

    
    eRestyleResult_ContinueAndForceDescendants
  };

  


  RestyleResult RestyleSelf(nsIFrame* aSelf,
                            nsRestyleHint aRestyleHint,
                            uint32_t* aSwappedStructs);

  




  void RestyleChildren(nsRestyleHint aChildRestyleHint);

  


  void AddLayerChangesForAnimation();

  


  void CaptureChange(nsStyleContext* aOldContext,
                     nsStyleContext* aNewContext,
                     nsChangeHint aChangeToAssume,
                     uint32_t* aEqualStructs);
  RestyleResult ComputeRestyleResultFromFrame(nsIFrame* aSelf);
  RestyleResult ComputeRestyleResultFromNewContext(nsIFrame* aSelf,
                                                   nsStyleContext* aNewContext);

  


  void RestyleUndisplayedDescendants(nsRestyleHint aChildRestyleHint);
  




  void DoRestyleUndisplayedDescendants(nsRestyleHint aChildRestyleHint,
                                       nsIContent* aParent,
                                       nsStyleContext* aParentStyleContext);
  void RestyleUndisplayedNodes(nsRestyleHint    aChildRestyleHint,
                               UndisplayedNode* aUndisplayed,
                               nsIContent*      aUndisplayedParent,
                               nsStyleContext*  aParentStyleContext,
                               const uint8_t    aDisplay);
  void MaybeReframeForBeforePseudo();
  void MaybeReframeForBeforePseudo(nsIFrame* aGenConParentFrame,
                                   nsIFrame* aFrame,
                                   nsIContent* aContent,
                                   nsStyleContext* aStyleContext);
  void MaybeReframeForAfterPseudo(nsIFrame* aFrame);
  void MaybeReframeForAfterPseudo(nsIFrame* aGenConParentFrame,
                                  nsIFrame* aFrame,
                                  nsIContent* aContent,
                                  nsStyleContext* aStyleContext);
  void RestyleContentChildren(nsIFrame* aParent,
                              nsRestyleHint aChildRestyleHint);
  void InitializeAccessibilityNotifications(nsStyleContext* aNewContext);
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

#ifdef RESTYLE_LOGGING
  int32_t& LoggingDepth() { return mLoggingDepth; }
#endif

#ifdef DEBUG
  static nsCString RestyleResultToString(RestyleResult aRestyleResult);
#endif

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

#ifdef RESTYLE_LOGGING
  int32_t mLoggingDepth;
#endif
};







class MOZ_STACK_CLASS AutoDisplayContentsAncestorPusher MOZ_FINAL
{
 public:
  typedef mozilla::dom::Element Element;
  AutoDisplayContentsAncestorPusher(TreeMatchContext& aTreeMatchContext,
                                    nsPresContext*    aPresContext,
                                    nsIContent*       aParent);
  ~AutoDisplayContentsAncestorPusher();
  bool IsEmpty() const { return mAncestors.Length() == 0; }
private:
  TreeMatchContext& mTreeMatchContext;
  nsPresContext* const mPresContext;
  nsAutoTArray<mozilla::dom::Element*, 4> mAncestors;
};

} 

#endif

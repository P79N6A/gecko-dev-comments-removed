



#ifndef nsAnimationManager_h_
#define nsAnimationManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/ContentEvents.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"

class nsCSSKeyframesRule;
class nsStyleContext;

namespace mozilla {
namespace css {
class Declaration;
}
}

struct AnimationEventInfo {
  nsRefPtr<mozilla::dom::Element> mElement;
  mozilla::InternalAnimationEvent mEvent;

  AnimationEventInfo(mozilla::dom::Element *aElement,
                     const nsString& aAnimationName,
                     uint32_t aMessage, mozilla::TimeDuration aElapsedTime,
                     const nsAString& aPseudoElement)
    : mElement(aElement), mEvent(true, aMessage)
  {
    
    mEvent.animationName = aAnimationName;
    mEvent.elapsedTime = aElapsedTime.ToSeconds();
    mEvent.pseudoElement = aPseudoElement;
  }

  
  
  AnimationEventInfo(const AnimationEventInfo &aOther)
    : mElement(aOther.mElement), mEvent(true, aOther.mEvent.message)
  {
    mEvent.AssignAnimationEventData(aOther.mEvent, false);
  }
};

typedef InfallibleTArray<AnimationEventInfo> EventArray;




struct ElementAnimations MOZ_FINAL
  : public mozilla::css::CommonElementAnimationData
{
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  ElementAnimations(mozilla::dom::Element *aElement, nsIAtom *aElementProperty,
                    nsAnimationManager *aAnimationManager, TimeStamp aNow);

  
  
  
  
  
  
  
  
  
  
  static double GetPositionInIteration(TimeDuration aElapsedDuration,
                                       TimeDuration aIterationDuration,
                                       double aIterationCount,
                                       uint32_t aDirection,
                                       uint8_t aFillMode,
                                       mozilla::ElementAnimation* aAnimation =
                                         nullptr,
                                       ElementAnimations* aEa = nullptr,
                                       EventArray* aEventsToDispatch = nullptr);

  void EnsureStyleRuleFor(TimeStamp aRefreshTime, bool aIsThrottled);
  void GetEventsAt(TimeStamp aRefreshTime, EventArray &aEventsToDispatch);

  bool IsForElement() const { 
    return mElementProperty == nsGkAtoms::animationsProperty;
  }

  nsString PseudoElement()
  {
    return mElementProperty == nsGkAtoms::animationsProperty ?
             EmptyString() :
             mElementProperty == nsGkAtoms::animationsOfBeforeProperty ?
               NS_LITERAL_STRING("::before") :
               NS_LITERAL_STRING("::after");
  }

  void PostRestyleForAnimation(nsPresContext *aPresContext) {
    nsRestyleHint styleHint = IsForElement() ? eRestyle_Self : eRestyle_Subtree;
    aPresContext->PresShell()->RestyleForAnimation(mElement, styleHint);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool CanPerformOnCompositorThread(CanAnimateFlags aFlags) const MOZ_OVERRIDE;

  virtual bool HasAnimationOfProperty(nsCSSProperty aProperty) const MOZ_OVERRIDE;

  
  
  
  bool mNeedsRefreshes;
};

class nsAnimationManager MOZ_FINAL
  : public mozilla::css::CommonAnimationManager
{
public:
  nsAnimationManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
    , mObservingRefreshDriver(false)
  {
  }

  static ElementAnimations* GetAnimationsForCompositor(nsIContent* aContent,
                                                       nsCSSProperty aProperty)
  {
    if (!aContent->MayHaveAnimations())
      return nullptr;
    ElementAnimations* animations = static_cast<ElementAnimations*>(
      aContent->GetProperty(nsGkAtoms::animationsProperty));
    if (!animations)
      return nullptr;
    bool propertyMatches = animations->HasAnimationOfProperty(aProperty);
    return (propertyMatches &&
            animations->CanPerformOnCompositorThread(
              mozilla::css::CommonElementAnimationData::CanAnimate_AllowPartial))
           ? animations
           : nullptr;
  }

  
  static bool ContentOrAncestorHasAnimation(nsIContent* aContent) {
    do {
      if (aContent->GetProperty(nsGkAtoms::animationsProperty)) {
        return true;
      }
    } while ((aContent = aContent->GetParent()));

    return false;
  }

  void EnsureStyleRuleFor(ElementAnimations* aET);

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime) MOZ_OVERRIDE;

  void FlushAnimations(FlushFlags aFlags);

  










  nsIStyleRule* CheckAnimationRule(nsStyleContext* aStyleContext,
                                   mozilla::dom::Element* aElement);

  






  void DispatchEvents() {
    
    if (!mPendingEvents.IsEmpty()) {
      DoDispatchEvents();
    }
  }

  ElementAnimations* GetElementAnimations(mozilla::dom::Element *aElement,
                                          nsCSSPseudoElements::Type aPseudoType,
                                          bool aCreateIfNeeded);

  
  void UpdateAllThrottledStyles();

protected:
  virtual void ElementDataRemoved() MOZ_OVERRIDE
  {
    CheckNeedsRefresh();
  }
  virtual void AddElementData(mozilla::css::CommonElementAnimationData* aData) MOZ_OVERRIDE;

  


  void CheckNeedsRefresh();

private:
  void BuildAnimations(nsStyleContext* aStyleContext,
                       mozilla::ElementAnimationPtrArray& aAnimations);
  bool BuildSegment(InfallibleTArray<mozilla::AnimationPropertySegment>&
                      aSegments,
                    nsCSSProperty aProperty, const nsAnimation& aAnimation,
                    float aFromKey, nsStyleContext* aFromContext,
                    mozilla::css::Declaration* aFromDeclaration,
                    float aToKey, nsStyleContext* aToContext);
  nsIStyleRule* GetAnimationRule(mozilla::dom::Element* aElement,
                                 nsCSSPseudoElements::Type aPseudoType);

  
  
  
  void UpdateThrottledStylesForSubtree(nsIContent* aContent,
                                       nsStyleContext* aParentStyle,
                                       nsStyleChangeList &aChangeList);
  void UpdateAllThrottledStylesInternal();

  
  void DoDispatchEvents();

  EventArray mPendingEvents;

  bool mObservingRefreshDriver;
};

#endif 

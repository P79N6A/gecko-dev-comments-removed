




































#ifndef nsAnimationManager_h_
#define nsAnimationManager_h_

#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"
#include "nsStyleContext.h"
#include "nsTHashtable.h"
#include "nsGUIEvent.h"
#include "mozilla/TimeStamp.h"
#include "nsThreadUtils.h"

class nsCSSKeyframesRule;
struct AnimationSegment;
struct ElementAnimation;
struct ElementAnimations;

namespace mozilla {
namespace css {
class Declaration;
}
}

class nsAnimationManager : public mozilla::css::CommonAnimationManager
{
public:
  nsAnimationManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext),
      mKeyframesListIsDirty(true)
  {
    mKeyframesRules.Init(16); 
  }

  struct AnimationEventInfo {
    nsRefPtr<mozilla::dom::Element> mElement;
    nsAnimationEvent mEvent;

    AnimationEventInfo(mozilla::dom::Element *aElement,
                       const nsString& aAnimationName,
                       PRUint32 aMessage, mozilla::TimeDuration aElapsedTime)
      : mElement(aElement),
        mEvent(PR_TRUE, aMessage, aAnimationName, aElapsedTime.ToSeconds())
    {
    }

    
    
    AnimationEventInfo(const AnimationEventInfo &aOther)
      : mElement(aOther.mElement),
        mEvent(PR_TRUE, aOther.mEvent.message,
               aOther.mEvent.animationName, aOther.mEvent.elapsedTime)
    {
    }
  };

  
  virtual void RulesMatching(ElementRuleProcessorData* aData);
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData);
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData);
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData);
#endif

  
  virtual void WillRefresh(mozilla::TimeStamp aTime);

  










  nsIStyleRule* CheckAnimationRule(nsStyleContext* aStyleContext,
                                   mozilla::dom::Element* aElement);

  void KeyframesListIsDirty() {
    mKeyframesListIsDirty = PR_TRUE;
  }

  typedef InfallibleTArray<AnimationEventInfo> EventArray;

  






  void DispatchEvents();

private:
  ElementAnimations* GetElementAnimations(mozilla::dom::Element *aElement,
                                          nsCSSPseudoElements::Type aPseudoType,
                                          PRBool aCreateIfNeeded);
  void BuildAnimations(nsStyleContext* aStyleContext,
                       InfallibleTArray<ElementAnimation>& aAnimations);
  void BuildSegment(InfallibleTArray<AnimationSegment>& aSegments,
                    const nsAnimation& aAnimation,
                    float aFromKey, nsStyleContext* aFromContext,
                    mozilla::css::Declaration* aFromDeclaration,
                    float aToKey, nsStyleContext* aToContext,
                    mozilla::css::Declaration* aToDeclaration);
  nsIStyleRule* GetAnimationRule(mozilla::dom::Element* aElement,
                                 nsCSSPseudoElements::Type aPseudoType);

  nsCSSKeyframesRule* KeyframesRuleFor(const nsSubstring& aName);

  bool mKeyframesListIsDirty;
  nsDataHashtable<nsStringHashKey, nsCSSKeyframesRule*> mKeyframesRules;

  EventArray mPendingEvents;
};

#endif 






#ifndef mozilla_css_AnimationCommon_h
#define mozilla_css_AnimationCommon_h

#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsRefreshDriver.h"
#include "prclist.h"
#include "nsCSSProperty.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/dom/AnimationPlayer.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Nullable.h"
#include "nsStyleStruct.h"
#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"
#include "nsCSSPseudoElements.h"
#include "nsCycleCollectionParticipant.h"

class nsIFrame;
class nsPresContext;
class nsStyleChangeList;

namespace mozilla {

class RestyleTracker;
struct AnimationPlayerCollection;

namespace css {

bool IsGeometricProperty(nsCSSProperty aProperty);

class CommonAnimationManager : public nsIStyleRuleProcessor,
                               public nsARefreshObserver {
public:
  CommonAnimationManager(nsPresContext *aPresContext);

  
  NS_DECL_ISUPPORTS

  
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) MOZ_OVERRIDE;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  


  void Disconnect();

  
  
  
  void AddStyleUpdatesTo(mozilla::RestyleTracker& aTracker);

  enum FlushFlags {
    Can_Throttle,
    Cannot_Throttle
  };

  static bool ExtractComputedValueForTransition(
                  nsCSSProperty aProperty,
                  nsStyleContext* aStyleContext,
                  mozilla::StyleAnimationValue& aComputedValue);
protected:
  virtual ~CommonAnimationManager();

  
  friend struct mozilla::AnimationPlayerCollection;

  virtual void
  AddElementCollection(AnimationPlayerCollection* aCollection) = 0;
  virtual void ElementCollectionRemoved() = 0;
  void RemoveAllElementCollections();

  
  
  static AnimationPlayerCollection*
  GetAnimationsForCompositor(nsIContent* aContent,
                             nsIAtom* aElementProperty,
                             nsCSSProperty aProperty);

  PRCList mElementCollections;
  nsPresContext *mPresContext; 
};




class AnimValuesStyleRule MOZ_FINAL : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) MOZ_OVERRIDE;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  void AddValue(nsCSSProperty aProperty,
                mozilla::StyleAnimationValue &aStartValue)
  {
    PropertyValuePair v = { aProperty, aStartValue };
    mPropertyValuePairs.AppendElement(v);
  }

  
  mozilla::StyleAnimationValue* AddEmptyValue(nsCSSProperty aProperty)
  {
    PropertyValuePair *p = mPropertyValuePairs.AppendElement();
    p->mProperty = aProperty;
    return &p->mValue;
  }

  struct PropertyValuePair {
    nsCSSProperty mProperty;
    mozilla::StyleAnimationValue mValue;
  };

private:
  ~AnimValuesStyleRule() {}

  InfallibleTArray<PropertyValuePair> mPropertyValuePairs;
};

} 

typedef InfallibleTArray<nsRefPtr<dom::AnimationPlayer> >
  AnimationPlayerPtrArray;

enum EnsureStyleRuleFlags {
  EnsureStyleRule_IsThrottled,
  EnsureStyleRule_IsNotThrottled
};

struct AnimationPlayerCollection : public PRCList
{
  AnimationPlayerCollection(dom::Element *aElement, nsIAtom *aElementProperty,
                            mozilla::css::CommonAnimationManager *aManager,
                            TimeStamp aNow)
    : mElement(aElement)
    , mElementProperty(aElementProperty)
    , mManager(aManager)
    , mAnimationGeneration(0)
    , mNeedsRefreshes(true)
#ifdef DEBUG
    , mCalledPropertyDtor(false)
#endif
  {
    MOZ_COUNT_CTOR(AnimationPlayerCollection);
    PR_INIT_CLIST(this);
  }
  ~AnimationPlayerCollection()
  {
    NS_ABORT_IF_FALSE(mCalledPropertyDtor,
                      "must call destructor through element property dtor");
    MOZ_COUNT_DTOR(AnimationPlayerCollection);
    PR_REMOVE_LINK(this);
    mManager->ElementCollectionRemoved();
  }

  void Destroy()
  {
    
    mElement->DeleteProperty(mElementProperty);
  }

  static void PropertyDtor(void *aObject, nsIAtom *aPropertyName,
                           void *aPropertyValue, void *aData);

  void Tick();

  
  
  
  
  void EnsureStyleRuleFor(TimeStamp aRefreshTime, EnsureStyleRuleFlags aFlags);

  bool CanThrottleTransformChanges(mozilla::TimeStamp aTime);

  bool CanThrottleAnimation(mozilla::TimeStamp aTime);

  enum CanAnimateFlags {
    
    CanAnimate_HasGeometricProperty = 1,
    
    
    CanAnimate_AllowPartial = 2
  };

  static bool
  CanAnimatePropertyOnCompositor(const dom::Element *aElement,
                                 nsCSSProperty aProperty,
                                 CanAnimateFlags aFlags);

  static bool IsCompositorAnimationDisabledForFrame(nsIFrame* aFrame);

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool CanPerformOnCompositorThread(CanAnimateFlags aFlags) const;
  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;

  bool IsForElement() const { 
    return mElementProperty == nsGkAtoms::animationsProperty ||
           mElementProperty == nsGkAtoms::transitionsProperty;
  }

  bool IsForTransitions() const {
    return mElementProperty == nsGkAtoms::transitionsProperty ||
           mElementProperty == nsGkAtoms::transitionsOfBeforeProperty ||
           mElementProperty == nsGkAtoms::transitionsOfAfterProperty;
  }

  bool IsForAnimations() const {
    return mElementProperty == nsGkAtoms::animationsProperty ||
           mElementProperty == nsGkAtoms::animationsOfBeforeProperty ||
           mElementProperty == nsGkAtoms::animationsOfAfterProperty;
  }

  nsString PseudoElement()
  {
    if (IsForElement()) {
      return EmptyString();
    } else if (mElementProperty == nsGkAtoms::animationsOfBeforeProperty ||
               mElementProperty == nsGkAtoms::transitionsOfBeforeProperty) {
      return NS_LITERAL_STRING("::before");
    } else {
      return NS_LITERAL_STRING("::after");
    }
  }

  void PostRestyleForAnimation(nsPresContext *aPresContext) {
    nsRestyleHint styleHint = IsForElement() ? eRestyle_Self : eRestyle_Subtree;
    aPresContext->PresShell()->RestyleForAnimation(mElement, styleHint);
  }

  static void LogAsyncAnimationFailure(nsCString& aMessage,
                                       const nsIContent* aContent = nullptr);

  dom::Element *mElement;

  
  
  nsIAtom *mElementProperty;

  mozilla::css::CommonAnimationManager *mManager;

  mozilla::AnimationPlayerPtrArray mAnimations;

  
  
  
  
  
  
  
  nsRefPtr<mozilla::css::AnimValuesStyleRule> mStyleRule;

  
  
  
  
  
  
  uint64_t mAnimationGeneration;
  
  void UpdateAnimationGeneration(nsPresContext* aPresContext);

  
  
  bool HasCurrentAnimations();

  
  TimeStamp mStyleRuleRefreshTime;

  
  
  
  bool mNeedsRefreshes;

#ifdef DEBUG
  bool mCalledPropertyDtor;
#endif
};

}

#endif 

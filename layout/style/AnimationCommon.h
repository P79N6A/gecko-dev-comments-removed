




#ifndef mozilla_css_AnimationCommon_h
#define mozilla_css_AnimationCommon_h

#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsRefreshDriver.h"
#include "prclist.h"
#include "nsChangeHint.h"
#include "nsCSSProperty.h"
#include "nsDisplayList.h" 
#include "mozilla/MemoryReporting.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/dom/AnimationPlayer.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Nullable.h"
#include "nsStyleStruct.h"
#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "nsCSSPseudoElements.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCSSPropertySet.h"

class nsIFrame;
class nsPresContext;
class nsStyleChangeList;

namespace mozilla {

class RestyleTracker;
struct AnimationPlayerCollection;


enum class GetCompositorAnimationOptions {
  
  
  NotifyActiveLayerTracker = 1 << 0
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(GetCompositorAnimationOptions)

namespace css {

bool IsGeometricProperty(nsCSSProperty aProperty);

class CommonAnimationManager : public nsIStyleRuleProcessor,
                               public nsARefreshObserver {
public:
  explicit CommonAnimationManager(nsPresContext *aPresContext);

  
  NS_DECL_ISUPPORTS

  
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) override;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) override;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) override;
  virtual void RulesMatching(ElementRuleProcessorData* aData) override;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) override;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) override;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) override;
#endif
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;

#ifdef DEBUG
  static void Initialize();
#endif

  
  nsPresContext* PresContext() const { return mPresContext; }

  


  void Disconnect();

  
  
  
  void AddStyleUpdatesTo(mozilla::RestyleTracker& aTracker);

  AnimationPlayerCollection*
  GetAnimations(dom::Element *aElement,
                nsCSSPseudoElements::Type aPseudoType,
                bool aCreateIfNeeded);

  
  
  static bool ContentOrAncestorHasAnimation(nsIContent* aContent) {
    do {
      if (aContent->GetProperty(nsGkAtoms::animationsProperty) ||
          aContent->GetProperty(nsGkAtoms::transitionsProperty)) {
        return true;
      }
    } while ((aContent = aContent->GetParent()));

    return false;
  }

  
  
  void NotifyCollectionUpdated(AnimationPlayerCollection& aCollection);

  enum FlushFlags {
    Can_Throttle,
    Cannot_Throttle
  };

  nsIStyleRule* GetAnimationRule(mozilla::dom::Element* aElement,
                                 nsCSSPseudoElements::Type aPseudoType);

  static bool ExtractComputedValueForTransition(
                  nsCSSProperty aProperty,
                  nsStyleContext* aStyleContext,
                  mozilla::StyleAnimationValue& aComputedValue);

  
  
  struct LayerAnimationRecord {
    nsCSSProperty mProperty;
    nsDisplayItem::Type mLayerType;
    nsChangeHint mChangeHint;
  };

protected:
  static const size_t kLayerRecords = 2;

public:
  static const LayerAnimationRecord sLayerAnimationInfo[kLayerRecords];

  
  
  
  static const LayerAnimationRecord*
    LayerAnimationRecordFor(nsCSSProperty aProperty);

protected:
  virtual ~CommonAnimationManager();

  
  friend struct mozilla::AnimationPlayerCollection;

  void AddElementCollection(AnimationPlayerCollection* aCollection);
  void ElementCollectionRemoved() { MaybeStartOrStopObservingRefreshDriver(); }
  void RemoveAllElementCollections();

  
  
  
  void MaybeStartObservingRefreshDriver();
  void MaybeStartOrStopObservingRefreshDriver();
  bool NeedsRefresh() const;

  virtual nsIAtom* GetAnimationsAtom() = 0;
  virtual nsIAtom* GetAnimationsBeforeAtom() = 0;
  virtual nsIAtom* GetAnimationsAfterAtom() = 0;

  virtual bool IsAnimationManager() {
    return false;
  }

  static AnimationPlayerCollection*
  GetAnimationsForCompositor(nsIContent* aContent,
                             nsIAtom* aElementProperty,
                             nsCSSProperty aProperty,
                             GetCompositorAnimationOptions aFlags);

  PRCList mElementCollections;
  nsPresContext *mPresContext; 
  bool mIsObservingRefreshDriver;
};




class AnimValuesStyleRule final : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
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

  void AddPropertiesToSet(nsCSSPropertySet& aSet) const
  {
    for (size_t i = 0, i_end = mPropertyValuePairs.Length(); i < i_end; ++i) {
      const PropertyValuePair &cv = mPropertyValuePairs[i];
      aSet.AddProperty(cv.mProperty);
    }
  }

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
                            mozilla::css::CommonAnimationManager *aManager)
    : mElement(aElement)
    , mElementProperty(aElementProperty)
    , mManager(aManager)
    , mAnimationGeneration(0)
    , mCheckGeneration(0)
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
    MOZ_ASSERT(mCalledPropertyDtor,
               "must call destructor through element property dtor");
    MOZ_COUNT_DTOR(AnimationPlayerCollection);
    PR_REMOVE_LINK(this);
    mManager->ElementCollectionRemoved();
  }

  void Destroy()
  {
    for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
      mPlayers[playerIdx]->Cancel();
    }
    
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

  void PostUpdateLayerAnimations();

  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;

  bool IsForElement() const { 
    return mElementProperty == nsGkAtoms::animationsProperty ||
           mElementProperty == nsGkAtoms::transitionsProperty;
  }

  bool IsForBeforePseudo() const {
    return mElementProperty == nsGkAtoms::animationsOfBeforeProperty ||
           mElementProperty == nsGkAtoms::transitionsOfBeforeProperty;
  }

  bool IsForAfterPseudo() const {
    return mElementProperty == nsGkAtoms::animationsOfAfterProperty ||
           mElementProperty == nsGkAtoms::transitionsOfAfterProperty;
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

  nsString PseudoElement() const
  {
    if (IsForElement()) {
      return EmptyString();
    }
    if (IsForBeforePseudo()) {
      return NS_LITERAL_STRING("::before");
    }
    MOZ_ASSERT(IsForAfterPseudo(),
               "::before & ::after should be the only pseudo-elements here");
    return NS_LITERAL_STRING("::after");
  }

  nsCSSPseudoElements::Type PseudoElementType() const
  {
    if (IsForElement()) {
      return nsCSSPseudoElements::ePseudo_NotPseudoElement;
    }
    if (IsForBeforePseudo()) {
      return nsCSSPseudoElements::ePseudo_before;
    }
    MOZ_ASSERT(IsForAfterPseudo(),
               "::before & ::after should be the only pseudo-elements here");
    return nsCSSPseudoElements::ePseudo_after;
  }

  mozilla::dom::Element* GetElementToRestyle() const;

  void PostRestyleForAnimation(nsPresContext *aPresContext) {
    mozilla::dom::Element* element = GetElementToRestyle();
    if (element) {
      nsRestyleHint hint = IsForTransitions() ? eRestyle_CSSTransitions
                                              : eRestyle_CSSAnimations;
      aPresContext->PresShell()->RestyleForAnimation(element, hint);
    }
  }

  void NotifyPlayerUpdated();

  static void LogAsyncAnimationFailure(nsCString& aMessage,
                                       const nsIContent* aContent = nullptr);

  dom::Element *mElement;

  
  
  nsIAtom *mElementProperty;

  mozilla::css::CommonAnimationManager *mManager;

  mozilla::AnimationPlayerPtrArray mPlayers;

  
  
  
  
  
  
  
  nsRefPtr<mozilla::css::AnimValuesStyleRule> mStyleRule;

  
  
  
  
  
  
  uint64_t mAnimationGeneration;
  
  void UpdateAnimationGeneration(nsPresContext* aPresContext);

  
  
  
  
  
  
  uint64_t mCheckGeneration;
  
  void UpdateCheckGeneration(nsPresContext* aPresContext);

  
  bool HasCurrentAnimations() const;
  
  
  bool HasCurrentAnimationsForProperties(const nsCSSProperty* aProperties,
                                         size_t aPropertyCount) const;

  
  TimeStamp mStyleRuleRefreshTime;

  
  
  
  bool mNeedsRefreshes;

#ifdef DEBUG
  bool mCalledPropertyDtor;
#endif
};

}

#endif 

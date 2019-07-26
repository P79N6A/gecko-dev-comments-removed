







#include "nsTransitionManager.h"
#include "nsAnimationManager.h"
#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsCSSProps.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "nsRefreshDriver.h"
#include "nsRuleProcessorData.h"
#include "nsRuleWalker.h"
#include "nsCSSPropertySet.h"
#include "nsStyleAnimation.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/dom/Element.h"
#include "nsIFrame.h"
#include "Layers.h"
#include "FrameLayerBuilder.h"
#include "nsDisplayList.h"
#include "nsStyleChangeList.h"
#include "nsStyleSet.h"
#include "RestyleManager.h"
#include "ActiveLayerTracker.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::css;

ElementTransitions::ElementTransitions(mozilla::dom::Element *aElement,
                                       nsIAtom *aElementProperty,
                                       nsTransitionManager *aTransitionManager,
                                       TimeStamp aNow)
  : CommonElementAnimationData(aElement, aElementProperty,
                               aTransitionManager, aNow)
{
}

double
ElementPropertyTransition::ValuePortionFor(TimeStamp aRefreshTime) const
{
  
  
  
  double duration = mTiming.mIterationDuration.ToSeconds();
  NS_ABORT_IF_FALSE(duration >= 0.0, "negative duration forbidden");
  double timePortion;
  if (IsRemovedSentinel()) {
    
    
    timePortion = 1.0;
  } else if (duration == 0.0) {
    
    
    if (aRefreshTime >= mStartTime + mDelay) {
      timePortion = 1.0;
    } else {
      timePortion = 0.0;
    }
  } else {
    timePortion = (aRefreshTime - (mStartTime + mDelay)).ToSeconds() / duration;
    if (timePortion < 0.0)
      timePortion = 0.0; 
    if (timePortion > 1.0)
      timePortion = 1.0; 
  }
  MOZ_ASSERT(mProperties.Length() == 1,
             "Should have one animation property for a transition");
  MOZ_ASSERT(mProperties[0].mSegments.Length() == 1,
             "Animation property should have one segment for a transition");

  return mProperties[0].mSegments[0].mTimingFunction.GetValue(timePortion);
}

static void
ElementTransitionsPropertyDtor(void           *aObject,
                               nsIAtom        *aPropertyName,
                               void           *aPropertyValue,
                               void           *aData)
{
  ElementTransitions *et = static_cast<ElementTransitions*>(aPropertyValue);
#ifdef DEBUG
  NS_ABORT_IF_FALSE(!et->mCalledPropertyDtor, "can't call dtor twice");
  et->mCalledPropertyDtor = true;
#endif
  delete et;
}

void
ElementTransitions::EnsureStyleRuleFor(TimeStamp aRefreshTime)
{
  if (!mStyleRule || mStyleRuleRefreshTime != aRefreshTime) {
    mStyleRule = new css::AnimValuesStyleRule();
    mStyleRuleRefreshTime = aRefreshTime;

    for (uint32_t i = 0, i_end = mAnimations.Length(); i < i_end; ++i)
    {
      ElementPropertyTransition* pt = mAnimations[i]->AsTransition();
      if (pt->IsRemovedSentinel()) {
        continue;
      }

      MOZ_ASSERT(pt->mProperties.Length() == 1,
                 "Should have one animation property for a transition");
      const AnimationProperty &prop = pt->mProperties[0];

      nsStyleAnimation::Value *val = mStyleRule->AddEmptyValue(prop.mProperty);

      double valuePortion = pt->ValuePortionFor(aRefreshTime);

      MOZ_ASSERT(prop.mSegments.Length() == 1,
                 "Animation property should have one segment for a transition");
#ifdef DEBUG
      bool ok =
#endif
        nsStyleAnimation::Interpolate(prop.mProperty,
                                      prop.mSegments[0].mFromValue,
                                      prop.mSegments[0].mToValue,
                                      valuePortion, *val);
      NS_ABORT_IF_FALSE(ok, "could not interpolate values");
    }
  }
}

bool
ElementTransitions::HasAnimationOfProperty(nsCSSProperty aProperty) const
{
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementPropertyTransition* pt = mAnimations[animIdx]->AsTransition();
    if (pt->HasAnimationOfProperty(aProperty) && !pt->IsRemovedSentinel()) {
      return true;
    }
  }
  return false;
}

bool
ElementTransitions::CanPerformOnCompositorThread(CanAnimateFlags aFlags) const
{
  nsIFrame* frame = nsLayoutUtils::GetStyleFrame(mElement);
  if (!frame) {
    return false;
  }

  if (mElementProperty != nsGkAtoms::transitionsProperty) {
    if (nsLayoutUtils::IsAnimationLoggingEnabled()) {
      nsCString message;
      message.AppendLiteral("Gecko bug: Async transition of pseudoelements not supported.  See bug 771367");
      LogAsyncAnimationFailure(message, mElement);
    }
    return false;
  }

  TimeStamp now = frame->PresContext()->RefreshDriver()->MostRecentRefresh();

  for (uint32_t i = 0, i_end = mAnimations.Length(); i < i_end; ++i) {
    const ElementAnimation* animation = mAnimations[i];
    MOZ_ASSERT(animation->mProperties.Length() == 1,
               "Should have one animation property for a transition");
    if (css::IsGeometricProperty(animation->mProperties[0].mProperty) &&
        animation->IsRunningAt(now)) {
      aFlags = CanAnimateFlags(aFlags | CanAnimate_HasGeometricProperty);
      break;
    }
  }

  bool hasOpacity = false;
  bool hasTransform = false;
  bool existsProperty = false;
  for (uint32_t i = 0, i_end = mAnimations.Length(); i < i_end; ++i) {
    const ElementAnimation* animation = mAnimations[i];
    if (!animation->IsRunningAt(now)) {
      continue;
    }

    existsProperty = true;

    MOZ_ASSERT(animation->mProperties.Length() == 1,
               "Should have one animation property for a transition");
    const AnimationProperty& prop = animation->mProperties[0];

    if (!css::CommonElementAnimationData::CanAnimatePropertyOnCompositor(
          mElement, prop.mProperty, aFlags) ||
        css::CommonElementAnimationData::IsCompositorAnimationDisabledForFrame(frame)) {
      return false;
    }
    if (prop.mProperty == eCSSProperty_opacity) {
      hasOpacity = true;
    } else if (prop.mProperty == eCSSProperty_transform) {
      hasTransform = true;
    }
  }
  
  
  if (!existsProperty) {
    return false;
  }

  
  
  if (hasOpacity) {
    ActiveLayerTracker::NotifyAnimated(frame, eCSSProperty_opacity);
  }
  if (hasTransform) {
    ActiveLayerTracker::NotifyAnimated(frame, eCSSProperty_transform);
  }
  return true;
}





void
nsTransitionManager::UpdateThrottledStylesForSubtree(nsIContent* aContent,
                                                     nsStyleContext* aParentStyle,
                                                     nsStyleChangeList& aChangeList)
{
  dom::Element* element;
  if (aContent->IsElement()) {
    element = aContent->AsElement();
  } else {
    element = nullptr;
  }

  nsRefPtr<nsStyleContext> newStyle;

  ElementTransitions* et;
  if (element &&
      (et = GetElementTransitions(element,
                                  nsCSSPseudoElements::ePseudo_NotPseudoElement,
                                  false))) {
    
    newStyle = UpdateThrottledStyle(element, aParentStyle, aChangeList);
    
    et->mFlushGeneration = mPresContext->RefreshDriver()->MostRecentRefresh();
  } else {
    newStyle = ReparentContent(aContent, aParentStyle);
  }

  
  if (newStyle) {
    for (nsIContent *child = aContent->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      UpdateThrottledStylesForSubtree(child, newStyle, aChangeList);
    }
  }
}

IMPL_UPDATE_ALL_THROTTLED_STYLES_INTERNAL(nsTransitionManager,
                                          GetElementTransitions)

void
nsTransitionManager::UpdateAllThrottledStyles()
{
  if (PR_CLIST_IS_EMPTY(&mElementData)) {
    
    mPresContext->TickLastUpdateThrottledTransitionStyle();
    return;
  }

  if (mPresContext->ThrottledTransitionStyleIsUpToDate()) {
    
    return;
  }

  mPresContext->TickLastUpdateThrottledTransitionStyle();
  UpdateAllThrottledStylesInternal();
}

void
nsTransitionManager::ElementDataRemoved()
{
  
  
  if (PR_CLIST_IS_EMPTY(&mElementData)) {
    mPresContext->RefreshDriver()->RemoveRefreshObserver(this, Flush_Style);
  }
}

void
nsTransitionManager::AddElementData(CommonElementAnimationData* aData)
{
  if (PR_CLIST_IS_EMPTY(&mElementData)) {
    
    nsRefreshDriver *rd = mPresContext->RefreshDriver();
    rd->AddRefreshObserver(this, Flush_Style);
  }

  PR_INSERT_BEFORE(aData, &mElementData);
}

already_AddRefed<nsIStyleRule>
nsTransitionManager::StyleContextChanged(dom::Element *aElement,
                                         nsStyleContext *aOldStyleContext,
                                         nsStyleContext *aNewStyleContext)
{
  NS_PRECONDITION(aOldStyleContext->GetPseudo() ==
                      aNewStyleContext->GetPseudo(),
                  "pseudo type mismatch");
  
  
  
  
  NS_PRECONDITION(aOldStyleContext->HasPseudoElementData() ==
                      aNewStyleContext->HasPseudoElementData(),
                  "pseudo type mismatch");

  if (!mPresContext->IsDynamic()) {
    
    return nullptr;
  }

  
  
  

  
  
  const nsStyleDisplay *disp = aNewStyleContext->StyleDisplay();
  nsCSSPseudoElements::Type pseudoType = aNewStyleContext->GetPseudoType();
  if (pseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    if (pseudoType != nsCSSPseudoElements::ePseudo_before &&
        pseudoType != nsCSSPseudoElements::ePseudo_after) {
      return nullptr;
    }

    NS_ASSERTION((pseudoType == nsCSSPseudoElements::ePseudo_before &&
                  aElement->Tag() == nsGkAtoms::mozgeneratedcontentbefore) ||
                 (pseudoType == nsCSSPseudoElements::ePseudo_after &&
                  aElement->Tag() == nsGkAtoms::mozgeneratedcontentafter),
                 "Unexpected aElement coming through");

    
    
    aElement = aElement->GetParent()->AsElement();
  }

  ElementTransitions *et =
      GetElementTransitions(aElement, pseudoType, false);
  if (!et &&
      disp->mTransitionPropertyCount == 1 &&
      disp->mTransitions[0].GetDelay() == 0.0f &&
      disp->mTransitions[0].GetDuration() == 0.0f) {
    return nullptr;
  }


  if (aNewStyleContext->PresContext()->IsProcessingAnimationStyleChange()) {
    return nullptr;
  }

  if (aNewStyleContext->GetParent() &&
      aNewStyleContext->GetParent()->HasPseudoElementData()) {
    
    
    
    return nullptr;
  }

  NS_WARN_IF_FALSE(!nsLayoutUtils::AreAsyncAnimationsEnabled() ||
                     mPresContext->ThrottledTransitionStyleIsUpToDate(),
                   "throttled animations not up to date");

  
  
  
  
  bool startedAny = false;
  nsCSSPropertySet whichStarted;
  for (uint32_t i = disp->mTransitionPropertyCount; i-- != 0; ) {
    const nsTransition& t = disp->mTransitions[i];
    
    
    if (t.GetDelay() != 0.0f || t.GetDuration() != 0.0f) {
      
      
      
      
      nsCSSProperty property = t.GetProperty();
      if (property == eCSSPropertyExtra_no_properties ||
          property == eCSSPropertyExtra_variable ||
          property == eCSSProperty_UNKNOWN) {
        
      } else if (property == eCSSPropertyExtra_all_properties) {
        for (nsCSSProperty p = nsCSSProperty(0);
             p < eCSSProperty_COUNT_no_shorthands;
             p = nsCSSProperty(p + 1)) {
          ConsiderStartingTransition(p, t, aElement, et,
                                     aOldStyleContext, aNewStyleContext,
                                     &startedAny, &whichStarted);
        }
      } else if (nsCSSProps::IsShorthand(property)) {
        CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(subprop, property) {
          ConsiderStartingTransition(*subprop, t, aElement, et,
                                     aOldStyleContext, aNewStyleContext,
                                     &startedAny, &whichStarted);
        }
      } else {
        ConsiderStartingTransition(property, t, aElement, et,
                                   aOldStyleContext, aNewStyleContext,
                                   &startedAny, &whichStarted);
      }
    }
  }

  
  
  
  
  
  if (et) {
    bool checkProperties =
      disp->mTransitions[0].GetProperty() != eCSSPropertyExtra_all_properties;
    nsCSSPropertySet allTransitionProperties;
    if (checkProperties) {
      for (uint32_t i = disp->mTransitionPropertyCount; i-- != 0; ) {
        const nsTransition& t = disp->mTransitions[i];
        
        
        nsCSSProperty property = t.GetProperty();
        if (property == eCSSPropertyExtra_no_properties ||
            property == eCSSPropertyExtra_variable ||
            property == eCSSProperty_UNKNOWN) {
          
        } else if (property == eCSSPropertyExtra_all_properties) {
          for (nsCSSProperty p = nsCSSProperty(0);
               p < eCSSProperty_COUNT_no_shorthands;
               p = nsCSSProperty(p + 1)) {
            allTransitionProperties.AddProperty(p);
          }
        } else if (nsCSSProps::IsShorthand(property)) {
          CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(subprop, property) {
            allTransitionProperties.AddProperty(*subprop);
          }
        } else {
          allTransitionProperties.AddProperty(property);
        }
      }
    }

    ElementAnimationPtrArray& animations = et->mAnimations;
    uint32_t i = animations.Length();
    NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
    nsStyleAnimation::Value currentValue;
    do {
      --i;
      ElementAnimation* animation = animations[i];
      MOZ_ASSERT(animation->mProperties.Length() == 1,
                 "Should have one animation property for a transition");
      MOZ_ASSERT(animation->mProperties[0].mSegments.Length() == 1,
                 "Animation property should have one segment for a transition");
      const AnimationProperty& prop = animation->mProperties[0];
      const AnimationPropertySegment& segment = prop.mSegments[0];
          
      if ((checkProperties &&
           !allTransitionProperties.HasProperty(prop.mProperty)) ||
          
          
          !ExtractComputedValueForTransition(prop.mProperty, aNewStyleContext,
                                             currentValue) ||
          currentValue != segment.mToValue) {
        
        animations.RemoveElementAt(i);
        et->UpdateAnimationGeneration(mPresContext);
      }
    } while (i != 0);

    if (animations.IsEmpty()) {
      et->Destroy();
      et = nullptr;
    }
  }

  if (!startedAny) {
    return nullptr;
  }

  NS_ABORT_IF_FALSE(et, "must have element transitions if we started "
                        "any transitions");

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<css::AnimValuesStyleRule> coverRule = new css::AnimValuesStyleRule;

  ElementAnimationPtrArray& animations = et->mAnimations;
  for (uint32_t i = 0, i_end = animations.Length(); i < i_end; ++i) {
    ElementAnimation* animation = animations[i];
    MOZ_ASSERT(animation->mProperties.Length() == 1,
               "Should have one animation property for a transition");
    MOZ_ASSERT(animation->mProperties[0].mSegments.Length() == 1,
               "Animation property should have one segment for a transition");
    AnimationProperty& prop = animation->mProperties[0];
    AnimationPropertySegment& segment = prop.mSegments[0];
    if (whichStarted.HasProperty(prop.mProperty)) {
      coverRule->AddValue(prop.mProperty, segment.mFromValue);
    }
  }

  et->mStyleRule = nullptr;

  return coverRule.forget();
}

void
nsTransitionManager::ConsiderStartingTransition(nsCSSProperty aProperty,
                                                const nsTransition& aTransition,
                                                dom::Element* aElement,
                                                ElementTransitions*& aElementTransitions,
                                                nsStyleContext* aOldStyleContext,
                                                nsStyleContext* aNewStyleContext,
                                                bool* aStartedAny,
                                                nsCSSPropertySet* aWhichStarted)
{
  
  NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty),
                    "property out of range");
  NS_ASSERTION(!aElementTransitions ||
               aElementTransitions->mElement == aElement, "Element mismatch");

  if (aWhichStarted->HasProperty(aProperty)) {
    
    
    
    
    return;
  }

  if (nsCSSProps::kAnimTypeTable[aProperty] == eStyleAnimType_None) {
    return;
  }

  nsRefPtr<ElementPropertyTransition> pt = new ElementPropertyTransition();

  nsStyleAnimation::Value startValue, endValue, dummyValue;
  bool haveValues =
    ExtractComputedValueForTransition(aProperty, aOldStyleContext,
                                      startValue) &&
    ExtractComputedValueForTransition(aProperty, aNewStyleContext,
                                      endValue);

  bool haveChange = startValue != endValue;

  bool shouldAnimate =
    haveValues &&
    haveChange &&
    
    
    
    nsStyleAnimation::Interpolate(aProperty, startValue, endValue,
                                  0.5, dummyValue);

  bool haveCurrentTransition = false;
  size_t currentIndex = nsTArray<ElementPropertyTransition>::NoIndex;
  const ElementPropertyTransition *oldPT = nullptr;
  if (aElementTransitions) {
    ElementAnimationPtrArray& animations = aElementTransitions->mAnimations;
    for (size_t i = 0, i_end = animations.Length(); i < i_end; ++i) {
      MOZ_ASSERT(animations[i]->mProperties.Length() == 1,
                 "Should have one animation property for a transition");
      if (animations[i]->mProperties[0].mProperty == aProperty) {
        haveCurrentTransition = true;
        currentIndex = i;
        oldPT =
          aElementTransitions->mAnimations[currentIndex]->AsTransition();
        break;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  MOZ_ASSERT(!oldPT || oldPT->mProperties[0].mSegments.Length() == 1,
             "Should have one animation property segment for a transition");
  if (haveCurrentTransition && haveValues &&
      oldPT->mProperties[0].mSegments[0].mToValue == endValue) {
    
    return;
  }

  nsPresContext *presContext = aNewStyleContext->PresContext();

  if (!shouldAnimate) {
    if (haveCurrentTransition) {
      
      
      
      
      
      
      ElementAnimationPtrArray& animations = aElementTransitions->mAnimations;
      animations.RemoveElementAt(currentIndex);
      aElementTransitions->UpdateAnimationGeneration(mPresContext);

      if (animations.IsEmpty()) {
        aElementTransitions->Destroy();
        
        aElementTransitions = nullptr;
      }
      
    }
    return;
  }

  TimeStamp mostRecentRefresh =
    presContext->RefreshDriver()->MostRecentRefresh();

  const nsTimingFunction &tf = aTransition.GetTimingFunction();
  float delay = aTransition.GetDelay();
  float duration = aTransition.GetDuration();
  if (duration < 0.0) {
    
    duration = 0.0;
  }
  pt->mStartForReversingTest = startValue;
  pt->mReversePortion = 1.0;

  
  
  if (haveCurrentTransition &&
      !oldPT->IsRemovedSentinel() &&
      oldPT->mStartForReversingTest == endValue) {
    
    
    double valuePortion =
      oldPT->ValuePortionFor(mostRecentRefresh) * oldPT->mReversePortion +
      (1.0 - oldPT->mReversePortion);
    
    
    
    
    if (valuePortion < 0.0) {
      valuePortion = -valuePortion;
    }
    
    
    
    
    
    if (valuePortion > 1.0) {
      valuePortion = 1.0;
    }

    
    
    
    if (delay < 0.0f) {
      delay *= valuePortion;
    }

    duration *= valuePortion;

    pt->mStartForReversingTest = oldPT->mProperties[0].mSegments[0].mToValue;
    pt->mReversePortion = valuePortion;
  }

  AnimationProperty& prop = *pt->mProperties.AppendElement();
  prop.mProperty = aProperty;

  AnimationPropertySegment& segment = *prop.mSegments.AppendElement();
  segment.mFromValue = startValue;
  segment.mToValue = endValue;
  segment.mFromKey = 0;
  segment.mToKey = 1;
  segment.mTimingFunction.Init(tf);

  pt->mStartTime = mostRecentRefresh;
  pt->mDelay = TimeDuration::FromMilliseconds(delay);
  pt->mTiming.mIterationDuration = TimeDuration::FromMilliseconds(duration);
  pt->mTiming.mIterationCount = 1;
  pt->mTiming.mDirection = NS_STYLE_ANIMATION_DIRECTION_NORMAL;
  pt->mTiming.mFillMode = NS_STYLE_ANIMATION_FILL_MODE_BACKWARDS;
  pt->mPlayState = NS_STYLE_ANIMATION_PLAY_STATE_RUNNING;
  pt->mPauseStart = TimeStamp();

  if (!aElementTransitions) {
    aElementTransitions =
      GetElementTransitions(aElement, aNewStyleContext->GetPseudoType(),
                            true);
    if (!aElementTransitions) {
      NS_WARNING("allocating ElementTransitions failed");
      return;
    }
  }

  ElementAnimationPtrArray &animations = aElementTransitions->mAnimations;
#ifdef DEBUG
  for (uint32_t i = 0, i_end = animations.Length(); i < i_end; ++i) {
    NS_ABORT_IF_FALSE(animations[i]->mProperties.Length() == 1,
                      "Should have one animation property for a transition");
    NS_ABORT_IF_FALSE(i == currentIndex ||
                      animations[i]->mProperties[0].mProperty != aProperty,
                      "duplicate transitions for property");
  }
#endif
  if (haveCurrentTransition) {
    animations[currentIndex] = pt;
  } else {
    if (!animations.AppendElement(pt)) {
      NS_WARNING("out of memory");
      return;
    }
  }
  aElementTransitions->UpdateAnimationGeneration(mPresContext);

  nsRestyleHint hint =
    aNewStyleContext->GetPseudoType() ==
      nsCSSPseudoElements::ePseudo_NotPseudoElement ?
    eRestyle_Self : eRestyle_Subtree;
  presContext->PresShell()->RestyleForAnimation(aElement, hint);

  *aStartedAny = true;
  aWhichStarted->AddProperty(aProperty);
}

ElementTransitions*
nsTransitionManager::GetElementTransitions(dom::Element *aElement,
                                           nsCSSPseudoElements::Type aPseudoType,
                                           bool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementData)) {
    
    return nullptr;
  }

  nsIAtom *propName;
  if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    propName = nsGkAtoms::transitionsProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_before) {
    propName = nsGkAtoms::transitionsOfBeforeProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_after) {
    propName = nsGkAtoms::transitionsOfAfterProperty;
  } else {
    NS_ASSERTION(!aCreateIfNeeded,
                 "should never try to create transitions for pseudo "
                 "other than :before or :after");
    return nullptr;
  }
  ElementTransitions *et = static_cast<ElementTransitions*>(
                             aElement->GetProperty(propName));
  if (!et && aCreateIfNeeded) {
    
    et = new ElementTransitions(aElement, propName, this,
      mPresContext->RefreshDriver()->MostRecentRefresh());
    nsresult rv = aElement->SetProperty(propName, et,
                                        ElementTransitionsPropertyDtor, false);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete et;
      return nullptr;
    }
    if (propName == nsGkAtoms::transitionsProperty) {
      aElement->SetMayHaveAnimations();
    }

    AddElementData(et);
  }

  return et;
}





void
nsTransitionManager::WalkTransitionRule(ElementDependentRuleProcessorData* aData,
                                        nsCSSPseudoElements::Type aPseudoType)
{
  ElementTransitions *et =
    GetElementTransitions(aData->mElement, aPseudoType, false);
  if (!et) {
    return;
  }

  if (!mPresContext->IsDynamic()) {
    
    return;
  }

  if (aData->mPresContext->IsProcessingRestyles() &&
      !aData->mPresContext->IsProcessingAnimationStyleChange()) {
    
    
    
    

    
    
    nsRestyleHint hint =
      aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ?
      eRestyle_Self : eRestyle_Subtree;
    mPresContext->PresShell()->RestyleForAnimation(aData->mElement, hint);
    return;
  }

  et->EnsureStyleRuleFor(
    aData->mPresContext->RefreshDriver()->MostRecentRefresh());

  aData->mRuleWalker->Forward(et->mStyleRule);
}

 void
nsTransitionManager::RulesMatching(ElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");
  WalkTransitionRule(aData,
                     nsCSSPseudoElements::ePseudo_NotPseudoElement);
}

 void
nsTransitionManager::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");

  
  
  
  WalkTransitionRule(aData, aData->mPseudoType);
}

 void
nsTransitionManager::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
nsTransitionManager::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif

 size_t
nsTransitionManager::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return CommonAnimationManager::SizeOfExcludingThis(aMallocSizeOf);
}

 size_t
nsTransitionManager::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

struct TransitionEventInfo {
  nsCOMPtr<nsIContent> mElement;
  InternalTransitionEvent mEvent;

  TransitionEventInfo(nsIContent *aElement, nsCSSProperty aProperty,
                      TimeDuration aDuration, const nsAString& aPseudoElement)
    : mElement(aElement)
    , mEvent(true, NS_TRANSITION_END)
  {
    
    mEvent.propertyName =
      NS_ConvertUTF8toUTF16(nsCSSProps::GetStringValue(aProperty));
    mEvent.elapsedTime = aDuration.ToSeconds();
    mEvent.pseudoElement = aPseudoElement;
  }

  
  
  TransitionEventInfo(const TransitionEventInfo &aOther)
    : mElement(aOther.mElement)
    , mEvent(true, NS_TRANSITION_END)
  {
    mEvent.AssignTransitionEventData(aOther.mEvent, false);
  }
};

 void
nsTransitionManager::WillRefresh(mozilla::TimeStamp aTime)
{
  NS_ABORT_IF_FALSE(mPresContext,
                    "refresh driver should not notify additional observers "
                    "after pres context has been destroyed");
  if (!mPresContext->GetPresShell()) {
    
    
    
    
    RemoveAllElementData();
    return;
  }

  FlushTransitions(Can_Throttle);
}

void
nsTransitionManager::FlushTransitions(FlushFlags aFlags)
{ 
  if (PR_CLIST_IS_EMPTY(&mElementData)) {
    
    return;
  }

  nsTArray<TransitionEventInfo> events;
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
  bool didThrottle = false;
  
  
  {
    PRCList *next = PR_LIST_HEAD(&mElementData);
    while (next != &mElementData) {
      ElementTransitions *et = static_cast<ElementTransitions*>(next);
      next = PR_NEXT_LINK(next);

      bool canThrottleTick = aFlags == Can_Throttle &&
        et->CanPerformOnCompositorThread(
          CommonElementAnimationData::CanAnimateFlags(0)) &&
        et->CanThrottleAnimation(now);

      NS_ABORT_IF_FALSE(et->mElement->GetCurrentDoc() ==
                          mPresContext->Document(),
                        "Element::UnbindFromTree should have "
                        "destroyed the element transitions object");

      uint32_t i = et->mAnimations.Length();
      NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
      bool transitionStartedOrEnded = false;
      do {
        --i;
        ElementPropertyTransition* pt = et->mAnimations[i]->AsTransition();
        if (pt->IsRemovedSentinel()) {
          
          
          
          
          
          if (aFlags == Can_Throttle) {
            et->mAnimations.RemoveElementAt(i);
          }
        } else if (pt->mStartTime + pt->mDelay +
                   pt->mTiming.mIterationDuration <= now) {
          MOZ_ASSERT(pt->mProperties.Length() == 1,
                     "Should have one animation property for a transition");
          nsCSSProperty prop = pt->mProperties[0].mProperty;
          if (nsCSSProps::PropHasFlags(prop, CSS_PROPERTY_REPORT_OTHER_NAME))
          {
            prop = nsCSSProps::OtherNameFor(prop);
          }
          nsIAtom* ep = et->mElementProperty;
          NS_NAMED_LITERAL_STRING(before, "::before");
          NS_NAMED_LITERAL_STRING(after, "::after");
          events.AppendElement(
            TransitionEventInfo(et->mElement, prop,
                                pt->mTiming.mIterationDuration,
                                ep == nsGkAtoms::transitionsProperty ?
                                  EmptyString() :
                                  ep == nsGkAtoms::transitionsOfBeforeProperty ?
                                    before :
                                    after));

          
          
          
          
          
          
          
          pt->SetRemovedSentinel();
          et->UpdateAnimationGeneration(mPresContext);
          transitionStartedOrEnded = true;
        } else if (pt->mStartTime + pt->mDelay <= now && canThrottleTick &&
                   !pt->mIsRunningOnCompositor) {
          
          
          et->UpdateAnimationGeneration(mPresContext);
          transitionStartedOrEnded = true;
        }
      } while (i != 0);

      
      
      NS_ASSERTION(et->mElementProperty == nsGkAtoms::transitionsProperty ||
                   et->mElementProperty == nsGkAtoms::transitionsOfBeforeProperty ||
                   et->mElementProperty == nsGkAtoms::transitionsOfAfterProperty,
                   "Unexpected element property; might restyle too much");
      if (!canThrottleTick || transitionStartedOrEnded) {
        nsRestyleHint hint = et->mElementProperty == nsGkAtoms::transitionsProperty ?
          eRestyle_Self : eRestyle_Subtree;
        mPresContext->PresShell()->RestyleForAnimation(et->mElement, hint);
      } else {
        didThrottle = true;
      }

      if (et->mAnimations.IsEmpty()) {
        et->Destroy();
        
        et = nullptr;
      }
    }
  }

  if (didThrottle) {
    mPresContext->Document()->SetNeedStyleFlush();
  }

  for (uint32_t i = 0, i_end = events.Length(); i < i_end; ++i) {
    TransitionEventInfo &info = events[i];
    EventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}

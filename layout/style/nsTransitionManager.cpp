







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
#include "nsEventDispatcher.h"
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
  
  
  
  double duration = mDuration.ToSeconds();
  NS_ABORT_IF_FALSE(duration >= 0.0, "negative duration forbidden");
  double timePortion;
  if (IsRemovedSentinel()) {
    
    
    timePortion = 1.0;
  } else if (duration == 0.0) {
    
    
    if (aRefreshTime >= mStartTime) {
      timePortion = 1.0;
    } else {
      timePortion = 0.0;
    }
  } else {
    timePortion = (aRefreshTime - mStartTime).ToSeconds() / duration;
    if (timePortion < 0.0)
      timePortion = 0.0; 
    if (timePortion > 1.0)
      timePortion = 1.0; 
  }

  return mTimingFunction.GetValue(timePortion);
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

    for (uint32_t i = 0, i_end = mPropertyTransitions.Length(); i < i_end; ++i)
    {
      ElementPropertyTransition &pt = mPropertyTransitions[i];
      if (pt.IsRemovedSentinel()) {
        continue;
      }

      nsStyleAnimation::Value *val = mStyleRule->AddEmptyValue(pt.mProperty);

      double valuePortion = pt.ValuePortionFor(aRefreshTime);
#ifdef DEBUG
      bool ok =
#endif
        nsStyleAnimation::Interpolate(pt.mProperty,
                                      pt.mStartValue, pt.mEndValue,
                                      valuePortion, *val);
      NS_ABORT_IF_FALSE(ok, "could not interpolate values");
    }
  }
}

bool
ElementPropertyTransition::IsRunningAt(TimeStamp aTime) const {
  return !IsRemovedSentinel() &&
         mStartTime <= aTime &&
         aTime < mStartTime + mDuration;
}

bool
ElementTransitions::HasAnimationOfProperty(nsCSSProperty aProperty) const
{
  for (uint32_t tranIdx = mPropertyTransitions.Length(); tranIdx-- != 0; ) {
    const ElementPropertyTransition& pt = mPropertyTransitions[tranIdx];
    if (aProperty == pt.mProperty && !pt.IsRemovedSentinel()) {
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

  for (uint32_t i = 0, i_end = mPropertyTransitions.Length(); i < i_end; ++i) {
    const ElementPropertyTransition& pt = mPropertyTransitions[i];
    if (css::IsGeometricProperty(pt.mProperty) && pt.IsRunningAt(now)) {
      aFlags = CanAnimateFlags(aFlags | CanAnimate_HasGeometricProperty);
      break;
    }
  }

  bool hasOpacity = false;
  bool hasTransform = false;
  bool existsProperty = false;
  for (uint32_t i = 0, i_end = mPropertyTransitions.Length(); i < i_end; ++i) {
    const ElementPropertyTransition& pt = mPropertyTransitions[i];
    if (pt.IsRemovedSentinel()) {
      continue;
    }
    
    existsProperty = true;

    if (!css::CommonElementAnimationData::CanAnimatePropertyOnCompositor(mElement,
                                                                         pt.mProperty,
                                                                         aFlags) ||
        css::CommonElementAnimationData::IsCompositorAnimationDisabledForFrame(frame)) {
      return false;
    }
    if (pt.mProperty == eCSSProperty_opacity) {
      hasOpacity = true;
    } else if (pt.mProperty == eCSSProperty_transform) {
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

    nsTArray<ElementPropertyTransition> &pts = et->mPropertyTransitions;
    uint32_t i = pts.Length();
    NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
    nsStyleAnimation::Value currentValue;
    do {
      --i;
      ElementPropertyTransition &pt = pts[i];
          
      if ((checkProperties &&
           !allTransitionProperties.HasProperty(pt.mProperty)) ||
          
          
          !ExtractComputedValueForTransition(pt.mProperty, aNewStyleContext,
                                             currentValue) ||
          currentValue != pt.mEndValue) {
        
        pts.RemoveElementAt(i);
        et->UpdateAnimationGeneration(mPresContext);
      }
    } while (i != 0);

    if (pts.IsEmpty()) {
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

  nsTArray<ElementPropertyTransition> &pts = et->mPropertyTransitions;
  for (uint32_t i = 0, i_end = pts.Length(); i < i_end; ++i) {
    ElementPropertyTransition &pt = pts[i];
    if (whichStarted.HasProperty(pt.mProperty)) {
      coverRule->AddValue(pt.mProperty, pt.mStartValue);
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

  ElementPropertyTransition pt;
  nsStyleAnimation::Value dummyValue;
  bool haveValues =
    ExtractComputedValueForTransition(aProperty, aOldStyleContext,
                                       pt.mStartValue) &&
    ExtractComputedValueForTransition(aProperty, aNewStyleContext,
                                       pt.mEndValue);

  bool haveChange = pt.mStartValue != pt.mEndValue;
    
  bool shouldAnimate =
    haveValues &&
    haveChange &&
    
    
    
    nsStyleAnimation::Interpolate(aProperty, pt.mStartValue, pt.mEndValue,
                                  0.5, dummyValue);

  bool haveCurrentTransition = false;
  uint32_t currentIndex = nsTArray<ElementPropertyTransition>::NoIndex;
  const ElementPropertyTransition *oldPT = nullptr;
  if (aElementTransitions) {
    nsTArray<ElementPropertyTransition> &pts =
      aElementTransitions->mPropertyTransitions;
    for (uint32_t i = 0, i_end = pts.Length(); i < i_end; ++i) {
      if (pts[i].mProperty == aProperty) {
        haveCurrentTransition = true;
        currentIndex = i;
        oldPT = &aElementTransitions->mPropertyTransitions[currentIndex];
        break;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  if (haveCurrentTransition && haveValues && oldPT->mEndValue == pt.mEndValue) {
    
    return;
  }

  nsPresContext *presContext = aNewStyleContext->PresContext();

  if (!shouldAnimate) {
    if (haveCurrentTransition) {
      
      
      
      
      
      
      nsTArray<ElementPropertyTransition> &pts =
        aElementTransitions->mPropertyTransitions;
      pts.RemoveElementAt(currentIndex);
      aElementTransitions->UpdateAnimationGeneration(mPresContext);

      if (pts.IsEmpty()) {
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
  pt.mStartForReversingTest = pt.mStartValue;
  pt.mReversePortion = 1.0;

  
  
  if (haveCurrentTransition &&
      !oldPT->IsRemovedSentinel() &&
      oldPT->mStartForReversingTest == pt.mEndValue) {
    
    
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

    pt.mStartForReversingTest = oldPT->mEndValue;
    pt.mReversePortion = valuePortion;
  }

  pt.mProperty = aProperty;
  pt.mStartTime = mostRecentRefresh + TimeDuration::FromMilliseconds(delay);
  pt.mDuration = TimeDuration::FromMilliseconds(duration);
  pt.mTimingFunction.Init(tf);
  if (!aElementTransitions) {
    aElementTransitions =
      GetElementTransitions(aElement, aNewStyleContext->GetPseudoType(),
                            true);
    if (!aElementTransitions) {
      NS_WARNING("allocating ElementTransitions failed");
      return;
    }
  }

  nsTArray<ElementPropertyTransition> &pts =
    aElementTransitions->mPropertyTransitions;
#ifdef DEBUG
  for (uint32_t i = 0, i_end = pts.Length(); i < i_end; ++i) {
    NS_ABORT_IF_FALSE(i == currentIndex ||
                      pts[i].mProperty != aProperty,
                      "duplicate transitions for property");
  }
#endif
  if (haveCurrentTransition) {
    pts[currentIndex] = pt;
  } else {
    if (!pts.AppendElement(pt)) {
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

      uint32_t i = et->mPropertyTransitions.Length();
      NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
      bool transitionStartedOrEnded = false;
      do {
        --i;
        ElementPropertyTransition &pt = et->mPropertyTransitions[i];
        if (pt.IsRemovedSentinel()) {
          
          
          
          
          
          if (aFlags == Can_Throttle) {
            et->mPropertyTransitions.RemoveElementAt(i);
          }
        } else if (pt.mStartTime + pt.mDuration <= now) {
          nsCSSProperty prop = pt.mProperty;
          if (nsCSSProps::PropHasFlags(prop, CSS_PROPERTY_REPORT_OTHER_NAME))
          {
            prop = nsCSSProps::OtherNameFor(prop);
          }
          nsIAtom* ep = et->mElementProperty;
          NS_NAMED_LITERAL_STRING(before, "::before");
          NS_NAMED_LITERAL_STRING(after, "::after");
          events.AppendElement(
            TransitionEventInfo(et->mElement, prop, pt.mDuration,
                                ep == nsGkAtoms::transitionsProperty ?
                                  EmptyString() :
                                  ep == nsGkAtoms::transitionsOfBeforeProperty ?
                                    before :
                                    after));

          
          
          
          
          
          
          
          pt.SetRemovedSentinel();
          et->UpdateAnimationGeneration(mPresContext);
          transitionStartedOrEnded = true;
        } else if (pt.mStartTime <= now && canThrottleTick &&
                   !pt.mIsRunningOnCompositor) {
          
          
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

      if (et->mPropertyTransitions.IsEmpty()) {
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
    nsEventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}

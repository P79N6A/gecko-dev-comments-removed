







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
#include "mozilla/EventDispatcher.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/dom/Element.h"
#include "nsIFrame.h"
#include "Layers.h"
#include "FrameLayerBuilder.h"
#include "nsDisplayList.h"
#include "nsStyleChangeList.h"
#include "nsStyleSet.h"
#include "RestyleManager.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;
using mozilla::dom::AnimationPlayer;
using mozilla::dom::Animation;

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::css;

double
ElementPropertyTransition::CurrentValuePortion() const
{
  
  
  
  MOZ_ASSERT(!IsFinishedTransition(),
             "Getting the value portion of a finished transition");
  MOZ_ASSERT(!GetLocalTime().IsNull(),
             "Getting the value portion of an animation that's not being "
             "sampled");

  
  
  
  
  
  
  AnimationTiming timingToUse = mTiming;
  timingToUse.mFillMode = NS_STYLE_ANIMATION_FILL_MODE_BOTH;
  ComputedTiming computedTiming = GetComputedTiming(&timingToUse);

  MOZ_ASSERT(computedTiming.mTimeFraction != ComputedTiming::kNullTimeFraction,
             "Got a null time fraction for a fill mode of 'both'");
  MOZ_ASSERT(mProperties.Length() == 1,
             "Should have one animation property for a transition");
  MOZ_ASSERT(mProperties[0].mSegments.Length() == 1,
             "Animation property should have one segment for a transition");
  return mProperties[0].mSegments[0].mTimingFunction
         .GetValue(computedTiming.mTimeFraction);
}





void
nsTransitionManager::ElementCollectionRemoved()
{
  
  
  if (PR_CLIST_IS_EMPTY(&mElementCollections)) {
    mPresContext->RefreshDriver()->RemoveRefreshObserver(this, Flush_Style);
  }
}

void
nsTransitionManager::AddElementCollection(
  AnimationPlayerCollection* aCollection)
{
  if (PR_CLIST_IS_EMPTY(&mElementCollections)) {
    
    nsRefreshDriver *rd = mPresContext->RefreshDriver();
    rd->AddRefreshObserver(this, Flush_Style);
  }

  PR_INSERT_BEFORE(aCollection, &mElementCollections);
}

already_AddRefed<nsIStyleRule>
nsTransitionManager::StyleContextChanged(dom::Element *aElement,
                                         nsStyleContext *aOldStyleContext,
                                         nsStyleContext *aNewStyleContext)
{
  NS_PRECONDITION(aOldStyleContext->GetPseudo() ==
                      aNewStyleContext->GetPseudo(),
                  "pseudo type mismatch");

  if (mInAnimationOnlyStyleUpdate) {
    
    
    
    
    
    return nullptr;
  }

  if (!mPresContext->IsDynamic()) {
    
    return nullptr;
  }

  if (aOldStyleContext->HasPseudoElementData() !=
      aNewStyleContext->HasPseudoElementData()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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

  AnimationPlayerCollection* collection =
    GetElementTransitions(aElement, pseudoType, false);
  if (!collection &&
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
                     mPresContext->RestyleManager()->
                       ThrottledAnimationStyleIsUpToDate(),
                   "throttled animations not up to date");

  
  
  
  
  bool startedAny = false;
  nsCSSPropertySet whichStarted;
  for (uint32_t i = disp->mTransitionPropertyCount; i-- != 0; ) {
    const StyleTransition& t = disp->mTransitions[i];
    
    
    if (t.GetDelay() != 0.0f || t.GetDuration() != 0.0f) {
      
      
      
      
      nsCSSProperty property = t.GetProperty();
      if (property == eCSSPropertyExtra_no_properties ||
          property == eCSSPropertyExtra_variable ||
          property == eCSSProperty_UNKNOWN) {
        
      } else if (property == eCSSPropertyExtra_all_properties) {
        for (nsCSSProperty p = nsCSSProperty(0);
             p < eCSSProperty_COUNT_no_shorthands;
             p = nsCSSProperty(p + 1)) {
          ConsiderStartingTransition(p, t, aElement, collection,
                                     aOldStyleContext, aNewStyleContext,
                                     &startedAny, &whichStarted);
        }
      } else if (nsCSSProps::IsShorthand(property)) {
        CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(subprop, property) {
          ConsiderStartingTransition(*subprop, t, aElement, collection,
                                     aOldStyleContext, aNewStyleContext,
                                     &startedAny, &whichStarted);
        }
      } else {
        ConsiderStartingTransition(property, t, aElement, collection,
                                   aOldStyleContext, aNewStyleContext,
                                   &startedAny, &whichStarted);
      }
    }
  }

  
  
  
  
  
  if (collection) {
    bool checkProperties =
      disp->mTransitions[0].GetProperty() != eCSSPropertyExtra_all_properties;
    nsCSSPropertySet allTransitionProperties;
    if (checkProperties) {
      for (uint32_t i = disp->mTransitionPropertyCount; i-- != 0; ) {
        const StyleTransition& t = disp->mTransitions[i];
        
        
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

    AnimationPlayerPtrArray& players = collection->mPlayers;
    size_t i = players.Length();
    NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
    StyleAnimationValue currentValue;
    do {
      --i;
      AnimationPlayer* player = players[i];
      dom::Animation* anim = player->GetSource();
      MOZ_ASSERT(anim && anim->Properties().Length() == 1,
                 "Should have one animation property for a transition");
      MOZ_ASSERT(anim && anim->Properties()[0].mSegments.Length() == 1,
                 "Animation property should have one segment for a transition");
      const AnimationProperty& prop = anim->Properties()[0];
      const AnimationPropertySegment& segment = prop.mSegments[0];
          
      if ((checkProperties &&
           !allTransitionProperties.HasProperty(prop.mProperty)) ||
          
          
          !ExtractComputedValueForTransition(prop.mProperty, aNewStyleContext,
                                             currentValue) ||
          currentValue != segment.mToValue) {
        
        players.RemoveElementAt(i);
        collection->UpdateAnimationGeneration(mPresContext);
      }
    } while (i != 0);

    if (players.IsEmpty()) {
      collection->Destroy();
      collection = nullptr;
    }
  }

  if (!startedAny) {
    return nullptr;
  }

  NS_ABORT_IF_FALSE(collection, "must have element transitions if we started "
                                "any transitions");

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<css::AnimValuesStyleRule> coverRule = new css::AnimValuesStyleRule;

  AnimationPlayerPtrArray& players = collection->mPlayers;
  for (size_t i = 0, i_end = players.Length(); i < i_end; ++i) {
    dom::Animation* anim = players[i]->GetSource();
    MOZ_ASSERT(anim && anim->Properties().Length() == 1,
               "Should have one animation property for a transition");
    MOZ_ASSERT(anim && anim->Properties()[0].mSegments.Length() == 1,
               "Animation property should have one segment for a transition");
    AnimationProperty& prop = anim->Properties()[0];
    AnimationPropertySegment& segment = prop.mSegments[0];
    if (whichStarted.HasProperty(prop.mProperty)) {
      coverRule->AddValue(prop.mProperty, segment.mFromValue);
    }
  }

  
  
  collection->mStyleRuleRefreshTime = TimeStamp();

  return coverRule.forget();
}

void
nsTransitionManager::ConsiderStartingTransition(
  nsCSSProperty aProperty,
  const StyleTransition& aTransition,
  dom::Element* aElement,
  AnimationPlayerCollection*& aElementTransitions,
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

  dom::AnimationTimeline* timeline = aElement->OwnerDoc()->Timeline();

  StyleAnimationValue startValue, endValue, dummyValue;
  bool haveValues =
    ExtractComputedValueForTransition(aProperty, aOldStyleContext,
                                      startValue) &&
    ExtractComputedValueForTransition(aProperty, aNewStyleContext,
                                      endValue);

  bool haveChange = startValue != endValue;

  bool shouldAnimate =
    haveValues &&
    haveChange &&
    
    
    
    StyleAnimationValue::Interpolate(aProperty, startValue, endValue,
                                     0.5, dummyValue);

  bool haveCurrentTransition = false;
  size_t currentIndex = nsTArray<ElementPropertyTransition>::NoIndex;
  const ElementPropertyTransition *oldPT = nullptr;
  if (aElementTransitions) {
    AnimationPlayerPtrArray& players = aElementTransitions->mPlayers;
    for (size_t i = 0, i_end = players.Length(); i < i_end; ++i) {
      MOZ_ASSERT(players[i]->GetSource() &&
                 players[i]->GetSource()->Properties().Length() == 1,
                 "Should have one animation property for a transition");
      if (players[i]->GetSource()->Properties()[0].mProperty == aProperty) {
        haveCurrentTransition = true;
        currentIndex = i;
        oldPT = players[currentIndex]->GetSource()->AsTransition();
        break;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  MOZ_ASSERT(!oldPT || oldPT->Properties()[0].mSegments.Length() == 1,
             "Should have one animation property segment for a transition");
  if (haveCurrentTransition && haveValues &&
      oldPT->Properties()[0].mSegments[0].mToValue == endValue) {
    
    return;
  }

  nsPresContext *presContext = aNewStyleContext->PresContext();

  if (!shouldAnimate) {
    if (haveCurrentTransition) {
      
      
      
      
      
      
      AnimationPlayerPtrArray& players = aElementTransitions->mPlayers;
      oldPT = nullptr; 
      players.RemoveElementAt(currentIndex);
      aElementTransitions->UpdateAnimationGeneration(mPresContext);

      if (players.IsEmpty()) {
        aElementTransitions->Destroy();
        
        aElementTransitions = nullptr;
      }
      
    }
    return;
  }

  const nsTimingFunction &tf = aTransition.GetTimingFunction();
  float delay = aTransition.GetDelay();
  float duration = aTransition.GetDuration();
  if (duration < 0.0) {
    
    duration = 0.0;
  }

  StyleAnimationValue startForReversingTest = startValue;
  double reversePortion = 1.0;

  
  
  if (haveCurrentTransition &&
      !oldPT->IsFinishedTransition() &&
      oldPT->mStartForReversingTest == endValue) {
    
    
    double valuePortion =
      oldPT->CurrentValuePortion() * oldPT->mReversePortion +
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

    startForReversingTest = oldPT->Properties()[0].mSegments[0].mToValue;
    reversePortion = valuePortion;
  }

  AnimationTiming timing;
  timing.mIterationDuration = TimeDuration::FromMilliseconds(duration);
  timing.mDelay = TimeDuration::FromMilliseconds(delay);
  timing.mIterationCount = 1;
  timing.mDirection = NS_STYLE_ANIMATION_DIRECTION_NORMAL;
  timing.mFillMode = NS_STYLE_ANIMATION_FILL_MODE_BACKWARDS;

  nsRefPtr<ElementPropertyTransition> pt =
    new ElementPropertyTransition(aElement->OwnerDoc(), timing);
  pt->mStartForReversingTest = startForReversingTest;
  pt->mReversePortion = reversePortion;

  AnimationProperty& prop = *pt->Properties().AppendElement();
  prop.mProperty = aProperty;

  AnimationPropertySegment& segment = *prop.mSegments.AppendElement();
  segment.mFromValue = startValue;
  segment.mToValue = endValue;
  segment.mFromKey = 0;
  segment.mToKey = 1;
  segment.mTimingFunction.Init(tf);

  nsRefPtr<dom::AnimationPlayer> player = new dom::AnimationPlayer(timeline);
  player->mStartTime = timeline->GetCurrentTimeDuration();
  player->SetSource(pt);

  if (!aElementTransitions) {
    aElementTransitions =
      GetElementTransitions(aElement, aNewStyleContext->GetPseudoType(),
                            true);
    if (!aElementTransitions) {
      NS_WARNING("allocating CommonAnimationManager failed");
      return;
    }
  }

  AnimationPlayerPtrArray& players = aElementTransitions->mPlayers;
#ifdef DEBUG
  for (size_t i = 0, i_end = players.Length(); i < i_end; ++i) {
    NS_ABORT_IF_FALSE(players[i]->GetSource() &&
                      players[i]->GetSource()->Properties().Length() == 1,
                      "Should have one animation property for a transition");
    NS_ABORT_IF_FALSE(i == currentIndex ||
                      (players[i]->GetSource() &&
                       players[i]->GetSource()->Properties()[0].mProperty
                       != aProperty),
                      "duplicate transitions for property");
  }
#endif
  if (haveCurrentTransition) {
    players[currentIndex] = player;
  } else {
    if (!players.AppendElement(player)) {
      NS_WARNING("out of memory");
      return;
    }
  }
  aElementTransitions->UpdateAnimationGeneration(mPresContext);
  aElementTransitions->PostRestyleForAnimation(presContext);

  *aStartedAny = true;
  aWhichStarted->AddProperty(aProperty);
}

AnimationPlayerCollection*
nsTransitionManager::GetElementTransitions(
  dom::Element *aElement,
  nsCSSPseudoElements::Type aPseudoType,
  bool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementCollections)) {
    
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
  AnimationPlayerCollection* collection =
    static_cast<AnimationPlayerCollection*>(aElement->GetProperty(propName));
  if (!collection && aCreateIfNeeded) {
    
    collection = new AnimationPlayerCollection(aElement, propName, this,
      mPresContext->RefreshDriver()->MostRecentRefresh());
    nsresult rv =
      aElement->SetProperty(propName, collection,
                            &AnimationPlayerCollection::PropertyDtor, false);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete collection;
      return nullptr;
    }
    if (propName == nsGkAtoms::transitionsProperty) {
      aElement->SetMayHaveAnimations();
    }

    AddElementCollection(collection);
  }

  return collection;
}





void
nsTransitionManager::WalkTransitionRule(
  ElementDependentRuleProcessorData* aData,
  nsCSSPseudoElements::Type aPseudoType)
{
  AnimationPlayerCollection* collection =
    GetElementTransitions(aData->mElement, aPseudoType, false);
  if (!collection) {
    return;
  }

  if (!mPresContext->IsDynamic()) {
    
    return;
  }

  if (aData->mPresContext->IsProcessingRestyles() &&
      !aData->mPresContext->IsProcessingAnimationStyleChange()) {
    
    
    
    

    
    
    collection->PostRestyleForAnimation(mPresContext);
    return;
  }

  collection->mNeedsRefreshes = true;
  collection->EnsureStyleRuleFor(
    aData->mPresContext->RefreshDriver()->MostRecentRefresh(),
    EnsureStyleRule_IsNotThrottled);

  if (collection->mStyleRule) {
    aData->mRuleWalker->Forward(collection->mStyleRule);
  }
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
    
    
    
    
    RemoveAllElementCollections();
    return;
  }

  FlushTransitions(Can_Throttle);
}

void
nsTransitionManager::FlushTransitions(FlushFlags aFlags)
{
  if (PR_CLIST_IS_EMPTY(&mElementCollections)) {
    
    return;
  }

  nsTArray<TransitionEventInfo> events;
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
  bool didThrottle = false;
  
  
  {
    PRCList *next = PR_LIST_HEAD(&mElementCollections);
    while (next != &mElementCollections) {
      AnimationPlayerCollection* collection =
        static_cast<AnimationPlayerCollection*>(next);
      next = PR_NEXT_LINK(next);

      collection->Tick();
      bool canThrottleTick = aFlags == Can_Throttle &&
        collection->CanPerformOnCompositorThread(
          AnimationPlayerCollection::CanAnimateFlags(0)) &&
        collection->CanThrottleAnimation(now);

      NS_ABORT_IF_FALSE(collection->mElement->GetCrossShadowCurrentDoc() ==
                          mPresContext->Document(),
                        "Element::UnbindFromTree should have "
                        "destroyed the element transitions object");

      size_t i = collection->mPlayers.Length();
      NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
      bool transitionStartedOrEnded = false;
      do {
        --i;
        AnimationPlayer* player = collection->mPlayers[i];
        if (player->GetSource()->IsFinishedTransition()) {
          
          
          
          
          
          if (aFlags == Can_Throttle) {
            collection->mPlayers.RemoveElementAt(i);
          }
        } else {
          MOZ_ASSERT(player->GetSource(),
                     "Transitions should have source content");
          ComputedTiming computedTiming =
            player->GetSource()->GetComputedTiming();
          if (computedTiming.mPhase == ComputedTiming::AnimationPhase_After) {
            MOZ_ASSERT(player->GetSource()->Properties().Length() == 1,
                       "Should have one animation property for a transition");
            nsCSSProperty prop = player->GetSource()->Properties()[0].mProperty;
            if (nsCSSProps::PropHasFlags(prop, CSS_PROPERTY_REPORT_OTHER_NAME))
            {
              prop = nsCSSProps::OtherNameFor(prop);
            }
            TimeDuration duration =
              player->GetSource()->Timing().mIterationDuration;
            events.AppendElement(
              TransitionEventInfo(collection->mElement, prop,
                                  duration,
                                  collection->PseudoElement()));

            
            
            
            
            
            
            
            player->GetSource()->SetIsFinishedTransition();
            collection->UpdateAnimationGeneration(mPresContext);
            transitionStartedOrEnded = true;
          } else if ((computedTiming.mPhase ==
                      ComputedTiming::AnimationPhase_Active) &&
                     canThrottleTick &&
                    !player->mIsRunningOnCompositor) {
            
            
            collection->UpdateAnimationGeneration(mPresContext);
            transitionStartedOrEnded = true;
          }
        }
      } while (i != 0);

      
      
      MOZ_ASSERT(collection->mElementProperty ==
                   nsGkAtoms::transitionsProperty ||
                 collection->mElementProperty ==
                   nsGkAtoms::transitionsOfBeforeProperty ||
                 collection->mElementProperty ==
                   nsGkAtoms::transitionsOfAfterProperty,
                 "Unexpected element property; might restyle too much");
      if (!canThrottleTick || transitionStartedOrEnded) {
        collection->PostRestyleForAnimation(mPresContext);
      } else {
        didThrottle = true;
      }

      if (collection->mPlayers.IsEmpty()) {
        collection->Destroy();
        
        collection = nullptr;
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

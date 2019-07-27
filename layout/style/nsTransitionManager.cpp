







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
#include "nsCSSProps.h"
#include "nsDisplayList.h"
#include "nsStyleChangeList.h"
#include "nsStyleSet.h"
#include "RestyleManager.h"
#include "nsDOMMutationObserver.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;
using mozilla::dom::AnimationPlayer;
using mozilla::dom::Animation;

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::css;

const nsString&
ElementPropertyTransition::Name() const
{
   if (!mName.Length()) {
     const_cast<ElementPropertyTransition*>(this)->mName =
       NS_ConvertUTF8toUTF16(nsCSSProps::GetStringValue(TransitionProperty()));
   }
   return dom::Animation::Name();
}

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





mozilla::dom::AnimationPlayState
CSSTransitionPlayer::PlayStateFromJS() const
{
  FlushStyle();
  return AnimationPlayer::PlayStateFromJS();
}

void
CSSTransitionPlayer::PlayFromJS()
{
  FlushStyle();
  AnimationPlayer::PlayFromJS();
}

CommonAnimationManager*
CSSTransitionPlayer::GetAnimationManager() const
{
  nsPresContext* context = GetPresContext();
  if (!context) {
    return nullptr;
  }

  return context->TransitionManager();
}





void
nsTransitionManager::StyleContextChanged(dom::Element *aElement,
                                         nsStyleContext *aOldStyleContext,
                                         nsRefPtr<nsStyleContext>* aNewStyleContext )
{
  nsStyleContext* newStyleContext = *aNewStyleContext;

  NS_PRECONDITION(aOldStyleContext->GetPseudo() == newStyleContext->GetPseudo(),
                  "pseudo type mismatch");

  if (mInAnimationOnlyStyleUpdate) {
    
    
    
    
    
    return;
  }

  if (!mPresContext->IsDynamic()) {
    
    return;
  }

  if (aOldStyleContext->HasPseudoElementData() !=
      newStyleContext->HasPseudoElementData()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return;
  }

  
  
  

  
  
  const nsStyleDisplay *disp = newStyleContext->StyleDisplay();
  nsCSSPseudoElements::Type pseudoType = newStyleContext->GetPseudoType();
  if (pseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    if (pseudoType != nsCSSPseudoElements::ePseudo_before &&
        pseudoType != nsCSSPseudoElements::ePseudo_after) {
      return;
    }

    NS_ASSERTION((pseudoType == nsCSSPseudoElements::ePseudo_before &&
                  aElement->NodeInfo()->NameAtom() == nsGkAtoms::mozgeneratedcontentbefore) ||
                 (pseudoType == nsCSSPseudoElements::ePseudo_after &&
                  aElement->NodeInfo()->NameAtom() == nsGkAtoms::mozgeneratedcontentafter),
                 "Unexpected aElement coming through");

    
    
    aElement = aElement->GetParent()->AsElement();
  }

  AnimationPlayerCollection* collection =
    GetAnimations(aElement, pseudoType, false);
  if (!collection &&
      disp->mTransitionPropertyCount == 1 &&
      disp->mTransitions[0].GetCombinedDuration() <= 0.0f) {
    return;
  }

  if (collection &&
      collection->mCheckGeneration ==
        mPresContext->RestyleManager()->GetAnimationGeneration()) {
    
    
    
    
    
    
    return;
  }
  if (newStyleContext->GetParent() &&
      newStyleContext->GetParent()->HasPseudoElementData()) {
    
    
    
    return;
  }

  NS_WARN_IF_FALSE(!nsLayoutUtils::AreAsyncAnimationsEnabled() ||
                     mPresContext->RestyleManager()->
                       ThrottledAnimationStyleIsUpToDate(),
                   "throttled animations not up to date");

  
  
  
  
  nsRefPtr<nsStyleContext> afterChangeStyle;
  if (collection) {
    nsStyleSet* styleSet = mPresContext->StyleSet();
    afterChangeStyle =
      styleSet->ResolveStyleWithoutAnimation(aElement, newStyleContext,
                                             eRestyle_CSSTransitions);
  } else {
    afterChangeStyle = newStyleContext;
  }

  nsAutoAnimationMutationBatch mb(aElement);

  
  
  
  
  bool startedAny = false;
  nsCSSPropertySet whichStarted;
  for (uint32_t i = disp->mTransitionPropertyCount; i-- != 0; ) {
    const StyleTransition& t = disp->mTransitions[i];
    
    
    
    if (t.GetCombinedDuration() > 0.0f) {
      
      
      
      
      nsCSSProperty property = t.GetProperty();
      if (property == eCSSPropertyExtra_no_properties ||
          property == eCSSPropertyExtra_variable ||
          property == eCSSProperty_UNKNOWN) {
        
      } else if (property == eCSSPropertyExtra_all_properties) {
        for (nsCSSProperty p = nsCSSProperty(0);
             p < eCSSProperty_COUNT_no_shorthands;
             p = nsCSSProperty(p + 1)) {
          ConsiderStartingTransition(p, t, aElement, collection,
                                     aOldStyleContext, afterChangeStyle,
                                     &startedAny, &whichStarted);
        }
      } else if (nsCSSProps::IsShorthand(property)) {
        CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(
            subprop, property, nsCSSProps::eEnabledForAllContent) {
          ConsiderStartingTransition(*subprop, t, aElement, collection,
                                     aOldStyleContext, afterChangeStyle,
                                     &startedAny, &whichStarted);
        }
      } else {
        ConsiderStartingTransition(property, t, aElement, collection,
                                   aOldStyleContext, afterChangeStyle,
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
          CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(
              subprop, property, nsCSSProps::eEnabledForAllContent) {
            allTransitionProperties.AddProperty(*subprop);
          }
        } else {
          allTransitionProperties.AddProperty(property);
        }
      }
    }

    AnimationPlayerPtrArray& players = collection->mPlayers;
    size_t i = players.Length();
    MOZ_ASSERT(i != 0, "empty transitions list?");
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
          
          
          
          
          
          !ExtractComputedValueForTransition(prop.mProperty, afterChangeStyle,
                                             currentValue) ||
          currentValue != segment.mToValue) {
        
        if (!player->GetSource()->IsFinishedTransition()) {
          player->Cancel();
          collection->UpdateAnimationGeneration(mPresContext);
        }
        players.RemoveElementAt(i);
      }
    } while (i != 0);

    if (players.IsEmpty()) {
      collection->Destroy();
      collection = nullptr;
    }
  }

  MOZ_ASSERT(!startedAny || collection,
             "must have element transitions if we started any transitions");

  if (collection) {
    UpdateCascadeResultsWithTransitions(collection);

    
    
    collection->mStyleRuleRefreshTime = TimeStamp();
    collection->UpdateCheckGeneration(mPresContext);
    collection->mNeedsRefreshes = true;
    TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
    collection->EnsureStyleRuleFor(now, EnsureStyleRule_IsNotThrottled);
  }

  
  *aNewStyleContext = afterChangeStyle;
  if (collection) {
    
    
    
    
    collection->PostRestyleForAnimation(mPresContext);
  }
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
  
  MOZ_ASSERT(!nsCSSProps::IsShorthand(aProperty),
             "property out of range");
  NS_ASSERTION(!aElementTransitions ||
               aElementTransitions->mElement == aElement, "Element mismatch");

  if (aWhichStarted->HasProperty(aProperty)) {
    
    
    
    
    return;
  }

  if (nsCSSProps::kAnimTypeTable[aProperty] == eStyleAnimType_None) {
    return;
  }

  dom::DocumentTimeline* timeline = aElement->OwnerDoc()->Timeline();

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
      const ElementPropertyTransition *iPt =
        players[i]->GetSource()->AsTransition();
      if (iPt->TransitionProperty() == aProperty) {
        haveCurrentTransition = true;
        currentIndex = i;
        oldPT = iPt;
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

  if (!shouldAnimate) {
    if (haveCurrentTransition && !oldPT->IsFinishedTransition()) {
      
      
      
      
      
      
      AnimationPlayerPtrArray& players = aElementTransitions->mPlayers;
      players[currentIndex]->Cancel();
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
    new ElementPropertyTransition(aElement->OwnerDoc(), aElement,
                                  aNewStyleContext->GetPseudoType(), timing);
  pt->mStartForReversingTest = startForReversingTest;
  pt->mReversePortion = reversePortion;

  AnimationProperty& prop = *pt->Properties().AppendElement();
  prop.mProperty = aProperty;
  prop.mWinsInCascade = true;

  AnimationPropertySegment& segment = *prop.mSegments.AppendElement();
  segment.mFromValue = startValue;
  segment.mToValue = endValue;
  segment.mFromKey = 0;
  segment.mToKey = 1;
  segment.mTimingFunction.Init(tf);

  nsRefPtr<CSSTransitionPlayer> player = new CSSTransitionPlayer(timeline);
  
  
  
  
  player->SetSource(pt);
  player->PlayFromStyle();

  if (!aElementTransitions) {
    aElementTransitions =
      GetAnimations(aElement, aNewStyleContext->GetPseudoType(), true);
    if (!aElementTransitions) {
      NS_WARNING("allocating CommonAnimationManager failed");
      return;
    }
  }

  AnimationPlayerPtrArray& players = aElementTransitions->mPlayers;
#ifdef DEBUG
  for (size_t i = 0, i_end = players.Length(); i < i_end; ++i) {
    MOZ_ASSERT(
      i == currentIndex ||
      (players[i]->GetSource() &&
       players[i]->GetSource()->AsTransition()->TransitionProperty()
         != aProperty),
      "duplicate transitions for property");
  }
#endif
  if (haveCurrentTransition) {
    players[currentIndex]->Cancel();
    oldPT = nullptr; 
    players[currentIndex] = player;
  } else {
    if (!players.AppendElement(player)) {
      NS_WARNING("out of memory");
      return;
    }
  }
  aElementTransitions->UpdateAnimationGeneration(mPresContext);

  *aStartedAny = true;
  aWhichStarted->AddProperty(aProperty);
}

void
nsTransitionManager::UpdateCascadeResultsWithTransitions(
                       AnimationPlayerCollection* aTransitions)
{
  AnimationPlayerCollection* animations =
    mPresContext->AnimationManager()->
      GetAnimations(aTransitions->mElement,
                    aTransitions->PseudoElementType(), false);
  UpdateCascadeResults(aTransitions, animations);
}

void
nsTransitionManager::UpdateCascadeResultsWithAnimations(
                       AnimationPlayerCollection* aAnimations)
{
  AnimationPlayerCollection* transitions =
    mPresContext->TransitionManager()->
      GetAnimations(aAnimations->mElement,
                    aAnimations->PseudoElementType(), false);
  UpdateCascadeResults(transitions, aAnimations);
}

void
nsTransitionManager::UpdateCascadeResultsWithAnimationsToBeDestroyed(
                       const AnimationPlayerCollection* aAnimations)
{
  
  
  
  AnimationPlayerCollection* transitions =
    mPresContext->TransitionManager()->
      GetAnimations(aAnimations->mElement,
                    aAnimations->PseudoElementType(), false);
  UpdateCascadeResults(transitions, nullptr);
}

void
nsTransitionManager::UpdateCascadeResults(
                       AnimationPlayerCollection* aTransitions,
                       AnimationPlayerCollection* aAnimations)
{
  if (!aTransitions) {
    
    return;
  }

  nsCSSPropertySet propertiesUsed;
#ifdef DEBUG
  nsCSSPropertySet propertiesWithTransitions;
#endif

  
  
  
  if (aAnimations) {
    TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
    
    
    aAnimations->EnsureStyleRuleFor(now, EnsureStyleRule_IsThrottled);

    if (aAnimations->mStyleRule) {
      aAnimations->mStyleRule->AddPropertiesToSet(propertiesUsed);
    }
  }

  
  
  
  bool changed = false;
  AnimationPlayerPtrArray& players = aTransitions->mPlayers;
  for (size_t playerIdx = players.Length(); playerIdx-- != 0; ) {
    MOZ_ASSERT(players[playerIdx]->GetSource() &&
               players[playerIdx]->GetSource()->Properties().Length() == 1,
               "Should have one animation property for a transition");
    AnimationProperty& prop = players[playerIdx]->GetSource()->Properties()[0];
    bool newWinsInCascade = !propertiesUsed.HasProperty(prop.mProperty);
    if (prop.mWinsInCascade != newWinsInCascade) {
      changed = true;
    }
    prop.mWinsInCascade = newWinsInCascade;
    
    
#ifdef DEBUG
    MOZ_ASSERT(!propertiesWithTransitions.HasProperty(prop.mProperty),
               "we're assuming we have only one transition per property");
    propertiesWithTransitions.AddProperty(prop.mProperty);
#endif
  }

  if (changed) {
    mPresContext->RestyleManager()->IncrementAnimationGeneration();
    aTransitions->UpdateAnimationGeneration(mPresContext);
    aTransitions->PostUpdateLayerAnimations();

    
    aTransitions->mStyleRuleRefreshTime = TimeStamp();
    aTransitions->mNeedsRefreshes = true;
  }
}





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
  MOZ_ASSERT(mPresContext,
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

      nsAutoAnimationMutationBatch mb(collection->mElement);

      collection->Tick();
      bool canThrottleTick = aFlags == Can_Throttle &&
        collection->CanPerformOnCompositorThread(
          AnimationPlayerCollection::CanAnimateFlags(0)) &&
        collection->CanThrottleAnimation(now);

      MOZ_ASSERT(collection->mElement->GetCrossShadowCurrentDoc() ==
                   mPresContext->Document(),
                 "Element::UnbindFromTree should have "
                 "destroyed the element transitions object");

      size_t i = collection->mPlayers.Length();
      MOZ_ASSERT(i != 0, "empty transitions list?");
      bool transitionStartedOrEnded = false;
      do {
        --i;
        AnimationPlayer* player = collection->mPlayers[i];
        if (!player->GetSource()->IsFinishedTransition()) {
          MOZ_ASSERT(player->GetSource(),
                     "Transitions should have source content");
          ComputedTiming computedTiming =
            player->GetSource()->GetComputedTiming();
          if (computedTiming.mPhase == ComputedTiming::AnimationPhase_After) {
            nsCSSProperty prop =
              player->GetSource()->AsTransition()->TransitionProperty();
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
                     !player->IsRunningOnCompositor()) {
            
            
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

  MaybeStartOrStopObservingRefreshDriver();

  for (uint32_t i = 0, i_end = events.Length(); i < i_end; ++i) {
    TransitionEventInfo &info = events[i];
    EventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}

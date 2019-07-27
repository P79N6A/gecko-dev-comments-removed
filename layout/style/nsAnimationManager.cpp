




#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/dom/KeyframeEffect.h"

#include "nsPresContext.h"
#include "nsStyleSet.h"
#include "nsStyleChangeList.h"
#include "nsCSSRules.h"
#include "RestyleManager.h"
#include "nsLayoutUtils.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsDOMMutationObserver.h"
#include <math.h>

using namespace mozilla;
using namespace mozilla::css;
using mozilla::dom::AnimationPlayer;
using mozilla::dom::KeyframeEffectReadonly;
using mozilla::CSSAnimationPlayer;

mozilla::dom::Promise*
CSSAnimationPlayer::GetReady(ErrorResult& aRv)
{
  FlushStyle();
  return AnimationPlayer::GetReady(aRv);
}

void
CSSAnimationPlayer::Play(LimitBehavior aLimitBehavior)
{
  mPauseShouldStick = false;
  AnimationPlayer::Play(aLimitBehavior);
}

void
CSSAnimationPlayer::Pause()
{
  mPauseShouldStick = true;
  AnimationPlayer::Pause();
}

mozilla::dom::AnimationPlayState
CSSAnimationPlayer::PlayStateFromJS() const
{
  
  
  FlushStyle();
  return AnimationPlayer::PlayStateFromJS();
}

void
CSSAnimationPlayer::PlayFromJS()
{
  
  
  FlushStyle();
  AnimationPlayer::PlayFromJS();
}

void
CSSAnimationPlayer::PlayFromStyle()
{
  mIsStylePaused = false;
  if (!mPauseShouldStick) {
    DoPlay(AnimationPlayer::LimitBehavior::Continue);
  }
}

void
CSSAnimationPlayer::PauseFromStyle()
{
  
  if (mIsStylePaused) {
    return;
  }

  mIsStylePaused = true;
  DoPause();
}

void
CSSAnimationPlayer::QueueEvents(EventArray& aEventsToDispatch)
{
  if (!mEffect) {
    return;
  }

  ComputedTiming computedTiming = mEffect->GetComputedTiming();

  if (computedTiming.mPhase == ComputedTiming::AnimationPhase_Null) {
    return; 
  }

  
  
  
  
  
  

  bool wasActive = mPreviousPhaseOrIteration != PREVIOUS_PHASE_BEFORE &&
                   mPreviousPhaseOrIteration != PREVIOUS_PHASE_AFTER;
  bool isActive =
         computedTiming.mPhase == ComputedTiming::AnimationPhase_Active;
  bool isSameIteration =
         computedTiming.mCurrentIteration == mPreviousPhaseOrIteration;
  bool skippedActivePhase =
    (mPreviousPhaseOrIteration == PREVIOUS_PHASE_BEFORE &&
     computedTiming.mPhase == ComputedTiming::AnimationPhase_After) ||
    (mPreviousPhaseOrIteration == PREVIOUS_PHASE_AFTER &&
     computedTiming.mPhase == ComputedTiming::AnimationPhase_Before);

  MOZ_ASSERT(!skippedActivePhase || (!isActive && !wasActive),
             "skippedActivePhase only makes sense if we were & are inactive");

  if (computedTiming.mPhase == ComputedTiming::AnimationPhase_Before) {
    mPreviousPhaseOrIteration = PREVIOUS_PHASE_BEFORE;
  } else if (computedTiming.mPhase == ComputedTiming::AnimationPhase_Active) {
    mPreviousPhaseOrIteration = computedTiming.mCurrentIteration;
  } else if (computedTiming.mPhase == ComputedTiming::AnimationPhase_After) {
    mPreviousPhaseOrIteration = PREVIOUS_PHASE_AFTER;
  }

  dom::Element* target;
  nsCSSPseudoElements::Type targetPseudoType;
  mEffect->GetTarget(target, targetPseudoType);

  uint32_t message;

  if (!wasActive && isActive) {
    message = NS_ANIMATION_START;
  } else if (wasActive && !isActive) {
    message = NS_ANIMATION_END;
  } else if (wasActive && isActive && !isSameIteration) {
    message = NS_ANIMATION_ITERATION;
  } else if (skippedActivePhase) {
    
    
    StickyTimeDuration elapsedTime =
      std::min(StickyTimeDuration(mEffect->InitialAdvance()),
               computedTiming.mActiveDuration);
    AnimationEventInfo ei(target, Name(), NS_ANIMATION_START,
                          elapsedTime,
                          PseudoTypeAsString(targetPseudoType));
    aEventsToDispatch.AppendElement(ei);
    
    message = NS_ANIMATION_END;
  } else {
    return; 
  }

  StickyTimeDuration elapsedTime;

  if (message == NS_ANIMATION_START ||
      message == NS_ANIMATION_ITERATION) {
    TimeDuration iterationStart = mEffect->Timing().mIterationDuration *
                                    computedTiming.mCurrentIteration;
    elapsedTime = StickyTimeDuration(std::max(iterationStart,
                                              mEffect->InitialAdvance()));
  } else {
    MOZ_ASSERT(message == NS_ANIMATION_END);
    elapsedTime = computedTiming.mActiveDuration;
  }

  AnimationEventInfo ei(target, Name(), message, elapsedTime,
                        PseudoTypeAsString(targetPseudoType));
  aEventsToDispatch.AppendElement(ei);
}

CommonAnimationManager*
CSSAnimationPlayer::GetAnimationManager() const
{
  nsPresContext* context = GetPresContext();
  if (!context) {
    return nullptr;
  }

  return context->AnimationManager();
}

 nsString
CSSAnimationPlayer::PseudoTypeAsString(nsCSSPseudoElements::Type aPseudoType)
{
  switch (aPseudoType) {
    case nsCSSPseudoElements::ePseudo_before:
      return NS_LITERAL_STRING("::before");
    case nsCSSPseudoElements::ePseudo_after:
      return NS_LITERAL_STRING("::after");
    default:
      return EmptyString();
  }
}

void
nsAnimationManager::UpdateStyleAndEvents(AnimationPlayerCollection*
                                           aCollection,
                                         TimeStamp aRefreshTime,
                                         EnsureStyleRuleFlags aFlags)
{
  aCollection->EnsureStyleRuleFor(aRefreshTime, aFlags);
  QueueEvents(aCollection, mPendingEvents);
}

void
nsAnimationManager::QueueEvents(AnimationPlayerCollection* aCollection,
                                EventArray& aEventsToDispatch)
{
  for (size_t playerIdx = aCollection->mPlayers.Length(); playerIdx-- != 0; ) {
    CSSAnimationPlayer* player =
      aCollection->mPlayers[playerIdx]->AsCSSAnimationPlayer();
    MOZ_ASSERT(player, "Expected a collection of CSS Animation players");
    player->QueueEvents(aEventsToDispatch);
  }
}

void
nsAnimationManager::MaybeUpdateCascadeResults(AnimationPlayerCollection* aCollection)
{
  for (size_t playerIdx = aCollection->mPlayers.Length(); playerIdx-- != 0; ) {
    CSSAnimationPlayer* player =
      aCollection->mPlayers[playerIdx]->AsCSSAnimationPlayer();

    if (player->IsInEffect() != player->mInEffectForCascadeResults) {
      
      mozilla::dom::Element* element = aCollection->GetElementToRestyle();
      if (element) {
        nsIFrame* frame = element->GetPrimaryFrame();
        if (frame) {
          UpdateCascadeResults(frame->StyleContext(), aCollection);
        }
      }

      
      mPresContext->TransitionManager()->
        UpdateCascadeResultsWithAnimations(aCollection);

      return;
    }
  }
}

 size_t
nsAnimationManager::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return CommonAnimationManager::SizeOfExcludingThis(aMallocSizeOf);

  
  
  
}

 size_t
nsAnimationManager::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

nsIStyleRule*
nsAnimationManager::CheckAnimationRule(nsStyleContext* aStyleContext,
                                       mozilla::dom::Element* aElement)
{
  if (!mPresContext->IsDynamic()) {
    
    return nullptr;
  }

  
  
  
  

  const nsStyleDisplay* disp = aStyleContext->StyleDisplay();
  AnimationPlayerCollection* collection =
    GetAnimations(aElement, aStyleContext->GetPseudoType(), false);
  if (!collection &&
      disp->mAnimationNameCount == 1 &&
      disp->mAnimations[0].GetName().IsEmpty()) {
    return nullptr;
  }

  nsAutoAnimationMutationBatch mb(aElement);

  
  dom::DocumentTimeline* timeline = aElement->OwnerDoc()->Timeline();
  AnimationPlayerPtrArray newPlayers;
  if (!aStyleContext->IsInDisplayNoneSubtree()) {
    BuildAnimations(aStyleContext, aElement, timeline, newPlayers);
  }

  if (newPlayers.IsEmpty()) {
    if (collection) {
      
      
      mPresContext->TransitionManager()->
        UpdateCascadeResultsWithAnimationsToBeDestroyed(collection);

      collection->Destroy();
    }
    return nullptr;
  }

  if (collection) {
    collection->mStyleRule = nullptr;
    collection->mStyleRuleRefreshTime = TimeStamp();
    collection->UpdateAnimationGeneration(mPresContext);

    
    
    
    
    
    
    
    
    
    
    if (!collection->mPlayers.IsEmpty()) {

      for (size_t newIdx = newPlayers.Length(); newIdx-- != 0;) {
        AnimationPlayer* newPlayer = newPlayers[newIdx];

        
        
        
        
        
        
        nsRefPtr<CSSAnimationPlayer> oldPlayer;
        size_t oldIdx = collection->mPlayers.Length();
        while (oldIdx-- != 0) {
          CSSAnimationPlayer* a =
            collection->mPlayers[oldIdx]->AsCSSAnimationPlayer();
          MOZ_ASSERT(a, "All players in the CSS Animation collection should"
                        " be CSSAnimationPlayer objects");
          if (a->Name() == newPlayer->Name()) {
            oldPlayer = a;
            break;
          }
        }
        if (!oldPlayer) {
          continue;
        }

        bool animationChanged = false;

        
        
        if (oldPlayer->GetEffect() && newPlayer->GetEffect()) {
          KeyframeEffectReadonly* oldEffect = oldPlayer->GetEffect();
          KeyframeEffectReadonly* newEffect = newPlayer->GetEffect();
          animationChanged =
            oldEffect->Timing() != newEffect->Timing() ||
            oldEffect->Properties() != newEffect->Properties();
          oldEffect->Timing() = newEffect->Timing();
          oldEffect->Properties() = newEffect->Properties();
        }

        
        oldPlayer->ClearIsRunningOnCompositor();

        
        
        
        
        
        
        
        
        if (!oldPlayer->IsStylePaused() && newPlayer->IsPausedOrPausing()) {
          oldPlayer->PauseFromStyle();
          animationChanged = true;
        } else if (oldPlayer->IsStylePaused() &&
                   !newPlayer->IsPausedOrPausing()) {
          oldPlayer->PlayFromStyle();
          animationChanged = true;
        }

        if (animationChanged) {
          nsNodeUtils::AnimationChanged(oldPlayer);
        }

        
        
        
        
        
        
        newPlayer->Cancel();
        newPlayer = nullptr;
        newPlayers.ReplaceElementAt(newIdx, oldPlayer);
        collection->mPlayers.RemoveElementAt(oldIdx);

        
        
        oldPlayer->UpdateRelevance();
      }
    }
  } else {
    collection =
      GetAnimations(aElement, aStyleContext->GetPseudoType(), true);
  }
  collection->mPlayers.SwapElements(newPlayers);
  collection->mNeedsRefreshes = true;
  collection->Tick();

  
  for (size_t newPlayerIdx = newPlayers.Length(); newPlayerIdx-- != 0; ) {
    newPlayers[newPlayerIdx]->Cancel();
  }

  UpdateCascadeResults(aStyleContext, collection);

  TimeStamp refreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();
  UpdateStyleAndEvents(collection, refreshTime,
                       EnsureStyleRule_IsNotThrottled);
  
  
  
  
  if (!mPendingEvents.IsEmpty()) {
    mPresContext->Document()->SetNeedStyleFlush();
  }

  return GetAnimationRule(aElement, aStyleContext->GetPseudoType());
}

struct KeyframeData {
  float mKey;
  uint32_t mIndex; 
  nsCSSKeyframeRule *mRule;
};

struct KeyframeDataComparator {
  bool Equals(const KeyframeData& A, const KeyframeData& B) const {
    return A.mKey == B.mKey && A.mIndex == B.mIndex;
  }
  bool LessThan(const KeyframeData& A, const KeyframeData& B) const {
    return A.mKey < B.mKey || (A.mKey == B.mKey && A.mIndex < B.mIndex);
  }
};

class ResolvedStyleCache {
public:
  ResolvedStyleCache() : mCache() {}
  nsStyleContext* Get(nsPresContext *aPresContext,
                      nsStyleContext *aParentStyleContext,
                      nsCSSKeyframeRule *aKeyframe);

private:
  nsRefPtrHashtable<nsPtrHashKey<nsCSSKeyframeRule>, nsStyleContext> mCache;
};

nsStyleContext*
ResolvedStyleCache::Get(nsPresContext *aPresContext,
                        nsStyleContext *aParentStyleContext,
                        nsCSSKeyframeRule *aKeyframe)
{
  
  
  
  
  
  
  
  
  nsStyleContext *result = mCache.GetWeak(aKeyframe);
  if (!result) {
    nsCOMArray<nsIStyleRule> rules;
    rules.AppendObject(aKeyframe);
    nsRefPtr<nsStyleContext> resultStrong = aPresContext->StyleSet()->
      ResolveStyleByAddingRules(aParentStyleContext, rules);
    mCache.Put(aKeyframe, resultStrong);
    result = resultStrong;
  }
  return result;
}

void
nsAnimationManager::BuildAnimations(nsStyleContext* aStyleContext,
                                    dom::Element* aTarget,
                                    dom::DocumentTimeline* aTimeline,
                                    AnimationPlayerPtrArray& aPlayers)
{
  MOZ_ASSERT(aPlayers.IsEmpty(), "expect empty array");

  ResolvedStyleCache resolvedStyles;

  const nsStyleDisplay *disp = aStyleContext->StyleDisplay();

  nsRefPtr<nsStyleContext> styleWithoutAnimation;

  for (size_t animIdx = 0, animEnd = disp->mAnimationNameCount;
       animIdx != animEnd; ++animIdx) {
    const StyleAnimation& src = disp->mAnimations[animIdx];

    
    
    
    
    
    nsCSSKeyframesRule* rule =
      src.GetName().IsEmpty()
      ? nullptr
      : mPresContext->StyleSet()->KeyframesRuleForName(mPresContext,
                                                       src.GetName());
    if (!rule) {
      continue;
    }

    nsRefPtr<CSSAnimationPlayer> dest = new CSSAnimationPlayer(aTimeline);
    aPlayers.AppendElement(dest);

    AnimationTiming timing;
    timing.mIterationDuration =
      TimeDuration::FromMilliseconds(src.GetDuration());
    timing.mDelay = TimeDuration::FromMilliseconds(src.GetDelay());
    timing.mIterationCount = src.GetIterationCount();
    timing.mDirection = src.GetDirection();
    timing.mFillMode = src.GetFillMode();

    nsRefPtr<KeyframeEffectReadonly> destEffect =
      new KeyframeEffectReadonly(mPresContext->Document(), aTarget,
                                 aStyleContext->GetPseudoType(), timing,
                                 src.GetName());
    dest->SetEffect(destEffect);

    
    
    
    dest->PlayFromStyle();

    if (src.GetPlayState() == NS_STYLE_ANIMATION_PLAY_STATE_PAUSED) {
      dest->PauseFromStyle();
    }

    
    
    
    
    
    

    AutoInfallibleTArray<KeyframeData, 16> sortedKeyframes;

    for (uint32_t ruleIdx = 0, ruleEnd = rule->StyleRuleCount();
         ruleIdx != ruleEnd; ++ruleIdx) {
      css::Rule* cssRule = rule->GetStyleRuleAt(ruleIdx);
      MOZ_ASSERT(cssRule, "must have rule");
      MOZ_ASSERT(cssRule->GetType() == css::Rule::KEYFRAME_RULE,
                 "must be keyframe rule");
      nsCSSKeyframeRule *kfRule = static_cast<nsCSSKeyframeRule*>(cssRule);

      const nsTArray<float> &keys = kfRule->GetKeys();
      for (uint32_t keyIdx = 0, keyEnd = keys.Length();
           keyIdx != keyEnd; ++keyIdx) {
        float key = keys[keyIdx];
        
        
        if (0.0f <= key && key <= 1.0f) {
          KeyframeData *data = sortedKeyframes.AppendElement();
          data->mKey = key;
          data->mIndex = ruleIdx;
          data->mRule = kfRule;
        }
      }
    }

    sortedKeyframes.Sort(KeyframeDataComparator());

    if (sortedKeyframes.Length() == 0) {
      
      continue;
    }

    
    
    nsCSSPropertySet properties;

    for (uint32_t kfIdx = 0, kfEnd = sortedKeyframes.Length();
         kfIdx != kfEnd; ++kfIdx) {
      css::Declaration *decl = sortedKeyframes[kfIdx].mRule->Declaration();
      for (uint32_t propIdx = 0, propEnd = decl->Count();
           propIdx != propEnd; ++propIdx) {
        nsCSSProperty prop = decl->GetPropertyAt(propIdx);
        if (prop != eCSSPropertyExtra_variable) {
          
          properties.AddProperty(prop);
        }
      }
    }

    for (nsCSSProperty prop = nsCSSProperty(0);
         prop < eCSSProperty_COUNT_no_shorthands;
         prop = nsCSSProperty(prop + 1)) {
      if (!properties.HasProperty(prop) ||
          nsCSSProps::kAnimTypeTable[prop] == eStyleAnimType_None) {
        continue;
      }

      
      
      
      
      AutoInfallibleTArray<uint32_t, 16> keyframesWithProperty;
      float lastKey = 100.0f; 
      for (uint32_t kfIdx = 0, kfEnd = sortedKeyframes.Length();
           kfIdx != kfEnd; ++kfIdx) {
        KeyframeData &kf = sortedKeyframes[kfIdx];
        if (!kf.mRule->Declaration()->HasProperty(prop)) {
          continue;
        }
        if (kf.mKey == lastKey) {
          
          keyframesWithProperty[keyframesWithProperty.Length() - 1] = kfIdx;
        } else {
          keyframesWithProperty.AppendElement(kfIdx);
        }
        lastKey = kf.mKey;
      }

      AnimationProperty &propData = *destEffect->Properties().AppendElement();
      propData.mProperty = prop;
      propData.mWinsInCascade = true;

      KeyframeData *fromKeyframe = nullptr;
      nsRefPtr<nsStyleContext> fromContext;
      bool interpolated = true;
      for (uint32_t wpIdx = 0, wpEnd = keyframesWithProperty.Length();
           wpIdx != wpEnd; ++wpIdx) {
        uint32_t kfIdx = keyframesWithProperty[wpIdx];
        KeyframeData &toKeyframe = sortedKeyframes[kfIdx];

        nsRefPtr<nsStyleContext> toContext =
          resolvedStyles.Get(mPresContext, aStyleContext, toKeyframe.mRule);

        if (fromKeyframe) {
          interpolated = interpolated &&
            BuildSegment(propData.mSegments, prop, src,
                         fromKeyframe->mKey, fromContext,
                         fromKeyframe->mRule->Declaration(),
                         toKeyframe.mKey, toContext);
        } else {
          if (toKeyframe.mKey != 0.0f) {
            
            
            if (!styleWithoutAnimation) {
              styleWithoutAnimation = mPresContext->StyleSet()->
                ResolveStyleWithoutAnimation(aTarget, aStyleContext,
                                             eRestyle_AllHintsWithAnimations);
            }
            interpolated = interpolated &&
              BuildSegment(propData.mSegments, prop, src,
                           0.0f, styleWithoutAnimation, nullptr,
                           toKeyframe.mKey, toContext);
          }
        }

        fromContext = toContext;
        fromKeyframe = &toKeyframe;
      }

      if (fromKeyframe->mKey != 1.0f) {
        
        
        if (!styleWithoutAnimation) {
          styleWithoutAnimation = mPresContext->StyleSet()->
            ResolveStyleWithoutAnimation(aTarget, aStyleContext,
                                         eRestyle_AllHintsWithAnimations);
        }
        interpolated = interpolated &&
          BuildSegment(propData.mSegments, prop, src,
                       fromKeyframe->mKey, fromContext,
                       fromKeyframe->mRule->Declaration(),
                       1.0f, styleWithoutAnimation);
      }

      
      
      
      
      
      
      if (!interpolated) {
        destEffect->Properties().RemoveElementAt(
          destEffect->Properties().Length() - 1);
      }
    }
  }
}

bool
nsAnimationManager::BuildSegment(InfallibleTArray<AnimationPropertySegment>&
                                   aSegments,
                                 nsCSSProperty aProperty,
                                 const StyleAnimation& aAnimation,
                                 float aFromKey, nsStyleContext* aFromContext,
                                 mozilla::css::Declaration* aFromDeclaration,
                                 float aToKey, nsStyleContext* aToContext)
{
  StyleAnimationValue fromValue, toValue, dummyValue;
  if (!ExtractComputedValueForTransition(aProperty, aFromContext, fromValue) ||
      !ExtractComputedValueForTransition(aProperty, aToContext, toValue) ||
      
      
      
      !StyleAnimationValue::Interpolate(aProperty, fromValue, toValue,
                                        0.5, dummyValue)) {
    return false;
  }

  AnimationPropertySegment &segment = *aSegments.AppendElement();

  segment.mFromValue = fromValue;
  segment.mToValue = toValue;
  segment.mFromKey = aFromKey;
  segment.mToKey = aToKey;
  const nsTimingFunction *tf;
  if (aFromDeclaration &&
      aFromDeclaration->HasProperty(eCSSProperty_animation_timing_function)) {
    tf = &aFromContext->StyleDisplay()->mAnimations[0].GetTimingFunction();
  } else {
    tf = &aAnimation.GetTimingFunction();
  }
  segment.mTimingFunction.Init(*tf);

  return true;
}

 void
nsAnimationManager::UpdateCascadeResults(
                      nsStyleContext* aStyleContext,
                      AnimationPlayerCollection* aElementAnimations)
{
  



  
  
  nsAutoTArray<nsCSSProperty, 2> propertiesToTrack;

  {
    nsCSSPropertySet propertiesToTrackAsSet;

    for (size_t playerIdx = aElementAnimations->mPlayers.Length();
         playerIdx-- != 0; ) {
      const AnimationPlayer* player = aElementAnimations->mPlayers[playerIdx];
      const KeyframeEffectReadonly* effect = player->GetEffect();
      if (!effect) {
        continue;
      }

      for (size_t propIdx = 0, propEnd = effect->Properties().Length();
           propIdx != propEnd; ++propIdx) {
        const AnimationProperty& prop = effect->Properties()[propIdx];
        
        
        if (nsCSSProps::PropHasFlags(prop.mProperty,
                                     CSS_PROPERTY_CAN_ANIMATE_ON_COMPOSITOR)) {
          if (!propertiesToTrackAsSet.HasProperty(prop.mProperty)) {
            propertiesToTrack.AppendElement(prop.mProperty);
            propertiesToTrackAsSet.AddProperty(prop.mProperty);
          }
        }
      }
    }
  }

  




  nsCSSPropertySet propertiesOverridden;
  nsRuleNode::ComputePropertiesOverridingAnimation(propertiesToTrack,
                                                   aStyleContext,
                                                   propertiesOverridden);

  












  bool changed = false;
  for (size_t playerIdx = aElementAnimations->mPlayers.Length();
       playerIdx-- != 0; ) {
    CSSAnimationPlayer* player =
      aElementAnimations->mPlayers[playerIdx]->AsCSSAnimationPlayer();
    KeyframeEffectReadonly* effect = player->GetEffect();

    player->mInEffectForCascadeResults = player->IsInEffect();

    if (!effect) {
      continue;
    }

    for (size_t propIdx = 0, propEnd = effect->Properties().Length();
         propIdx != propEnd; ++propIdx) {
      AnimationProperty& prop = effect->Properties()[propIdx];
      
      
      if (nsCSSProps::PropHasFlags(prop.mProperty,
                                   CSS_PROPERTY_CAN_ANIMATE_ON_COMPOSITOR)) {
        bool newWinsInCascade =
          !propertiesOverridden.HasProperty(prop.mProperty);
        if (newWinsInCascade != prop.mWinsInCascade) {
          changed = true;
        }
        prop.mWinsInCascade = newWinsInCascade;

        if (prop.mWinsInCascade && player->mInEffectForCascadeResults) {
          
          
          
          
          propertiesOverridden.AddProperty(prop.mProperty);
        }
      }
    }
  }

  if (changed) {
    nsPresContext* presContext = aElementAnimations->mManager->PresContext();
    presContext->RestyleManager()->IncrementAnimationGeneration();
    aElementAnimations->UpdateAnimationGeneration(presContext);
    aElementAnimations->PostUpdateLayerAnimations();

    
    aElementAnimations->mNeedsRefreshes = true;
    aElementAnimations->mStyleRuleRefreshTime = TimeStamp();
  }
}

 void
nsAnimationManager::WillRefresh(mozilla::TimeStamp aTime)
{
  MOZ_ASSERT(mPresContext,
             "refresh driver should not notify additional observers "
             "after pres context has been destroyed");
  if (!mPresContext->GetPresShell()) {
    
    
    
    
    RemoveAllElementCollections();
    return;
  }

  FlushAnimations(Can_Throttle);
}

void
nsAnimationManager::FlushAnimations(FlushFlags aFlags)
{
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
  bool didThrottle = false;
  for (PRCList *l = PR_LIST_HEAD(&mElementCollections);
       l != &mElementCollections;
       l = PR_NEXT_LINK(l)) {
    AnimationPlayerCollection* collection =
      static_cast<AnimationPlayerCollection*>(l);

    nsAutoAnimationMutationBatch mb(collection->mElement);

    collection->Tick();
    bool canThrottleTick = aFlags == Can_Throttle &&
      collection->CanPerformOnCompositorThread(
        AnimationPlayerCollection::CanAnimateFlags(0)) &&
      collection->CanThrottleAnimation(now);

    nsRefPtr<css::AnimValuesStyleRule> oldStyleRule = collection->mStyleRule;
    UpdateStyleAndEvents(collection, now, canThrottleTick
                                          ? EnsureStyleRule_IsThrottled
                                          : EnsureStyleRule_IsNotThrottled);
    if (oldStyleRule != collection->mStyleRule) {
      collection->PostRestyleForAnimation(mPresContext);
    } else {
      didThrottle = true;
    }
  }

  if (didThrottle) {
    mPresContext->Document()->SetNeedStyleFlush();
  }

  MaybeStartOrStopObservingRefreshDriver();

  DispatchEvents(); 
}

void
nsAnimationManager::DoDispatchEvents()
{
  EventArray events;
  mPendingEvents.SwapElements(events);
  for (uint32_t i = 0, i_end = events.Length(); i < i_end; ++i) {
    AnimationEventInfo &info = events[i];
    EventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}

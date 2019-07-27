




#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/dom/AnimationPlayer.h"

#include "nsPresContext.h"
#include "nsRuleProcessorData.h"
#include "nsStyleSet.h"
#include "nsStyleChangeList.h"
#include "nsCSSRules.h"
#include "RestyleManager.h"
#include "nsLayoutUtils.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include <math.h>

using namespace mozilla;
using namespace mozilla::css;
using mozilla::dom::Animation;
using mozilla::dom::AnimationPlayer;

void
nsAnimationManager::UpdateStyleAndEvents(AnimationPlayerCollection*
                                           aCollection,
                                         TimeStamp aRefreshTime,
                                         EnsureStyleRuleFlags aFlags)
{
  aCollection->EnsureStyleRuleFor(aRefreshTime, aFlags);
  GetEventsForCurrentTime(aCollection, mPendingEvents);
  CheckNeedsRefresh();
}

void
nsAnimationManager::GetEventsForCurrentTime(AnimationPlayerCollection*
                                              aCollection,
                                            EventArray& aEventsToDispatch)
{
  for (size_t playerIdx = aCollection->mPlayers.Length(); playerIdx-- != 0; ) {
    AnimationPlayer* player = aCollection->mPlayers[playerIdx];
    Animation* anim = player->GetSource();
    if (!anim) {
      continue;
    }

    ComputedTiming computedTiming = anim->GetComputedTiming();

    switch (computedTiming.mPhase) {
      case ComputedTiming::AnimationPhase_Null:
      case ComputedTiming::AnimationPhase_Before:
        
        break;

      case ComputedTiming::AnimationPhase_Active:
        
        if (computedTiming.mCurrentIteration != anim->LastNotification()) {
          
          
          
          
          
          
          uint32_t message =
            anim->LastNotification() == Animation::LAST_NOTIFICATION_NONE
                                        ? NS_ANIMATION_START
                                        : NS_ANIMATION_ITERATION;
          anim->SetLastNotification(computedTiming.mCurrentIteration);
          TimeDuration iterationStart =
            anim->Timing().mIterationDuration *
            computedTiming.mCurrentIteration;
          TimeDuration elapsedTime =
            std::max(iterationStart, anim->InitialAdvance());
          AnimationEventInfo ei(aCollection->mElement, player->Name(), message,
                                elapsedTime, aCollection->PseudoElement());
          aEventsToDispatch.AppendElement(ei);
        }
        break;

      case ComputedTiming::AnimationPhase_After:
        
        
        if (anim->LastNotification() == Animation::LAST_NOTIFICATION_NONE) {
          
          
          
          anim->SetLastNotification(0);
          TimeDuration elapsedTime =
            std::min(anim->InitialAdvance(), computedTiming.mActiveDuration);
          AnimationEventInfo ei(aCollection->mElement,
                                player->Name(), NS_ANIMATION_START,
                                elapsedTime, aCollection->PseudoElement());
          aEventsToDispatch.AppendElement(ei);
        }
        
        if (anim->LastNotification() != Animation::LAST_NOTIFICATION_END) {
          anim->SetLastNotification(Animation::LAST_NOTIFICATION_END);
          AnimationEventInfo ei(aCollection->mElement,
                                player->Name(), NS_ANIMATION_END,
                                computedTiming.mActiveDuration,
                                aCollection->PseudoElement());
          aEventsToDispatch.AppendElement(ei);
        }
        break;
    }
  }
}

AnimationPlayerCollection*
nsAnimationManager::GetAnimationPlayers(dom::Element *aElement,
                                        nsCSSPseudoElements::Type aPseudoType,
                                        bool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementCollections)) {
    
    return nullptr;
  }

  nsIAtom *propName;
  if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    propName = nsGkAtoms::animationsProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_before) {
    propName = nsGkAtoms::animationsOfBeforeProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_after) {
    propName = nsGkAtoms::animationsOfAfterProperty;
  } else {
    NS_ASSERTION(!aCreateIfNeeded,
                 "should never try to create transitions for pseudo "
                 "other than :before or :after");
    return nullptr;
  }
  AnimationPlayerCollection* collection =
    static_cast<AnimationPlayerCollection*>(aElement->GetProperty(propName));
  if (!collection && aCreateIfNeeded) {
    
    collection =
      new AnimationPlayerCollection(aElement, propName, this,
        mPresContext->RefreshDriver()->MostRecentRefresh());
    nsresult rv =
      aElement->SetProperty(propName, collection,
                            &AnimationPlayerCollection::PropertyDtor, false);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete collection;
      return nullptr;
    }
    if (propName == nsGkAtoms::animationsProperty) {
      aElement->SetMayHaveAnimations();
    }

    AddElementCollection(collection);
  }

  return collection;
}

 void
nsAnimationManager::RulesMatching(ElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");
  nsIStyleRule *rule =
    GetAnimationRule(aData->mElement,
                     nsCSSPseudoElements::ePseudo_NotPseudoElement);
  if (rule) {
    aData->mRuleWalker->Forward(rule);
  }
}

 void
nsAnimationManager::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");
  if (aData->mPseudoType != nsCSSPseudoElements::ePseudo_before &&
      aData->mPseudoType != nsCSSPseudoElements::ePseudo_after) {
    return;
  }

  
  
  
  nsIStyleRule *rule = GetAnimationRule(aData->mElement, aData->mPseudoType);
  if (rule) {
    aData->mRuleWalker->Forward(rule);
  }
}

 void
nsAnimationManager::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
nsAnimationManager::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif

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
  if (!mPresContext->IsProcessingAnimationStyleChange()) {
    if (!mPresContext->IsDynamic()) {
      
      return nullptr;
    }

    
    
    
    

    const nsStyleDisplay* disp = aStyleContext->StyleDisplay();
    AnimationPlayerCollection* collection =
      GetAnimationPlayers(aElement, aStyleContext->GetPseudoType(), false);
    if (!collection &&
        disp->mAnimationNameCount == 1 &&
        disp->mAnimations[0].GetName().IsEmpty()) {
      return nullptr;
    }

    
    dom::AnimationTimeline* timeline = aElement->OwnerDoc()->Timeline();
    AnimationPlayerPtrArray newPlayers;
    BuildAnimations(aStyleContext, timeline, newPlayers);

    if (newPlayers.IsEmpty()) {
      if (collection) {
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

          
          
          
          
          
          
          nsRefPtr<AnimationPlayer> oldPlayer;
          size_t oldIdx = collection->mPlayers.Length();
          while (oldIdx-- != 0) {
            AnimationPlayer* a = collection->mPlayers[oldIdx];
            if (a->Name() == newPlayer->Name()) {
              oldPlayer = a;
              break;
            }
          }
          if (!oldPlayer) {
            continue;
          }

          
          
          if (oldPlayer->GetSource() && newPlayer->GetSource()) {
            Animation* oldAnim = oldPlayer->GetSource();
            Animation* newAnim = newPlayer->GetSource();
            oldAnim->Timing() = newAnim->Timing();
            oldAnim->Properties() = newAnim->Properties();
          }

          
          oldPlayer->mIsRunningOnCompositor = false;

          
          if (!oldPlayer->IsPaused() && newPlayer->IsPaused()) {
            
            oldPlayer->mHoldTime = timeline->GetCurrentTimeStamp();
          } else if (oldPlayer->IsPaused() && !newPlayer->IsPaused()) {
            const TimeStamp& now = timeline->GetCurrentTimeStamp();
            if (!now.IsNull()) {
              
              
              
              
              
              oldPlayer->mStartTime.SetValue(now + oldPlayer->mStartTime.Value() - oldPlayer->mHoldTime);
            }
            oldPlayer->mHoldTime = TimeStamp();
          }
          oldPlayer->mPlayState = newPlayer->mPlayState;

          
          
          
          
          
          
          newPlayer = nullptr;
          newPlayers.ReplaceElementAt(newIdx, oldPlayer);
          collection->mPlayers.RemoveElementAt(oldIdx);
        }
      }
    } else {
      collection =
        GetAnimationPlayers(aElement, aStyleContext->GetPseudoType(), true);
    }
    collection->mPlayers.SwapElements(newPlayers);
    collection->mNeedsRefreshes = true;
    collection->Tick();

    TimeStamp refreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();
    UpdateStyleAndEvents(collection, refreshTime,
                         EnsureStyleRule_IsNotThrottled);
    
    
    
    
    if (!mPendingEvents.IsEmpty()) {
      mPresContext->Document()->SetNeedStyleFlush();
    }
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
                                    dom::AnimationTimeline* aTimeline,
                                    AnimationPlayerPtrArray& aPlayers)
{
  NS_ABORT_IF_FALSE(aPlayers.IsEmpty(), "expect empty array");

  ResolvedStyleCache resolvedStyles;

  const nsStyleDisplay *disp = aStyleContext->StyleDisplay();
  TimeStamp now = aTimeline->GetCurrentTimeStamp();

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

    nsRefPtr<AnimationPlayer> dest =
      *aPlayers.AppendElement(new AnimationPlayer(aTimeline));

    AnimationTiming timing;
    timing.mIterationDuration =
      TimeDuration::FromMilliseconds(src.GetDuration());
    timing.mDelay = TimeDuration::FromMilliseconds(src.GetDelay());
    timing.mIterationCount = src.GetIterationCount();
    timing.mDirection = src.GetDirection();
    timing.mFillMode = src.GetFillMode();

    nsRefPtr<Animation> destAnim =
      new Animation(mPresContext->Document(), timing, src.GetName());
    dest->SetSource(destAnim);

    dest->mStartTime = aTimeline->GetCurrentTimeDuration();
    dest->mPlayState = src.GetPlayState();
    if (dest->IsPaused()) {
      dest->mHoldTime = now;
    } else {
      dest->mHoldTime = TimeStamp();
    }

    
    
    
    
    
    

    AutoInfallibleTArray<KeyframeData, 16> sortedKeyframes;

    for (uint32_t ruleIdx = 0, ruleEnd = rule->StyleRuleCount();
         ruleIdx != ruleEnd; ++ruleIdx) {
      css::Rule* cssRule = rule->GetStyleRuleAt(ruleIdx);
      NS_ABORT_IF_FALSE(cssRule, "must have rule");
      NS_ABORT_IF_FALSE(cssRule->GetType() == css::Rule::KEYFRAME_RULE,
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

      AnimationProperty &propData = *destAnim->Properties().AppendElement();
      propData.mProperty = prop;

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
            
            
            interpolated = interpolated &&
              BuildSegment(propData.mSegments, prop, src,
                           0.0f, aStyleContext, nullptr,
                           toKeyframe.mKey, toContext);
          }
        }

        fromContext = toContext;
        fromKeyframe = &toKeyframe;
      }

      if (fromKeyframe->mKey != 1.0f) {
        
        
        interpolated = interpolated &&
          BuildSegment(propData.mSegments, prop, src,
                       fromKeyframe->mKey, fromContext,
                       fromKeyframe->mRule->Declaration(),
                       1.0f, aStyleContext);
      }

      
      
      
      
      
      
      if (!interpolated) {
        destAnim->Properties().RemoveElementAt(
          destAnim->Properties().Length() - 1);
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

nsIStyleRule*
nsAnimationManager::GetAnimationRule(mozilla::dom::Element* aElement,
                                     nsCSSPseudoElements::Type aPseudoType)
{
  NS_ABORT_IF_FALSE(
    aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
    aPseudoType == nsCSSPseudoElements::ePseudo_before ||
    aPseudoType == nsCSSPseudoElements::ePseudo_after,
    "forbidden pseudo type");

  if (!mPresContext->IsDynamic()) {
    
    return nullptr;
  }

  AnimationPlayerCollection* collection =
    GetAnimationPlayers(aElement, aPseudoType, false);
  if (!collection) {
    return nullptr;
  }

  if (mPresContext->IsProcessingRestyles() &&
      !mPresContext->IsProcessingAnimationStyleChange()) {
    
    

    if (collection->mStyleRule) {
      collection->PostRestyleForAnimation(mPresContext);
    }

    return nullptr;
  }

  NS_WARN_IF_FALSE(!collection->mNeedsRefreshes ||
                   collection->mStyleRuleRefreshTime ==
                     mPresContext->RefreshDriver()->MostRecentRefresh(),
                   "should already have refreshed style rule");

  return collection->mStyleRule;
}

 void
nsAnimationManager::WillRefresh(mozilla::TimeStamp aTime)
{
  NS_ABORT_IF_FALSE(mPresContext,
                    "refresh driver should not notify additional observers "
                    "after pres context has been destroyed");
  if (!mPresContext->GetPresShell()) {
    
    
    
    
    RemoveAllElementCollections();
    return;
  }

  FlushAnimations(Can_Throttle);
}

void
nsAnimationManager::AddElementCollection(
  AnimationPlayerCollection* aCollection)
{
  if (!mObservingRefreshDriver) {
    NS_ASSERTION(
      static_cast<AnimationPlayerCollection*>(aCollection)->mNeedsRefreshes,
      "Added data which doesn't need refreshing?");
    
    mPresContext->RefreshDriver()->AddRefreshObserver(this, Flush_Style);
    mObservingRefreshDriver = true;
  }

  PR_INSERT_BEFORE(aCollection, &mElementCollections);
}

void
nsAnimationManager::CheckNeedsRefresh()
{
  for (PRCList *l = PR_LIST_HEAD(&mElementCollections);
       l != &mElementCollections;
       l = PR_NEXT_LINK(l)) {
    if (static_cast<AnimationPlayerCollection*>(l)->mNeedsRefreshes) {
      if (!mObservingRefreshDriver) {
        mPresContext->RefreshDriver()->AddRefreshObserver(this, Flush_Style);
        mObservingRefreshDriver = true;
      }
      return;
    }
  }
  if (mObservingRefreshDriver) {
    mObservingRefreshDriver = false;
    mPresContext->RefreshDriver()->RemoveRefreshObserver(this, Flush_Style);
  }
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

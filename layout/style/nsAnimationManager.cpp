




#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StyleAnimationValue.h"

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

void
nsAnimationManager::UpdateStyleAndEvents(ElementAnimationCollection* aEA,
                                         TimeStamp aRefreshTime,
                                         EnsureStyleRuleFlags aFlags)
{
  aEA->EnsureStyleRuleFor(aRefreshTime, aFlags);
  GetEventsAt(aEA, aRefreshTime, mPendingEvents);
  CheckNeedsRefresh();
}

void
nsAnimationManager::GetEventsAt(ElementAnimationCollection* aEA,
                                TimeStamp aRefreshTime,
                                EventArray& aEventsToDispatch)
{
  for (uint32_t animIdx = aEA->mAnimations.Length(); animIdx-- != 0; ) {
    ElementAnimation* anim = aEA->mAnimations[animIdx];

    TimeDuration localTime = anim->GetLocalTimeAt(aRefreshTime);
    ComputedTiming computedTiming =
      ElementAnimation::GetComputedTimingAt(localTime, anim->mTiming);

    switch (computedTiming.mPhase) {
      case ComputedTiming::AnimationPhase_Before:
        
        break;

      case ComputedTiming::AnimationPhase_Active:
        
        if (computedTiming.mCurrentIteration != anim->mLastNotification) {
          
          
          
          
          
          
          uint32_t message =
            anim->mLastNotification == ElementAnimation::LAST_NOTIFICATION_NONE
              ? NS_ANIMATION_START : NS_ANIMATION_ITERATION;

          anim->mLastNotification = computedTiming.mCurrentIteration;
          TimeDuration iterationStart =
            anim->mTiming.mIterationDuration *
            computedTiming.mCurrentIteration;
          TimeDuration elapsedTime =
            std::max(iterationStart, anim->InitialAdvance());
          AnimationEventInfo ei(aEA->mElement, anim->mName, message,
                                elapsedTime, aEA->PseudoElement());
          aEventsToDispatch.AppendElement(ei);
        }
        break;

      case ComputedTiming::AnimationPhase_After:
        
        
        if (anim->mLastNotification ==
            ElementAnimation::LAST_NOTIFICATION_NONE) {
          
          
          
          anim->mLastNotification = 0;
          TimeDuration elapsedTime =
            std::min(anim->InitialAdvance(), computedTiming.mActiveDuration);
          AnimationEventInfo ei(aEA->mElement, anim->mName, NS_ANIMATION_START,
                                elapsedTime, aEA->PseudoElement());
          aEventsToDispatch.AppendElement(ei);
        }
        
        if (anim->mLastNotification !=
            ElementAnimation::LAST_NOTIFICATION_END) {
          anim->mLastNotification = ElementAnimation::LAST_NOTIFICATION_END;
          AnimationEventInfo ei(aEA->mElement, anim->mName, NS_ANIMATION_END,
                                computedTiming.mActiveDuration,
                                aEA->PseudoElement());
          aEventsToDispatch.AppendElement(ei);
        }
        break;
    }
  }
}

ElementAnimationCollection*
nsAnimationManager::GetElementAnimations(dom::Element *aElement,
                                         nsCSSPseudoElements::Type aPseudoType,
                                         bool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementData)) {
    
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
  ElementAnimationCollection *ea = static_cast<ElementAnimationCollection*>(
                                   aElement->GetProperty(propName));
  if (!ea && aCreateIfNeeded) {
    
    ea = new ElementAnimationCollection(aElement, propName, this,
           mPresContext->RefreshDriver()->MostRecentRefresh());
    nsresult rv =
      aElement->SetProperty(propName, ea,
                            &ElementAnimationCollection::PropertyDtor, false);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete ea;
      return nullptr;
    }
    if (propName == nsGkAtoms::animationsProperty) {
      aElement->SetMayHaveAnimations();
    }

    AddElementData(ea);
  }

  return ea;
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

    
    
    
    

    const nsStyleDisplay *disp = aStyleContext->StyleDisplay();
    ElementAnimationCollection *ea =
      GetElementAnimations(aElement, aStyleContext->GetPseudoType(), false);
    if (!ea &&
        disp->mAnimationNameCount == 1 &&
        disp->mAnimations[0].GetName().IsEmpty()) {
      return nullptr;
    }

    
    ElementAnimationPtrArray newAnimations;
    BuildAnimations(aStyleContext, newAnimations);

    if (newAnimations.IsEmpty()) {
      if (ea) {
        ea->Destroy();
      }
      return nullptr;
    }

    TimeStamp refreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();

    if (ea) {
      ea->mStyleRule = nullptr;
      ea->mStyleRuleRefreshTime = TimeStamp();
      ea->UpdateAnimationGeneration(mPresContext);

      
      
      
      
      
      
      
      
      
      
      if (!ea->mAnimations.IsEmpty()) {
        for (uint32_t newIdx = 0, newEnd = newAnimations.Length();
             newIdx != newEnd; ++newIdx) {
          nsRefPtr<ElementAnimation> newAnim = newAnimations[newIdx];

          
          
          
          
          
          
          
          
          const ElementAnimation* oldAnim = nullptr;
          for (uint32_t oldIdx = ea->mAnimations.Length(); oldIdx-- != 0; ) {
            const ElementAnimation* a = ea->mAnimations[oldIdx];
            if (a->mName == newAnim->mName) {
              oldAnim = a;
              break;
            }
          }
          if (!oldAnim) {
            continue;
          }

          newAnim->mStartTime = oldAnim->mStartTime;
          newAnim->mLastNotification = oldAnim->mLastNotification;

          if (oldAnim->IsPaused()) {
            if (newAnim->IsPaused()) {
              
              newAnim->mPauseStart = oldAnim->mPauseStart;
            } else {
              
              
              newAnim->mStartTime += refreshTime - oldAnim->mPauseStart;
            }
          }
        }
      }
    } else {
      ea = GetElementAnimations(aElement, aStyleContext->GetPseudoType(),
                                true);
    }
    ea->mAnimations.SwapElements(newAnimations);
    ea->mNeedsRefreshes = true;

    UpdateStyleAndEvents(ea, refreshTime, EnsureStyleRule_IsNotThrottled);
    
    
    
    
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
  ResolvedStyleCache() : mCache(16) {}
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
                                    ElementAnimationPtrArray& aAnimations)
{
  NS_ABORT_IF_FALSE(aAnimations.IsEmpty(), "expect empty array");

  ResolvedStyleCache resolvedStyles;

  const nsStyleDisplay *disp = aStyleContext->StyleDisplay();
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
  for (uint32_t animIdx = 0, animEnd = disp->mAnimationNameCount;
       animIdx != animEnd; ++animIdx) {
    const StyleAnimation& src = disp->mAnimations[animIdx];
    nsRefPtr<ElementAnimation> dest =
      *aAnimations.AppendElement(new ElementAnimation());

    dest->mName = src.GetName();

    dest->mTiming.mIterationDuration =
      TimeDuration::FromMilliseconds(src.GetDuration());
    dest->mTiming.mDelay = TimeDuration::FromMilliseconds(src.GetDelay());
    dest->mTiming.mIterationCount = src.GetIterationCount();
    dest->mTiming.mDirection = src.GetDirection();
    dest->mTiming.mFillMode = src.GetFillMode();

    dest->mStartTime = now;
    dest->mPlayState = src.GetPlayState();
    if (dest->IsPaused()) {
      dest->mPauseStart = now;
    } else {
      dest->mPauseStart = TimeStamp();
    }

    nsCSSKeyframesRule* rule =
      mPresContext->StyleSet()->KeyframesRuleForName(mPresContext,
                                                     dest->mName);
    if (!rule) {
      
      continue;
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

      AnimationProperty &propData = *dest->mProperties.AppendElement();
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
        dest->mProperties.RemoveElementAt(dest->mProperties.Length() - 1);
      }
    }

    aAnimations.AppendElement(dest);
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

  ElementAnimationCollection *ea =
    GetElementAnimations(aElement, aPseudoType, false);
  if (!ea) {
    return nullptr;
  }

  if (mPresContext->IsProcessingRestyles() &&
      !mPresContext->IsProcessingAnimationStyleChange()) {
    
    

    if (ea->mStyleRule) {
      ea->PostRestyleForAnimation(mPresContext);
    }

    return nullptr;
  }

  NS_WARN_IF_FALSE(!ea->mNeedsRefreshes ||
                   ea->mStyleRuleRefreshTime ==
                     mPresContext->RefreshDriver()->MostRecentRefresh(),
                   "should already have refreshed style rule");

  return ea->mStyleRule;
}

 void
nsAnimationManager::WillRefresh(mozilla::TimeStamp aTime)
{
  NS_ABORT_IF_FALSE(mPresContext,
                    "refresh driver should not notify additional observers "
                    "after pres context has been destroyed");
  if (!mPresContext->GetPresShell()) {
    
    
    
    
    RemoveAllElementData();
    return;
  }

  FlushAnimations(Can_Throttle);
}

void
nsAnimationManager::AddElementData(ElementAnimationCollection* aData)
{
  if (!mObservingRefreshDriver) {
    NS_ASSERTION(
      static_cast<ElementAnimationCollection*>(aData)->mNeedsRefreshes,
      "Added data which doesn't need refreshing?");
    
    mPresContext->RefreshDriver()->AddRefreshObserver(this, Flush_Style);
    mObservingRefreshDriver = true;
  }

  PR_INSERT_BEFORE(aData, &mElementData);
}

void
nsAnimationManager::CheckNeedsRefresh()
{
  for (PRCList *l = PR_LIST_HEAD(&mElementData); l != &mElementData;
       l = PR_NEXT_LINK(l)) {
    if (static_cast<ElementAnimationCollection*>(l)->mNeedsRefreshes) {
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
  for (PRCList *l = PR_LIST_HEAD(&mElementData); l != &mElementData;
       l = PR_NEXT_LINK(l)) {
    ElementAnimationCollection *ea =
      static_cast<ElementAnimationCollection*>(l);
    bool canThrottleTick = aFlags == Can_Throttle &&
      ea->CanPerformOnCompositorThread(
        ElementAnimationCollection::CanAnimateFlags(0)) &&
      ea->CanThrottleAnimation(now);

    nsRefPtr<css::AnimValuesStyleRule> oldStyleRule = ea->mStyleRule;
    UpdateStyleAndEvents(ea, now, canThrottleTick
                                  ? EnsureStyleRule_IsThrottled
                                  : EnsureStyleRule_IsNotThrottled);
    if (oldStyleRule != ea->mStyleRule) {
      ea->PostRestyleForAnimation(mPresContext);
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

void
nsAnimationManager::UpdateThrottledStylesForSubtree(nsIContent* aContent,
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

  ElementAnimationCollection* ea;
  if (element &&
      (ea = GetElementAnimations(element,
                                 nsCSSPseudoElements::ePseudo_NotPseudoElement,
                                 false))) {
    
    newStyle = UpdateThrottledStyle(element, aParentStyle, aChangeList);
    
    ea->mFlushGeneration = mPresContext->RefreshDriver()->MostRecentRefresh();
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

IMPL_UPDATE_ALL_THROTTLED_STYLES_INTERNAL(nsAnimationManager,
                                          GetElementAnimations)

void
nsAnimationManager::UpdateAllThrottledStyles()
{
  if (PR_CLIST_IS_EMPTY(&mElementData)) {
    
    mPresContext->TickLastUpdateThrottledAnimationStyle();
    return;
  }

  if (mPresContext->ThrottledAnimationStyleIsUpToDate()) {
    
    return;
  }

  mPresContext->TickLastUpdateThrottledAnimationStyle();

  UpdateAllThrottledStylesInternal();
}


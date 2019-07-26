




#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/MemoryReporting.h"

#include "nsPresContext.h"
#include "nsRuleProcessorData.h"
#include "nsStyleSet.h"
#include "nsStyleChangeList.h"
#include "nsCSSRules.h"
#include "RestyleManager.h"
#include "nsStyleAnimation.h"
#include "nsLayoutUtils.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "ActiveLayerTracker.h"
#include <math.h>

using namespace mozilla;
using namespace mozilla::css;

ElementAnimations::ElementAnimations(mozilla::dom::Element *aElement,
                                     nsIAtom *aElementProperty,
                                     nsAnimationManager *aAnimationManager,
                                     TimeStamp aNow)
  : CommonElementAnimationData(aElement, aElementProperty,
                               aAnimationManager, aNow),
    mNeedsRefreshes(true)
{
}

static void
ElementAnimationsPropertyDtor(void           *aObject,
                              nsIAtom        *aPropertyName,
                              void           *aPropertyValue,
                              void           *aData)
{
  ElementAnimations *ea = static_cast<ElementAnimations*>(aPropertyValue);
#ifdef DEBUG
  NS_ABORT_IF_FALSE(!ea->mCalledPropertyDtor, "can't call dtor twice");
  ea->mCalledPropertyDtor = true;
#endif
  delete ea;
}

double
ElementAnimations::GetPositionInIteration(TimeDuration aElapsedDuration,
                                          const AnimationTiming& aTiming,
                                          ElementAnimation* aAnimation,
                                          ElementAnimations* aEa,
                                          EventArray* aEventsToDispatch)
{
  MOZ_ASSERT(!aAnimation == !aEa && !aAnimation == !aEventsToDispatch);

  
  
  double currentIterationCount = aElapsedDuration / aTiming.mIterationDuration;
  bool dispatchStartOrIteration = false;
  if (currentIterationCount >= aTiming.mIterationCount) {
    if (aAnimation) {
      
      if (aAnimation->mLastNotification !=
          ElementAnimation::LAST_NOTIFICATION_END) {
        aAnimation->mLastNotification = ElementAnimation::LAST_NOTIFICATION_END;
        AnimationEventInfo ei(aEa->mElement, aAnimation->mName, NS_ANIMATION_END,
                              aElapsedDuration, aEa->PseudoElement());
        aEventsToDispatch->AppendElement(ei);
      }
    }
    if (!aTiming.FillsForwards()) {
      
      return -1;
    }
    currentIterationCount = aTiming.mIterationCount;
  } else if (currentIterationCount < 0.0) {
    if (!aTiming.FillsBackwards()) {
      
      return -1;
    }
    currentIterationCount = 0.0;
  } else {
    dispatchStartOrIteration = aAnimation && !aAnimation->IsPaused();
  }

  
  
  NS_ABORT_IF_FALSE(currentIterationCount >= 0.0, "must be positive");
  double whichIteration = floor(currentIterationCount);
  if (whichIteration == aTiming.mIterationCount && whichIteration != 0.0) {
    
    
    
    whichIteration -= 1.0;
  }
  double positionInIteration = currentIterationCount - whichIteration;

  bool thisIterationReverse = false;
  switch (aTiming.mDirection) {
    case NS_STYLE_ANIMATION_DIRECTION_NORMAL:
      thisIterationReverse = false;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_REVERSE:
      thisIterationReverse = true;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE:
      
      
      
      
      thisIterationReverse = (uint64_t(whichIteration) & 1) == 1;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE_REVERSE:
      
      thisIterationReverse = (uint64_t(whichIteration) & 1) == 0;
      break;
  }
  if (thisIterationReverse) {
    positionInIteration = 1.0 - positionInIteration;
  }

  
  if (aAnimation && dispatchStartOrIteration &&
      whichIteration != aAnimation->mLastNotification) {
    
    
    
    
    
    
    uint32_t message =
      aAnimation->mLastNotification == ElementAnimation::LAST_NOTIFICATION_NONE
        ? NS_ANIMATION_START : NS_ANIMATION_ITERATION;

    aAnimation->mLastNotification = whichIteration;
    AnimationEventInfo ei(aEa->mElement, aAnimation->mName, message,
                          aElapsedDuration, aEa->PseudoElement());
    aEventsToDispatch->AppendElement(ei);
  }

  return positionInIteration;
}

void
ElementAnimations::EnsureStyleRuleFor(TimeStamp aRefreshTime,
                                      bool aIsThrottled)
{
  if (!mNeedsRefreshes) {
    mStyleRuleRefreshTime = aRefreshTime;
    return;
  }

  
  
  
  
  
  
  if (aIsThrottled) {
    for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
      ElementAnimation* anim = mAnimations[animIdx];

      if (anim->mProperties.IsEmpty() ||
          anim->mTiming.mIterationDuration.ToMilliseconds() <= 0.0) {
        continue;
      }

      
      
      AnimationTiming timing = anim->mTiming;
      timing.mFillMode = NS_STYLE_ANIMATION_FILL_MODE_BOTH;
      double positionInIteration =
        GetPositionInIteration(anim->ElapsedDurationAt(aRefreshTime), timing);

      
      
      
      
      
      
      
      
      
      
      if (!anim->mIsRunningOnCompositor ||
          (positionInIteration >= 1.0 &&
           anim->mLastNotification != ElementAnimation::LAST_NOTIFICATION_END))
      {
        aIsThrottled = false;
        break;
      }
    }
  }

  if (aIsThrottled) {
    return;
  }

  
  if (mStyleRuleRefreshTime.IsNull() ||
      mStyleRuleRefreshTime != aRefreshTime) {
    mStyleRuleRefreshTime = aRefreshTime;
    mStyleRule = nullptr;
    
    mNeedsRefreshes = false;

    
    
    
    nsCSSPropertySet properties;

    for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
      ElementAnimation* anim = mAnimations[animIdx];

      if (anim->mProperties.IsEmpty() ||
          anim->mTiming.mIterationDuration.ToMilliseconds() <= 0.0) {
        
        continue;
      }

      
      
      double positionInIteration =
        GetPositionInIteration(anim->ElapsedDurationAt(aRefreshTime),
                               anim->mTiming);
      
      
      
      
      
      if (positionInIteration <= 1 && !anim->IsPaused()) {
        mNeedsRefreshes = true;
      }

      
      
      if (positionInIteration == -1)
        continue;

      NS_ABORT_IF_FALSE(0.0 <= positionInIteration &&
                          positionInIteration <= 1.0,
                        "position should be in [0-1]");

      for (uint32_t propIdx = 0, propEnd = anim->mProperties.Length();
           propIdx != propEnd; ++propIdx)
      {
        const AnimationProperty &prop = anim->mProperties[propIdx];

        NS_ABORT_IF_FALSE(prop.mSegments[0].mFromKey == 0.0,
                          "incorrect first from key");
        NS_ABORT_IF_FALSE(prop.mSegments[prop.mSegments.Length() - 1].mToKey
                            == 1.0,
                          "incorrect last to key");

        if (properties.HasProperty(prop.mProperty)) {
          
          continue;
        }
        properties.AddProperty(prop.mProperty);

        NS_ABORT_IF_FALSE(prop.mSegments.Length() > 0,
                          "property should not be in animations if it "
                          "has no segments");

        
        const AnimationPropertySegment *segment = prop.mSegments.Elements(),
                               *segmentEnd = segment + prop.mSegments.Length();
        while (segment->mToKey < positionInIteration) {
          NS_ABORT_IF_FALSE(segment->mFromKey < segment->mToKey,
                            "incorrect keys");
          ++segment;
          if (segment == segmentEnd) {
            NS_ABORT_IF_FALSE(false, "incorrect positionInIteration");
            break; 
          }
          NS_ABORT_IF_FALSE(segment->mFromKey == (segment-1)->mToKey,
                            "incorrect keys");
        }
        if (segment == segmentEnd) {
          continue;
        }
        NS_ABORT_IF_FALSE(segment->mFromKey < segment->mToKey,
                          "incorrect keys");
        NS_ABORT_IF_FALSE(segment >= prop.mSegments.Elements() &&
                          size_t(segment - prop.mSegments.Elements()) <
                            prop.mSegments.Length(),
                          "out of array bounds");

        if (!mStyleRule) {
          
          mStyleRule = new css::AnimValuesStyleRule();
        }

        double positionInSegment = (positionInIteration - segment->mFromKey) /
                                   (segment->mToKey - segment->mFromKey);
        double valuePosition =
          segment->mTimingFunction.GetValue(positionInSegment);

        nsStyleAnimation::Value *val =
          mStyleRule->AddEmptyValue(prop.mProperty);

#ifdef DEBUG
        bool result =
#endif
          nsStyleAnimation::Interpolate(prop.mProperty,
                                        segment->mFromValue, segment->mToValue,
                                        valuePosition, *val);
        NS_ABORT_IF_FALSE(result, "interpolate must succeed now");
      }
    }
  }
}

void
ElementAnimations::GetEventsAt(TimeStamp aRefreshTime,
                               EventArray& aEventsToDispatch)
{
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    ElementAnimation* anim = mAnimations[animIdx];

    
    
    
    
    if (anim->mProperties.IsEmpty() ||
        anim->mTiming.mIterationDuration.ToMilliseconds() <= 0.0) {
      
      continue;
    }

    GetPositionInIteration(anim->ElapsedDurationAt(aRefreshTime),
                           anim->mTiming, anim, this, &aEventsToDispatch);
  }
}

bool
ElementAnimations::HasAnimationOfProperty(nsCSSProperty aProperty) const
{
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementAnimation* anim = mAnimations[animIdx];
    if (anim->HasAnimationOfProperty(aProperty)) {
      return true;
    }
  }
  return false;
}

bool
ElementAnimations::CanPerformOnCompositorThread(CanAnimateFlags aFlags) const
{
  nsIFrame* frame = nsLayoutUtils::GetStyleFrame(mElement);
  if (!frame) {
    return false;
  }

  if (mElementProperty != nsGkAtoms::animationsProperty) {
    if (nsLayoutUtils::IsAnimationLoggingEnabled()) {
      nsCString message;
      message.AppendLiteral("Gecko bug: Async animation of pseudoelements not supported.  See bug 771367 (");
      message.Append(nsAtomCString(mElementProperty));
      message.Append(')');
      LogAsyncAnimationFailure(message, mElement);
    }
    return false;
  }

  TimeStamp now = frame->PresContext()->RefreshDriver()->MostRecentRefresh();

  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementAnimation* anim = mAnimations[animIdx];
    for (uint32_t propIdx = 0, propEnd = anim->mProperties.Length();
         propIdx != propEnd; ++propIdx) {
      if (IsGeometricProperty(anim->mProperties[propIdx].mProperty) &&
          anim->IsRunningAt(now)) {
        aFlags = CanAnimateFlags(aFlags | CanAnimate_HasGeometricProperty);
        break;
      }
    }
  }

  bool hasOpacity = false;
  bool hasTransform = false;
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementAnimation* anim = mAnimations[animIdx];
    if (!anim->IsRunningAt(now)) {
      continue;
    }

    for (uint32_t propIdx = 0, propEnd = anim->mProperties.Length();
         propIdx != propEnd; ++propIdx) {
      const AnimationProperty& prop = anim->mProperties[propIdx];
      if (!CanAnimatePropertyOnCompositor(mElement,
                                          prop.mProperty,
                                          aFlags) ||
          IsCompositorAnimationDisabledForFrame(frame)) {
        return false;
      }
      if (prop.mProperty == eCSSProperty_opacity) {
        hasOpacity = true;
      } else if (prop.mProperty == eCSSProperty_transform) {
        hasTransform = true;
      }
    }
  }
  
  
  if (hasOpacity) {
    ActiveLayerTracker::NotifyAnimated(frame, eCSSProperty_opacity);
  }
  if (hasTransform) {
    ActiveLayerTracker::NotifyAnimated(frame, eCSSProperty_transform);
  }
  return true;
}

ElementAnimations*
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
  ElementAnimations *ea = static_cast<ElementAnimations*>(
                             aElement->GetProperty(propName));
  if (!ea && aCreateIfNeeded) {
    
    ea = new ElementAnimations(aElement, propName, this,
           mPresContext->RefreshDriver()->MostRecentRefresh());
    nsresult rv = aElement->SetProperty(propName, ea,
                                        ElementAnimationsPropertyDtor, false);
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
nsAnimationManager::EnsureStyleRuleFor(ElementAnimations* aEA)
{
  TimeStamp refreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();
  aEA->EnsureStyleRuleFor(refreshTime, false);
  aEA->GetEventsAt(refreshTime, mPendingEvents);
  CheckNeedsRefresh();
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
    ElementAnimations *ea =
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

    ea->EnsureStyleRuleFor(refreshTime, false);
    ea->GetEventsAt(refreshTime, mPendingEvents);
    CheckNeedsRefresh();
    
    
    
    
    if (!mPendingEvents.IsEmpty()) {
      mPresContext->Document()->SetNeedStyleFlush();
    }
  }

  return GetAnimationRule(aElement, aStyleContext->GetPseudoType());
}

class PercentageHashKey : public PLDHashEntryHdr
{
public:
  typedef const float& KeyType;
  typedef const float* KeyTypePointer;

  PercentageHashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  PercentageHashKey(const PercentageHashKey& toCopy) : mValue(toCopy.mValue) { }
  ~PercentageHashKey() { }

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) {
    static_assert(sizeof(PLDHashNumber) == sizeof(uint32_t),
                  "this hash function assumes PLDHashNumber is uint32_t");
    static_assert(PLDHashNumber(-1) > PLDHashNumber(0),
                  "this hash function assumes PLDHashNumber is uint32_t");
    float key = *aKey;
    NS_ABORT_IF_FALSE(0.0f <= key && key <= 1.0f, "out of range");
    return PLDHashNumber(key * UINT32_MAX);
  }
  enum { ALLOW_MEMMOVE = true };

private:
  const float mValue;
};

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
    const nsAnimation& src = disp->mAnimations[animIdx];
    nsRefPtr<ElementAnimation> dest =
      *aAnimations.AppendElement(new ElementAnimation());

    dest->mName = src.GetName();

    dest->mTiming.mIterationDuration =
      TimeDuration::FromMilliseconds(src.GetDuration());
    dest->mTiming.mIterationCount = src.GetIterationCount();
    dest->mTiming.mDirection = src.GetDirection();
    dest->mTiming.mFillMode = src.GetFillMode();
    dest->mPlayState = src.GetPlayState();

    dest->mDelay = TimeDuration::FromMilliseconds(src.GetDelay());
    dest->mStartTime = now;
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
                                 const nsAnimation& aAnimation,
                                 float aFromKey, nsStyleContext* aFromContext,
                                 mozilla::css::Declaration* aFromDeclaration,
                                 float aToKey, nsStyleContext* aToContext)
{
  nsStyleAnimation::Value fromValue, toValue, dummyValue;
  if (!ExtractComputedValueForTransition(aProperty, aFromContext, fromValue) ||
      !ExtractComputedValueForTransition(aProperty, aToContext, toValue) ||
      
      
      
      !nsStyleAnimation::Interpolate(aProperty, fromValue, toValue,
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

  ElementAnimations *ea =
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
nsAnimationManager::AddElementData(CommonElementAnimationData* aData)
{
  if (!mObservingRefreshDriver) {
    NS_ASSERTION(static_cast<ElementAnimations*>(aData)->mNeedsRefreshes,
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
    if (static_cast<ElementAnimations*>(l)->mNeedsRefreshes) {
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
    ElementAnimations *ea = static_cast<ElementAnimations*>(l);
    bool canThrottleTick = aFlags == Can_Throttle &&
      ea->CanPerformOnCompositorThread(
        CommonElementAnimationData::CanAnimateFlags(0)) &&
      ea->CanThrottleAnimation(now);

    nsRefPtr<css::AnimValuesStyleRule> oldStyleRule = ea->mStyleRule;
    ea->EnsureStyleRuleFor(now, canThrottleTick);
    ea->GetEventsAt(now, mPendingEvents);
    CheckNeedsRefresh();
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

  ElementAnimations* ea;
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


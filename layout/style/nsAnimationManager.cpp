




#include "nsAnimationManager.h"
#include "nsPresContext.h"
#include "nsRuleProcessorData.h"
#include "nsStyleSet.h"
#include "nsCSSRules.h"
#include "nsStyleAnimation.h"
#include "nsSMILKeySpline.h"
#include "nsEventDispatcher.h"

using namespace mozilla;

struct AnimationPropertySegment
{
  float mFromKey, mToKey;
  nsStyleAnimation::Value mFromValue, mToValue;
  css::ComputedTimingFunction mTimingFunction;
};

struct AnimationProperty
{
  nsCSSProperty mProperty;
  InfallibleTArray<AnimationPropertySegment> mSegments;
};





struct ElementAnimation
{
  ElementAnimation()
    : mLastNotification(LAST_NOTIFICATION_NONE)
  {
  }

  nsString mName; 
  float mIterationCount; 
  PRUint8 mDirection;
  PRUint8 mFillMode;
  PRUint8 mPlayState;

  bool FillsForwards() const {
    return mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BOTH ||
           mFillMode == NS_STYLE_ANIMATION_FILL_MODE_FORWARDS;
  }
  bool FillsBackwards() const {
    return mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BOTH ||
           mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BACKWARDS;
  }

  bool IsPaused() const {
    return mPlayState == NS_STYLE_ANIMATION_PLAY_STATE_PAUSED;
  }

  TimeStamp mStartTime; 
  TimeStamp mPauseStart;
  TimeDuration mIterationDuration;

  enum {
    LAST_NOTIFICATION_NONE = PRUint32(-1),
    LAST_NOTIFICATION_END = PRUint32(-2)
  };
  
  
  PRUint32 mLastNotification;

  InfallibleTArray<AnimationProperty> mProperties;
};

typedef nsAnimationManager::EventArray EventArray;
typedef nsAnimationManager::AnimationEventInfo AnimationEventInfo;




struct ElementAnimations : public mozilla::css::CommonElementAnimationData
{
  ElementAnimations(dom::Element *aElement, nsIAtom *aElementProperty,
                     nsAnimationManager *aAnimationManager)
    : CommonElementAnimationData(aElement, aElementProperty,
                                 aAnimationManager),
      mNeedsRefreshes(true)
  {
  }

  void EnsureStyleRuleFor(TimeStamp aRefreshTime,
                          EventArray &aEventsToDispatch);

  bool IsForElement() const { 
    return mElementProperty == nsGkAtoms::animationsProperty;
  }

  void PostRestyleForAnimation(nsPresContext *aPresContext) {
    nsRestyleHint hint = IsForElement() ? eRestyle_Self : eRestyle_Subtree;
    aPresContext->PresShell()->RestyleForAnimation(mElement, hint);
  }

  
  
  
  
  
  
  
  nsRefPtr<css::AnimValuesStyleRule> mStyleRule;
  
  TimeStamp mStyleRuleRefreshTime;

  
  
  
  bool mNeedsRefreshes;

  InfallibleTArray<ElementAnimation> mAnimations;
};

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

void
ElementAnimations::EnsureStyleRuleFor(TimeStamp aRefreshTime,
                                      EventArray& aEventsToDispatch)
{
  if (!mNeedsRefreshes) {
    
    mStyleRuleRefreshTime = aRefreshTime;
    return;
  }

  
  if (mStyleRuleRefreshTime.IsNull() ||
      mStyleRuleRefreshTime != aRefreshTime) {
    mStyleRuleRefreshTime = aRefreshTime;
    mStyleRule = nsnull;
    
    mNeedsRefreshes = false;

    
    
    
    nsCSSPropertySet properties;

    for (PRUint32 animIdx = mAnimations.Length(); animIdx-- != 0; ) {
      ElementAnimation &anim = mAnimations[animIdx];

      if (anim.mProperties.Length() == 0 ||
          anim.mIterationDuration.ToMilliseconds() <= 0.0) {
        
        continue;
      }

      TimeDuration currentTimeDuration;
      if (anim.IsPaused()) {
        
        currentTimeDuration = anim.mPauseStart - anim.mStartTime;
      } else {
        currentTimeDuration = aRefreshTime - anim.mStartTime;
      }

      
      
      double currentIterationCount =
        currentTimeDuration / anim.mIterationDuration;
      bool dispatchStartOrIteration = false;
      if (currentIterationCount >= double(anim.mIterationCount)) {
        
        if (IsForElement() && 
            anim.mLastNotification !=
              ElementAnimation::LAST_NOTIFICATION_END) {
          anim.mLastNotification = ElementAnimation::LAST_NOTIFICATION_END;
          AnimationEventInfo ei(mElement, anim.mName, NS_ANIMATION_END,
                                currentTimeDuration);
          aEventsToDispatch.AppendElement(ei);
        }

        if (!anim.FillsForwards()) {
          
          continue;
        }
        currentIterationCount = double(anim.mIterationCount);
      } else {
        if (!anim.IsPaused()) {
          mNeedsRefreshes = true;
        }
        if (currentIterationCount < 0.0) {
          if (!anim.FillsBackwards()) {
            
            continue;
          }
          currentIterationCount = 0.0;
        } else {
          dispatchStartOrIteration = !anim.IsPaused();
        }
      }

      
      
      NS_ABORT_IF_FALSE(currentIterationCount >= 0.0, "must be positive");
      PRUint32 whichIteration = int(currentIterationCount);
      if (whichIteration == anim.mIterationCount && whichIteration != 0) {
        
        
        
        --whichIteration;
      }
      double positionInIteration =
        currentIterationCount - double(whichIteration);
      bool thisIterationReverse = false;
      switch (anim.mDirection) {
        case NS_STYLE_ANIMATION_DIRECTION_NORMAL:
          thisIterationReverse = false;
          break;
        case NS_STYLE_ANIMATION_DIRECTION_REVERSE:
          thisIterationReverse = true;
          break;
        case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE:
          thisIterationReverse = (whichIteration & 1) == 1;
          break;
        case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE_REVERSE:
          thisIterationReverse = (whichIteration & 1) == 0;
          break;
      }
      if (thisIterationReverse) {
        positionInIteration = 1.0 - positionInIteration;
      }

      
      if (IsForElement() && dispatchStartOrIteration &&
          whichIteration != anim.mLastNotification) {
        
        
        
        
        
        
        PRUint32 message =
          anim.mLastNotification == ElementAnimation::LAST_NOTIFICATION_NONE
            ? NS_ANIMATION_START : NS_ANIMATION_ITERATION;
        anim.mLastNotification = whichIteration;
        AnimationEventInfo ei(mElement, anim.mName, message,
                              currentTimeDuration);
        aEventsToDispatch.AppendElement(ei);
      }

      NS_ABORT_IF_FALSE(0.0 <= positionInIteration &&
                          positionInIteration <= 1.0,
                        "position should be in [0-1]");

      for (PRUint32 propIdx = 0, propEnd = anim.mProperties.Length();
           propIdx != propEnd; ++propIdx)
      {
        const AnimationProperty &prop = anim.mProperties[propIdx];

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

        
        const AnimationPropertySegment *segment = prop.mSegments.Elements();
        while (segment->mToKey < positionInIteration) {
          NS_ABORT_IF_FALSE(segment->mFromKey < segment->mToKey,
                            "incorrect keys");
          ++segment;
          NS_ABORT_IF_FALSE(segment->mFromKey == (segment-1)->mToKey,
                            "incorrect keys");
        }
        NS_ABORT_IF_FALSE(segment->mFromKey < segment->mToKey,
                          "incorrect keys");
        NS_ABORT_IF_FALSE(segment - prop.mSegments.Elements() <
                            prop.mSegments.Length(),
                          "ran off end");

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

ElementAnimations*
nsAnimationManager::GetElementAnimations(dom::Element *aElement,
                                         nsCSSPseudoElements::Type aPseudoType,
                                         bool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementData)) {
    
    return nsnull;
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
    return nsnull;
  }
  ElementAnimations *ea = static_cast<ElementAnimations*>(
                             aElement->GetProperty(propName));
  if (!ea && aCreateIfNeeded) {
    
    ea = new ElementAnimations(aElement, propName, this);
    if (!ea) {
      NS_WARNING("out of memory");
      return nsnull;
    }
    nsresult rv = aElement->SetProperty(propName, ea,
                                        ElementAnimationsPropertyDtor, nsnull);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete ea;
      return nsnull;
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
nsAnimationManager::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return CommonAnimationManager::SizeOfExcludingThis(aMallocSizeOf);

  
  
  
  
}

 size_t
nsAnimationManager::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

nsIStyleRule*
nsAnimationManager::CheckAnimationRule(nsStyleContext* aStyleContext,
                                       mozilla::dom::Element* aElement)
{
  if (!mPresContext->IsProcessingAnimationStyleChange()) {
    
    
    
    

    const nsStyleDisplay *disp = aStyleContext->GetStyleDisplay();
    ElementAnimations *ea =
      GetElementAnimations(aElement, aStyleContext->GetPseudoType(), false);
    if (!ea &&
        disp->mAnimations.Length() == 1 &&
        disp->mAnimations[0].GetName().IsEmpty()) {
      return nsnull;
    }

    
    InfallibleTArray<ElementAnimation> newAnimations;
    BuildAnimations(aStyleContext, newAnimations);

    if (newAnimations.IsEmpty()) {
      if (ea) {
        ea->Destroy();
      }
      return nsnull;
    }

    TimeStamp refreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();

    if (ea) {
      
      ea->mStyleRule = nsnull;
      ea->mStyleRuleRefreshTime = TimeStamp();

      
      
      
      
      
      
      
      
      
      
      if (!ea->mAnimations.IsEmpty()) {
        for (PRUint32 newIdx = 0, newEnd = newAnimations.Length();
             newIdx != newEnd; ++newIdx) {
          ElementAnimation *newAnim = &newAnimations[newIdx];

          
          
          
          
          
          
          
          
          const ElementAnimation *oldAnim = nsnull;
          for (PRUint32 oldIdx = ea->mAnimations.Length(); oldIdx-- != 0; ) {
            const ElementAnimation *a = &ea->mAnimations[oldIdx];
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

    ea->EnsureStyleRuleFor(refreshTime, mPendingEvents);
    
    
    
    
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
    MOZ_STATIC_ASSERT(sizeof(PLDHashNumber) == sizeof(PRUint32),
                      "this hash function assumes PLDHashNumber is PRUint32");
    MOZ_STATIC_ASSERT(PLDHashNumber(-1) > PLDHashNumber(0),
                      "this hash function assumes PLDHashNumber is PRUint32");
    float key = *aKey;
    NS_ABORT_IF_FALSE(0.0f <= key && key <= 1.0f, "out of range");
    return PLDHashNumber(key * PR_UINT32_MAX);
  }
  enum { ALLOW_MEMMOVE = true };

private:
  const float mValue;
};

struct KeyframeData {
  float mKey;
  PRUint32 mIndex; 
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
  ResolvedStyleCache() {
    mCache.Init(16); 
  }
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
                                    InfallibleTArray<ElementAnimation>& aAnimations)
{
  NS_ABORT_IF_FALSE(aAnimations.IsEmpty(), "expect empty array");

  ResolvedStyleCache resolvedStyles;

  const nsStyleDisplay *disp = aStyleContext->GetStyleDisplay();
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();
  for (PRUint32 animIdx = 0, animEnd = disp->mAnimations.Length();
       animIdx != animEnd; ++animIdx) {
    const nsAnimation& aSrc = disp->mAnimations[animIdx];
    ElementAnimation& aDest = *aAnimations.AppendElement();

    aDest.mName = aSrc.GetName();
    aDest.mIterationCount = aSrc.GetIterationCount();
    aDest.mDirection = aSrc.GetDirection();
    aDest.mFillMode = aSrc.GetFillMode();
    aDest.mPlayState = aSrc.GetPlayState();

    aDest.mStartTime = now + TimeDuration::FromMilliseconds(aSrc.GetDelay());
    if (aDest.IsPaused()) {
      aDest.mPauseStart = now;
    } else {
      aDest.mPauseStart = TimeStamp();
    }

    aDest.mIterationDuration = TimeDuration::FromMilliseconds(aSrc.GetDuration());

    nsCSSKeyframesRule *rule = KeyframesRuleFor(aDest.mName);
    if (!rule) {
      
      continue;
    }

    
    
    
    
    
    

    AutoInfallibleTArray<KeyframeData, 16> sortedKeyframes;

    for (PRUint32 ruleIdx = 0, ruleEnd = rule->StyleRuleCount();
         ruleIdx != ruleEnd; ++ruleIdx) {
      css::Rule* cssRule = rule->GetStyleRuleAt(ruleIdx);
      NS_ABORT_IF_FALSE(cssRule, "must have rule");
      NS_ABORT_IF_FALSE(cssRule->GetType() == css::Rule::KEYFRAME_RULE,
                        "must be keyframe rule");
      nsCSSKeyframeRule *kfRule = static_cast<nsCSSKeyframeRule*>(cssRule);

      const nsTArray<float> &keys = kfRule->GetKeys();
      for (PRUint32 keyIdx = 0, keyEnd = keys.Length();
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

    for (PRUint32 kfIdx = 0, kfEnd = sortedKeyframes.Length();
         kfIdx != kfEnd; ++kfIdx) {
      css::Declaration *decl = sortedKeyframes[kfIdx].mRule->Declaration();
      for (PRUint32 propIdx = 0, propEnd = decl->Count();
           propIdx != propEnd; ++propIdx) {
        properties.AddProperty(decl->OrderValueAt(propIdx));
      }
    }

    for (nsCSSProperty prop = nsCSSProperty(0);
         prop < eCSSProperty_COUNT_no_shorthands;
         prop = nsCSSProperty(prop + 1)) {
      if (!properties.HasProperty(prop) ||
          nsCSSProps::kAnimTypeTable[prop] == eStyleAnimType_None) {
        continue;
      }

      
      
      
      
      AutoInfallibleTArray<PRUint32, 16> keyframesWithProperty;
      float lastKey = 100.0f; 
      for (PRUint32 kfIdx = 0, kfEnd = sortedKeyframes.Length();
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

      AnimationProperty &propData = *aDest.mProperties.AppendElement();
      propData.mProperty = prop;

      KeyframeData *fromKeyframe = nsnull;
      nsRefPtr<nsStyleContext> fromContext;
      bool interpolated = true;
      for (PRUint32 wpIdx = 0, wpEnd = keyframesWithProperty.Length();
           wpIdx != wpEnd; ++wpIdx) {
        PRUint32 kfIdx = keyframesWithProperty[wpIdx];
        KeyframeData &toKeyframe = sortedKeyframes[kfIdx];

        nsRefPtr<nsStyleContext> toContext =
          resolvedStyles.Get(mPresContext, aStyleContext, toKeyframe.mRule);

        if (fromKeyframe) {
          interpolated = interpolated &&
            BuildSegment(propData.mSegments, prop, aSrc,
                         fromKeyframe->mKey, fromContext,
                         fromKeyframe->mRule->Declaration(),
                         toKeyframe.mKey, toContext);
        } else {
          if (toKeyframe.mKey != 0.0f) {
            
            
            interpolated = interpolated &&
              BuildSegment(propData.mSegments, prop, aSrc,
                           0.0f, aStyleContext, nsnull,
                           toKeyframe.mKey, toContext);
          }
        }

        fromContext = toContext;
        fromKeyframe = &toKeyframe;
      }

      if (fromKeyframe->mKey != 1.0f) {
        
        
        interpolated = interpolated &&
          BuildSegment(propData.mSegments, prop, aSrc,
                       fromKeyframe->mKey, fromContext,
                       fromKeyframe->mRule->Declaration(),
                       1.0f, aStyleContext);
      }

      
      
      
      
      
      
      if (!interpolated) {
        aDest.mProperties.RemoveElementAt(aDest.mProperties.Length() - 1);
      }
    }
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
    tf = &aFromContext->GetStyleDisplay()->mAnimations[0].GetTimingFunction();
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

  ElementAnimations *ea =
    GetElementAnimations(aElement, aPseudoType, false);
  if (!ea) {
    return nsnull;
  }

  NS_WARN_IF_FALSE(ea->mStyleRuleRefreshTime ==
                     mPresContext->RefreshDriver()->MostRecentRefresh(),
                   "should already have refreshed style rule");

  if (mPresContext->IsProcessingRestyles() &&
      !mPresContext->IsProcessingAnimationStyleChange()) {
    
    

    if (ea->mStyleRule) {
      ea->PostRestyleForAnimation(mPresContext);
    }

    return nsnull;
  }

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

  
  
  
  for (PRCList *l = PR_LIST_HEAD(&mElementData); l != &mElementData;
       l = PR_NEXT_LINK(l)) {
    ElementAnimations *ea = static_cast<ElementAnimations*>(l);
    nsRefPtr<css::AnimValuesStyleRule> oldStyleRule = ea->mStyleRule;
    ea->EnsureStyleRuleFor(mPresContext->RefreshDriver()->MostRecentRefresh(),
                           mPendingEvents);
    if (oldStyleRule != ea->mStyleRule) {
      ea->PostRestyleForAnimation(mPresContext);
    }
  }

  DispatchEvents(); 
}

void
nsAnimationManager::DoDispatchEvents()
{
  EventArray events;
  mPendingEvents.SwapElements(events);
  for (PRUint32 i = 0, i_end = events.Length(); i < i_end; ++i) {
    AnimationEventInfo &info = events[i];
    nsEventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}

nsCSSKeyframesRule*
nsAnimationManager::KeyframesRuleFor(const nsSubstring& aName)
{
  if (mKeyframesListIsDirty) {
    mKeyframesListIsDirty = false;

    nsTArray<nsCSSKeyframesRule*> rules;
    mPresContext->StyleSet()->AppendKeyframesRules(mPresContext, rules);

    
    mKeyframesRules.Clear();
    for (PRUint32 i = 0, i_end = rules.Length(); i != i_end; ++i) {
      nsCSSKeyframesRule *rule = rules[i];
      mKeyframesRules.Put(rule->GetName(), rule);
    }
  }

  return mKeyframesRules.Get(aName);
}


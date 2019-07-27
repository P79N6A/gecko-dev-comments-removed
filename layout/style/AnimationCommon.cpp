




#include "AnimationCommon.h"
#include "nsTransitionManager.h"
#include "nsAnimationManager.h"

#include "mozilla/dom/AnimationPlayerBinding.h"
#include "ActiveLayerTracker.h"
#include "gfxPlatform.h"
#include "nsRuleData.h"
#include "nsCSSPropertySet.h"
#include "nsCSSValue.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStyleContext.h"
#include "nsIFrame.h"
#include "nsLayoutUtils.h"
#include "mozilla/LookAndFeel.h"
#include "Layers.h"
#include "FrameLayerBuilder.h"
#include "nsDisplayList.h"
#include "mozilla/MemoryReporting.h"
#include "RestyleManager.h"
#include "nsStyleSet.h"
#include "nsStyleChangeList.h"


using mozilla::layers::Layer;

namespace mozilla {

 bool
IsGeometricProperty(nsCSSProperty aProperty)
{
  switch (aProperty) {
    case eCSSProperty_bottom:
    case eCSSProperty_height:
    case eCSSProperty_left:
    case eCSSProperty_right:
    case eCSSProperty_top:
    case eCSSProperty_width:
      return true;
    default:
      return false;
  }
}

namespace css {

CommonAnimationManager::CommonAnimationManager(nsPresContext *aPresContext)
  : mPresContext(aPresContext)
{
  PR_INIT_CLIST(&mElementCollections);
}

CommonAnimationManager::~CommonAnimationManager()
{
  NS_ABORT_IF_FALSE(!mPresContext, "Disconnect should have been called");
}

void
CommonAnimationManager::Disconnect()
{
  
  RemoveAllElementCollections();

  mPresContext = nullptr;
}

void
CommonAnimationManager::RemoveAllElementCollections()
{
  while (!PR_CLIST_IS_EMPTY(&mElementCollections)) {
    ElementAnimationCollection* head =
      static_cast<ElementAnimationCollection*>(
        PR_LIST_HEAD(&mElementCollections));
    head->Destroy();
  }
}

ElementAnimationCollection*
CommonAnimationManager::GetAnimationsForCompositor(nsIContent* aContent,
                                                   nsIAtom* aElementProperty,
                                                   nsCSSProperty aProperty)
{
  if (!aContent->MayHaveAnimations())
    return nullptr;
  ElementAnimationCollection* collection =
    static_cast<ElementAnimationCollection*>(
      aContent->GetProperty(aElementProperty));
  if (!collection ||
      !collection->HasAnimationOfProperty(aProperty) ||
      !collection->CanPerformOnCompositorThread(
        ElementAnimationCollection::CanAnimate_AllowPartial)) {
    return nullptr;
  }

  
  
  nsIFrame* frame = nsLayoutUtils::GetStyleFrame(collection->mElement);
  if (frame) {
    if (aProperty == eCSSProperty_opacity) {
      ActiveLayerTracker::NotifyAnimated(frame, eCSSProperty_opacity);
    } else if (aProperty == eCSSProperty_transform) {
      ActiveLayerTracker::NotifyAnimated(frame, eCSSProperty_transform);
    }
  }

  return collection;
}





NS_IMPL_ISUPPORTS(CommonAnimationManager, nsIStyleRuleProcessor)

nsRestyleHint
CommonAnimationManager::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

nsRestyleHint
CommonAnimationManager::HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

bool
CommonAnimationManager::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return false;
}

nsRestyleHint
CommonAnimationManager::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

 bool
CommonAnimationManager::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  return false;
}

 size_t
CommonAnimationManager::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  
  

  return 0;
}

 size_t
CommonAnimationManager::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

void
CommonAnimationManager::AddStyleUpdatesTo(RestyleTracker& aTracker)
{
  PRCList* next = PR_LIST_HEAD(&mElementCollections);
  while (next != &mElementCollections) {
    ElementAnimationCollection* collection = static_cast<ElementAnimationCollection*>(next);
    next = PR_NEXT_LINK(next);

    if (!collection->IsForElement()) {
      
      
      
      
      continue;
    }

    nsRestyleHint rshint = collection->IsForTransitions()
      ? eRestyle_CSSTransitions : eRestyle_CSSAnimations;
    aTracker.AddPendingRestyle(collection->mElement, rshint, nsChangeHint(0));
  }
}

 bool
CommonAnimationManager::ExtractComputedValueForTransition(
                          nsCSSProperty aProperty,
                          nsStyleContext* aStyleContext,
                          StyleAnimationValue& aComputedValue)
{
  bool result = StyleAnimationValue::ExtractComputedValue(aProperty,
                                                          aStyleContext,
                                                          aComputedValue);
  if (aProperty == eCSSProperty_visibility) {
    NS_ABORT_IF_FALSE(aComputedValue.GetUnit() ==
                        StyleAnimationValue::eUnit_Enumerated,
                      "unexpected unit");
    aComputedValue.SetIntValue(aComputedValue.GetIntValue(),
                               StyleAnimationValue::eUnit_Visibility);
  }
  return result;
}

already_AddRefed<nsStyleContext>
CommonAnimationManager::ReparentContent(nsIContent* aContent,
                                        nsStyleContext* aParentStyle)
{
  nsStyleSet* styleSet = mPresContext->PresShell()->StyleSet();
  nsIFrame* primaryFrame = nsLayoutUtils::GetStyleFrame(aContent);
  if (!primaryFrame) {
    return nullptr;
  }

  dom::Element* element = aContent->IsElement()
                          ? aContent->AsElement()
                          : nullptr;

  nsRefPtr<nsStyleContext> newStyle =
    styleSet->ReparentStyleContext(primaryFrame->StyleContext(),
                                   aParentStyle, element);
  primaryFrame->SetStyleContext(newStyle);
  ReparentBeforeAndAfter(element, primaryFrame, newStyle, styleSet);

  return newStyle.forget();
}

 void
CommonAnimationManager::ReparentBeforeAndAfter(dom::Element* aElement,
                                               nsIFrame* aPrimaryFrame,
                                               nsStyleContext* aNewStyle,
                                               nsStyleSet* aStyleSet)
{
  if (nsIFrame* before = nsLayoutUtils::GetBeforeFrame(aPrimaryFrame)) {
    nsRefPtr<nsStyleContext> beforeStyle =
      aStyleSet->ReparentStyleContext(before->StyleContext(),
                                     aNewStyle, aElement);
    before->SetStyleContext(beforeStyle);
  }
  if (nsIFrame* after = nsLayoutUtils::GetBeforeFrame(aPrimaryFrame)) {
    nsRefPtr<nsStyleContext> afterStyle =
      aStyleSet->ReparentStyleContext(after->StyleContext(),
                                     aNewStyle, aElement);
    after->SetStyleContext(afterStyle);
  }
}

nsStyleContext*
CommonAnimationManager::UpdateThrottledStyle(dom::Element* aElement,
                                             nsStyleContext* aParentStyle,
                                             nsStyleChangeList& aChangeList)
{
  NS_ASSERTION(mPresContext->TransitionManager()->GetElementTransitions(
                 aElement,
                 nsCSSPseudoElements::ePseudo_NotPseudoElement,
                 false) ||
               mPresContext->AnimationManager()->GetElementAnimations(
                 aElement,
                 nsCSSPseudoElements::ePseudo_NotPseudoElement,
                 false), "element not animated");

  nsIFrame* primaryFrame = nsLayoutUtils::GetStyleFrame(aElement);
  if (!primaryFrame) {
    return nullptr;
  }

  nsStyleContext* oldStyle = primaryFrame->StyleContext();

  nsStyleSet* styleSet = mPresContext->StyleSet();
  nsRefPtr<nsStyleContext> newStyle =
    styleSet->ResolveStyleWithReplacement(aElement, aParentStyle, oldStyle,
      nsRestyleHint(eRestyle_CSSTransitions | eRestyle_CSSAnimations));

  
  
  
  nsChangeHint styleChange =
    oldStyle->CalcStyleDifference(newStyle, nsChangeHint(0));
  aChangeList.AppendChange(primaryFrame, primaryFrame->GetContent(),
                           styleChange);

  primaryFrame->SetStyleContext(newStyle);

  ReparentBeforeAndAfter(aElement, primaryFrame, newStyle, styleSet);

  return newStyle;
}

NS_IMPL_ISUPPORTS(AnimValuesStyleRule, nsIStyleRule)

 void
AnimValuesStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  nsStyleContext *contextParent = aRuleData->mStyleContext->GetParent();
  if (contextParent && contextParent->HasPseudoElementData()) {
    
    
    
    return;
  }

  for (uint32_t i = 0, i_end = mPropertyValuePairs.Length(); i < i_end; ++i) {
    PropertyValuePair &cv = mPropertyValuePairs[i];
    if (aRuleData->mSIDs & nsCachedStyleData::GetBitForSID(
                             nsCSSProps::kSIDTable[cv.mProperty]))
    {
      nsCSSValue *prop = aRuleData->ValueFor(cv.mProperty);
      if (prop->GetUnit() == eCSSUnit_Null) {
#ifdef DEBUG
        bool ok =
#endif
          StyleAnimationValue::UncomputeValue(cv.mProperty, cv.mValue, *prop);
        NS_ABORT_IF_FALSE(ok, "could not store computed value");
      }
    }
  }
}

#ifdef DEBUG
 void
AnimValuesStyleRule::List(FILE* out, int32_t aIndent) const
{
  for (int32_t index = aIndent; --index >= 0; ) fputs("  ", out);
  fputs("[anim values] { ", out);
  for (uint32_t i = 0, i_end = mPropertyValuePairs.Length(); i < i_end; ++i) {
    const PropertyValuePair &pair = mPropertyValuePairs[i];
    nsAutoString value;
    StyleAnimationValue::UncomputeValue(pair.mProperty, pair.mValue, value);
    fprintf(out, "%s: %s; ", nsCSSProps::GetStringValue(pair.mProperty).get(),
                             NS_ConvertUTF16toUTF8(value).get());
  }
  fputs("}\n", out);
}
#endif

void
ComputedTimingFunction::Init(const nsTimingFunction &aFunction)
{
  mType = aFunction.mType;
  if (mType == nsTimingFunction::Function) {
    mTimingFunction.Init(aFunction.mFunc.mX1, aFunction.mFunc.mY1,
                         aFunction.mFunc.mX2, aFunction.mFunc.mY2);
  } else {
    mSteps = aFunction.mSteps;
  }
}

static inline double
StepEnd(uint32_t aSteps, double aPortion)
{
  NS_ABORT_IF_FALSE(0.0 <= aPortion && aPortion <= 1.0, "out of range");
  uint32_t step = uint32_t(aPortion * aSteps); 
  return double(step) / double(aSteps);
}

double
ComputedTimingFunction::GetValue(double aPortion) const
{
  switch (mType) {
    case nsTimingFunction::Function:
      return mTimingFunction.GetSplineValue(aPortion);
    case nsTimingFunction::StepStart:
      
      
      
      
      
      
      return 1.0 - StepEnd(mSteps, 1.0 - aPortion);
    default:
      NS_ABORT_IF_FALSE(false, "bad type");
      
    case nsTimingFunction::StepEnd:
      return StepEnd(mSteps, aPortion);
  }
}

} 



const double ComputedTiming::kNullTimeFraction =
  mozilla::PositiveInfinity<double>();

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(ElementAnimation, mTimeline)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(ElementAnimation, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(ElementAnimation, Release)

JSObject*
ElementAnimation::WrapObject(JSContext* aCx)
{
  return dom::AnimationPlayerBinding::Wrap(aCx, this);
}

double
ElementAnimation::StartTime() const
{
  Nullable<double> startTime = mTimeline->ToTimelineTime(mStartTime);
  return startTime.IsNull() ? 0.0 : startTime.Value();
}

double
ElementAnimation::CurrentTime() const
{
  
  
  
  
  Nullable<TimeDuration> currentTime = GetLocalTime();

  
  
  
  
  
  
  
  
  
  
  
  if (currentTime.IsNull()) {
    return 0.0;
  }

  return currentTime.Value().ToMilliseconds();
}

bool
ElementAnimation::IsRunning() const
{
  if (IsPaused() || IsFinishedTransition()) {
    return false;
  }

  ComputedTiming computedTiming = GetComputedTiming(mTiming);
  return computedTiming.mPhase == ComputedTiming::AnimationPhase_Active;
}

bool
ElementAnimation::IsCurrent() const
{
  if (IsFinishedTransition()) {
    return false;
  }

  ComputedTiming computedTiming = GetComputedTiming(mTiming);
  return computedTiming.mPhase == ComputedTiming::AnimationPhase_Before ||
         computedTiming.mPhase == ComputedTiming::AnimationPhase_Active;
}

bool
ElementAnimation::HasAnimationOfProperty(nsCSSProperty aProperty) const
{
  for (uint32_t propIdx = 0, propEnd = mProperties.Length();
       propIdx != propEnd; ++propIdx) {
    if (aProperty == mProperties[propIdx].mProperty) {
      return true;
    }
  }
  return false;
}

ComputedTiming
ElementAnimation::GetComputedTimingAt(const Nullable<TimeDuration>& aLocalTime,
                                      const AnimationTiming& aTiming)
{
  const TimeDuration zeroDuration;

  
  
  
  
  
  MOZ_ASSERT(aTiming.mIterationDuration >= zeroDuration,
             "Expecting iteration duration >= 0");

  
  ComputedTiming result;

  result.mActiveDuration = ActiveDuration(aTiming);

  
  
  if (aLocalTime.IsNull()) {
    return result;
  }
  const TimeDuration& localTime = aLocalTime.Value();

  
  
  
  bool isEndOfFinalIteration = false;

  
  TimeDuration activeTime;
  
  
  
  if (result.mActiveDuration != TimeDuration::Forever() &&
      localTime >= aTiming.mDelay + result.mActiveDuration) {
    result.mPhase = ComputedTiming::AnimationPhase_After;
    if (!aTiming.FillsForwards()) {
      
      result.mTimeFraction = ComputedTiming::kNullTimeFraction;
      return result;
    }
    activeTime = result.mActiveDuration;
    
    
    isEndOfFinalIteration =
      aTiming.mIterationCount != 0.0 &&
      aTiming.mIterationCount == floor(aTiming.mIterationCount);
  } else if (localTime < aTiming.mDelay) {
    result.mPhase = ComputedTiming::AnimationPhase_Before;
    if (!aTiming.FillsBackwards()) {
      
      result.mTimeFraction = ComputedTiming::kNullTimeFraction;
      return result;
    }
    
  } else {
    MOZ_ASSERT(result.mActiveDuration != zeroDuration,
               "How can we be in the middle of a zero-duration interval?");
    result.mPhase = ComputedTiming::AnimationPhase_Active;
    activeTime = localTime - aTiming.mDelay;
  }

  
  TimeDuration iterationTime;
  if (aTiming.mIterationDuration != zeroDuration) {
    iterationTime = isEndOfFinalIteration
                    ? aTiming.mIterationDuration
                    : activeTime % aTiming.mIterationDuration;
  } 

  
  if (isEndOfFinalIteration) {
    result.mCurrentIteration =
      aTiming.mIterationCount == NS_IEEEPositiveInfinity()
      ? UINT64_MAX 
                   
      : static_cast<uint64_t>(aTiming.mIterationCount) - 1;
  } else if (activeTime == zeroDuration) {
    
    
    
    
    result.mCurrentIteration =
      result.mPhase == ComputedTiming::AnimationPhase_After
      ? static_cast<uint64_t>(aTiming.mIterationCount) 
      : 0;
  } else {
    result.mCurrentIteration =
      static_cast<uint64_t>(activeTime / aTiming.mIterationDuration); 
  }

  
  if (result.mPhase == ComputedTiming::AnimationPhase_Before) {
    result.mTimeFraction = 0.0;
  } else if (result.mPhase == ComputedTiming::AnimationPhase_After) {
    result.mTimeFraction = isEndOfFinalIteration
                         ? 1.0
                         : fmod(aTiming.mIterationCount, 1.0f);
  } else {
    
    MOZ_ASSERT(aTiming.mIterationDuration != zeroDuration,
               "In the active phase of a zero-duration animation?");
    result.mTimeFraction =
      aTiming.mIterationDuration == TimeDuration::Forever()
      ? 0.0
      : iterationTime / aTiming.mIterationDuration;
  }

  bool thisIterationReverse = false;
  switch (aTiming.mDirection) {
    case NS_STYLE_ANIMATION_DIRECTION_NORMAL:
      thisIterationReverse = false;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_REVERSE:
      thisIterationReverse = true;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE:
      thisIterationReverse = (result.mCurrentIteration & 1) == 1;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE_REVERSE:
      thisIterationReverse = (result.mCurrentIteration & 1) == 0;
      break;
  }
  if (thisIterationReverse) {
    result.mTimeFraction = 1.0 - result.mTimeFraction;
  }

  return result;
}

TimeDuration
ElementAnimation::ActiveDuration(const AnimationTiming& aTiming)
{
  if (aTiming.mIterationCount == mozilla::PositiveInfinity<float>()) {
    
    
    
    const TimeDuration zeroDuration;
    return aTiming.mIterationDuration == zeroDuration
           ? zeroDuration
           : TimeDuration::Forever();
  }
  return aTiming.mIterationDuration.MultDouble(aTiming.mIterationCount);
}

bool
ElementAnimationCollection::CanAnimatePropertyOnCompositor(
  const dom::Element *aElement,
  nsCSSProperty aProperty,
  CanAnimateFlags aFlags)
{
  bool shouldLog = nsLayoutUtils::IsAnimationLoggingEnabled();
  if (!gfxPlatform::OffMainThreadCompositingEnabled()) {
    if (shouldLog) {
      nsCString message;
      message.AppendLiteral("Performance warning: Compositor disabled");
      LogAsyncAnimationFailure(message);
    }
    return false;
  }

  nsIFrame* frame = nsLayoutUtils::GetStyleFrame(aElement);
  if (IsGeometricProperty(aProperty)) {
    if (shouldLog) {
      nsCString message;
      message.AppendLiteral("Performance warning: Async animation of geometric property '");
      message.Append(nsCSSProps::GetStringValue(aProperty));
      message.AppendLiteral("' is disabled");
      LogAsyncAnimationFailure(message, aElement);
    }
    return false;
  }
  if (aProperty == eCSSProperty_transform) {
    if (frame->Preserves3D() &&
        frame->Preserves3DChildren()) {
      if (shouldLog) {
        nsCString message;
        message.AppendLiteral("Gecko bug: Async animation of 'preserve-3d' transforms is not supported.  See bug 779598");
        LogAsyncAnimationFailure(message, aElement);
      }
      return false;
    }
    if (frame->IsSVGTransformed()) {
      if (shouldLog) {
        nsCString message;
        message.AppendLiteral("Gecko bug: Async 'transform' animations of frames with SVG transforms is not supported.  See bug 779599");
        LogAsyncAnimationFailure(message, aElement);
      }
      return false;
    }
    if (aFlags & CanAnimate_HasGeometricProperty) {
      if (shouldLog) {
        nsCString message;
        message.AppendLiteral("Performance warning: Async animation of 'transform' not possible due to presence of geometric properties");
        LogAsyncAnimationFailure(message, aElement);
      }
      return false;
    }
  }
  bool enabled = nsLayoutUtils::AreAsyncAnimationsEnabled();
  if (!enabled && shouldLog) {
    nsCString message;
    message.AppendLiteral("Performance warning: Async animations are disabled");
    LogAsyncAnimationFailure(message);
  }
  bool propertyAllowed = (aProperty == eCSSProperty_transform) ||
                         (aProperty == eCSSProperty_opacity) ||
                         (aFlags & CanAnimate_AllowPartial);
  return enabled && propertyAllowed;
}

 bool
ElementAnimationCollection::IsCompositorAnimationDisabledForFrame(
  nsIFrame* aFrame)
{
  void* prop = aFrame->Properties().Get(nsIFrame::RefusedAsyncAnimation());
  return bool(reinterpret_cast<intptr_t>(prop));
}

bool
ElementAnimationCollection::CanPerformOnCompositorThread(
  CanAnimateFlags aFlags) const
{
  nsIFrame* frame = nsLayoutUtils::GetStyleFrame(mElement);
  if (!frame) {
    return false;
  }

  if (mElementProperty != nsGkAtoms::transitionsProperty &&
      mElementProperty != nsGkAtoms::animationsProperty) {
    if (nsLayoutUtils::IsAnimationLoggingEnabled()) {
      nsCString message;
      message.AppendLiteral("Gecko bug: Async animation of pseudoelements"
                            " not supported.  See bug 771367 (");
      message.Append(nsAtomCString(mElementProperty));
      message.Append(")");
      LogAsyncAnimationFailure(message, mElement);
    }
    return false;
  }

  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementAnimation* anim = mAnimations[animIdx];
    bool isRunning = anim->IsRunning();
    for (uint32_t propIdx = 0, propEnd = anim->mProperties.Length();
         propIdx != propEnd; ++propIdx) {
      if (IsGeometricProperty(anim->mProperties[propIdx].mProperty) &&
          isRunning) {
        aFlags = CanAnimateFlags(aFlags | CanAnimate_HasGeometricProperty);
        break;
      }
    }
  }

  bool existsProperty = false;
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementAnimation* anim = mAnimations[animIdx];
    if (!anim->IsRunning()) {
      continue;
    }

    existsProperty = true;

    for (uint32_t propIdx = 0, propEnd = anim->mProperties.Length();
         propIdx != propEnd; ++propIdx) {
      const AnimationProperty& prop = anim->mProperties[propIdx];
      if (!CanAnimatePropertyOnCompositor(mElement,
                                          prop.mProperty,
                                          aFlags) ||
          IsCompositorAnimationDisabledForFrame(frame)) {
        return false;
      }
    }
  }

  
  if (!existsProperty) {
    return false;
  }

  return true;
}

bool
ElementAnimationCollection::HasAnimationOfProperty(
  nsCSSProperty aProperty) const
{
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    const ElementAnimation* anim = mAnimations[animIdx];
    if (anim->HasAnimationOfProperty(aProperty) &&
        !anim->IsFinishedTransition()) {
      return true;
    }
  }
  return false;
}

 void
ElementAnimationCollection::LogAsyncAnimationFailure(nsCString& aMessage,
                                                     const nsIContent* aContent)
{
  if (aContent) {
    aMessage.AppendLiteral(" [");
    aMessage.Append(nsAtomCString(aContent->Tag()));

    nsIAtom* id = aContent->GetID();
    if (id) {
      aMessage.AppendLiteral(" with id '");
      aMessage.Append(nsAtomCString(aContent->GetID()));
      aMessage.Append('\'');
    }
    aMessage.Append(']');
  }
  aMessage.Append('\n');
  printf_stderr(aMessage.get());
}

 void
ElementAnimationCollection::PropertyDtor(void *aObject, nsIAtom *aPropertyName,
                                         void *aPropertyValue, void *aData)
{
  ElementAnimationCollection* collection =
    static_cast<ElementAnimationCollection*>(aPropertyValue);
#ifdef DEBUG
  NS_ABORT_IF_FALSE(!collection->mCalledPropertyDtor, "can't call dtor twice");
  collection->mCalledPropertyDtor = true;
#endif
  delete collection;
}

void
ElementAnimationCollection::EnsureStyleRuleFor(TimeStamp aRefreshTime,
                                               EnsureStyleRuleFlags aFlags)
{
  if (!mNeedsRefreshes) {
    mStyleRuleRefreshTime = aRefreshTime;
    return;
  }

  
  
  
  
  
  
  if (aFlags == EnsureStyleRule_IsThrottled) {
    for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
      ElementAnimation* anim = mAnimations[animIdx];

      
      
      if (anim->IsFinishedTransition() || anim->mProperties.IsEmpty()) {
        continue;
      }

      
      
      ComputedTiming computedTiming = anim->GetComputedTiming(anim->mTiming);

      
      
      
      
      if (!anim->mIsRunningOnCompositor ||
          (computedTiming.mPhase == ComputedTiming::AnimationPhase_After &&
           anim->mLastNotification != ElementAnimation::LAST_NOTIFICATION_END))
      {
        aFlags = EnsureStyleRule_IsNotThrottled;
        break;
      }
    }
  }

  if (aFlags == EnsureStyleRule_IsThrottled) {
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

      if (anim->IsFinishedTransition()) {
        continue;
      }

      
      
      ComputedTiming computedTiming = anim->GetComputedTiming(anim->mTiming);

      if ((computedTiming.mPhase == ComputedTiming::AnimationPhase_Before ||
           computedTiming.mPhase == ComputedTiming::AnimationPhase_Active) &&
          !anim->IsPaused()) {
        mNeedsRefreshes = true;
      }

      
      
      if (computedTiming.mTimeFraction == ComputedTiming::kNullTimeFraction) {
        continue;
      }

      NS_ABORT_IF_FALSE(0.0 <= computedTiming.mTimeFraction &&
                        computedTiming.mTimeFraction <= 1.0,
                        "timing fraction should be in [0-1]");

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
        while (segment->mToKey < computedTiming.mTimeFraction) {
          NS_ABORT_IF_FALSE(segment->mFromKey < segment->mToKey,
                            "incorrect keys");
          ++segment;
          if (segment == segmentEnd) {
            NS_ABORT_IF_FALSE(false, "incorrect time fraction");
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

        double positionInSegment =
          (computedTiming.mTimeFraction - segment->mFromKey) /
          (segment->mToKey - segment->mFromKey);
        double valuePosition =
          segment->mTimingFunction.GetValue(positionInSegment);

        StyleAnimationValue *val =
          mStyleRule->AddEmptyValue(prop.mProperty);

#ifdef DEBUG
        bool result =
#endif
          StyleAnimationValue::Interpolate(prop.mProperty,
                                           segment->mFromValue,
                                           segment->mToValue,
                                           valuePosition, *val);
        NS_ABORT_IF_FALSE(result, "interpolate must succeed now");
      }
    }
  }
}


bool
ElementAnimationCollection::CanThrottleTransformChanges(TimeStamp aTime)
{
  if (!nsLayoutUtils::AreAsyncAnimationsEnabled()) {
    return false;
  }

  
  

  
  if (LookAndFeel::GetInt(LookAndFeel::eIntID_ShowHideScrollbars) == 0) {
    return true;
  }

  
  if ((aTime - mStyleRuleRefreshTime) < TimeDuration::FromMilliseconds(200)) {
    return true;
  }

  
  
  nsIScrollableFrame* scrollable = nsLayoutUtils::GetNearestScrollableFrame(
                                     nsLayoutUtils::GetStyleFrame(mElement));
  if (!scrollable) {
    return true;
  }

  ScrollbarStyles ss = scrollable->GetScrollbarStyles();
  if (ss.mVertical == NS_STYLE_OVERFLOW_HIDDEN &&
      ss.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN &&
      scrollable->GetLogicalScrollPosition() == nsPoint(0, 0)) {
    return true;
  }

  return false;
}

bool
ElementAnimationCollection::CanThrottleAnimation(TimeStamp aTime)
{
  nsIFrame* frame = nsLayoutUtils::GetStyleFrame(mElement);
  if (!frame) {
    return false;
  }

  bool hasTransform = HasAnimationOfProperty(eCSSProperty_transform);
  bool hasOpacity = HasAnimationOfProperty(eCSSProperty_opacity);
  if (hasOpacity) {
    Layer* layer = FrameLayerBuilder::GetDedicatedLayer(
                     frame, nsDisplayItem::TYPE_OPACITY);
    if (!layer || mAnimationGeneration > layer->GetAnimationGeneration()) {
      return false;
    }
  }

  if (!hasTransform) {
    return true;
  }

  Layer* layer = FrameLayerBuilder::GetDedicatedLayer(
                   frame, nsDisplayItem::TYPE_TRANSFORM);
  if (!layer || mAnimationGeneration > layer->GetAnimationGeneration()) {
    return false;
  }

  return CanThrottleTransformChanges(aTime);
}

void
ElementAnimationCollection::UpdateAnimationGeneration(
  nsPresContext* aPresContext)
{
  mAnimationGeneration =
    aPresContext->RestyleManager()->GetAnimationGeneration();
}

bool
ElementAnimationCollection::HasCurrentAnimations()
{
  for (uint32_t animIdx = mAnimations.Length(); animIdx-- != 0; ) {
    if (mAnimations[animIdx]->IsCurrent()) {
      return true;
    }
  }

  return false;
}

}

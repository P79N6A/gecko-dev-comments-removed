




#include "AnimationCommon.h"
#include "nsTransitionManager.h"
#include "nsAnimationManager.h"

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
using mozilla::dom::AnimationPlayer;
using mozilla::dom::Animation;

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
    AnimationPlayerCollection* head =
      static_cast<AnimationPlayerCollection*>(
        PR_LIST_HEAD(&mElementCollections));
    head->Destroy();
  }
}

AnimationPlayerCollection*
CommonAnimationManager::GetAnimationsForCompositor(nsIContent* aContent,
                                                   nsIAtom* aElementProperty,
                                                   nsCSSProperty aProperty)
{
  if (!aContent->MayHaveAnimations())
    return nullptr;
  AnimationPlayerCollection* collection =
    static_cast<AnimationPlayerCollection*>(
      aContent->GetProperty(aElementProperty));
  if (!collection ||
      !collection->HasAnimationOfProperty(aProperty) ||
      !collection->CanPerformOnCompositorThread(
        AnimationPlayerCollection::CanAnimate_AllowPartial)) {
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
    AnimationPlayerCollection* collection =
      static_cast<AnimationPlayerCollection*>(next);
    next = PR_NEXT_LINK(next);

    dom::Element* elementToRestyle = collection->GetElementToRestyle();
    if (elementToRestyle) {
      nsRestyleHint rshint = collection->IsForTransitions()
        ? eRestyle_CSSTransitions : eRestyle_CSSAnimations;
      aTracker.AddPendingRestyle(elementToRestyle, rshint, nsChangeHint(0));
    }
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

} 

bool
AnimationPlayerCollection::CanAnimatePropertyOnCompositor(
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
AnimationPlayerCollection::IsCompositorAnimationDisabledForFrame(
  nsIFrame* aFrame)
{
  void* prop = aFrame->Properties().Get(nsIFrame::RefusedAsyncAnimation());
  return bool(reinterpret_cast<intptr_t>(prop));
}

bool
AnimationPlayerCollection::CanPerformOnCompositorThread(
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

  for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
    const AnimationPlayer* player = mPlayers[playerIdx];
    if (!player->IsRunning() || !player->GetSource()) {
      continue;
    }
    const Animation* anim = player->GetSource();
    for (size_t propIdx = 0, propEnd = anim->Properties().Length();
         propIdx != propEnd; ++propIdx) {
      if (IsGeometricProperty(anim->Properties()[propIdx].mProperty)) {
        aFlags = CanAnimateFlags(aFlags | CanAnimate_HasGeometricProperty);
        break;
      }
    }
  }

  bool existsProperty = false;
  for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
    const AnimationPlayer* player = mPlayers[playerIdx];
    if (!player->IsRunning() || !player->GetSource()) {
      continue;
    }

    const Animation* anim = player->GetSource();
    existsProperty = existsProperty || anim->Properties().Length() > 0;

    for (size_t propIdx = 0, propEnd = anim->Properties().Length();
         propIdx != propEnd; ++propIdx) {
      const AnimationProperty& prop = anim->Properties()[propIdx];
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
AnimationPlayerCollection::HasAnimationOfProperty(
  nsCSSProperty aProperty) const
{
  for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
    const Animation* anim = mPlayers[playerIdx]->GetSource();
    if (anim && anim->HasAnimationOfProperty(aProperty) &&
        !anim->IsFinishedTransition()) {
      return true;
    }
  }
  return false;
}

mozilla::dom::Element*
AnimationPlayerCollection::GetElementToRestyle() const
{
  if (IsForElement()) {
    return mElement;
  }

  nsIFrame* primaryFrame = mElement->GetPrimaryFrame();
  if (!primaryFrame) {
    return nullptr;
  }
  nsIFrame* pseudoFrame;
  if (IsForBeforePseudo()) {
    pseudoFrame = nsLayoutUtils::GetBeforeFrame(primaryFrame);
  } else if (IsForAfterPseudo()) {
    pseudoFrame = nsLayoutUtils::GetAfterFrame(primaryFrame);
  } else {
    MOZ_ASSERT(false, "unknown mElementProperty");
    return nullptr;
  }
  if (!pseudoFrame) {
    return nullptr;
  }
  return pseudoFrame->GetContent()->AsElement();
}

 void
AnimationPlayerCollection::LogAsyncAnimationFailure(nsCString& aMessage,
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
  printf_stderr("%s", aMessage.get());
}

 void
AnimationPlayerCollection::PropertyDtor(void *aObject, nsIAtom *aPropertyName,
                                         void *aPropertyValue, void *aData)
{
  AnimationPlayerCollection* collection =
    static_cast<AnimationPlayerCollection*>(aPropertyValue);
#ifdef DEBUG
  NS_ABORT_IF_FALSE(!collection->mCalledPropertyDtor, "can't call dtor twice");
  collection->mCalledPropertyDtor = true;
#endif
  delete collection;
}

void
AnimationPlayerCollection::Tick()
{
  for (size_t playerIdx = 0, playerEnd = mPlayers.Length();
       playerIdx != playerEnd; playerIdx++) {
    mPlayers[playerIdx]->Tick();
  }
}

void
AnimationPlayerCollection::EnsureStyleRuleFor(TimeStamp aRefreshTime,
                                              EnsureStyleRuleFlags aFlags)
{
  if (!mNeedsRefreshes) {
    mStyleRuleRefreshTime = aRefreshTime;
    return;
  }

  
  
  
  
  
  
  if (aFlags == EnsureStyleRule_IsThrottled) {
    for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
      AnimationPlayer* player = mPlayers[playerIdx];

      
      
      if (!player->GetSource() ||
          player->GetSource()->IsFinishedTransition() ||
          player->GetSource()->Properties().IsEmpty()) {
        continue;
      }

      
      
      ComputedTiming computedTiming = player->GetSource()->GetComputedTiming();

      
      
      
      
      if (!player->mIsRunningOnCompositor ||
          (computedTiming.mPhase == ComputedTiming::AnimationPhase_After &&
           player->GetSource()->LastNotification()
             != Animation::LAST_NOTIFICATION_END))
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

    for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
      AnimationPlayer* player = mPlayers[playerIdx];

      if (!player->GetSource() || player->GetSource()->IsFinishedTransition()) {
        continue;
      }

      
      
      ComputedTiming computedTiming = player->GetSource()->GetComputedTiming();

      if ((computedTiming.mPhase == ComputedTiming::AnimationPhase_Before ||
           computedTiming.mPhase == ComputedTiming::AnimationPhase_Active) &&
          !player->IsPaused()) {
        mNeedsRefreshes = true;
      }

      
      
      
      if (computedTiming.mTimeFraction == ComputedTiming::kNullTimeFraction) {
        continue;
      }

      NS_ABORT_IF_FALSE(0.0 <= computedTiming.mTimeFraction &&
                        computedTiming.mTimeFraction <= 1.0,
                        "timing fraction should be in [0-1]");

      const Animation* anim = player->GetSource();
      for (size_t propIdx = 0, propEnd = anim->Properties().Length();
           propIdx != propEnd; ++propIdx)
      {
        const AnimationProperty& prop = anim->Properties()[propIdx];

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
AnimationPlayerCollection::CanThrottleTransformChanges(TimeStamp aTime)
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
AnimationPlayerCollection::CanThrottleAnimation(TimeStamp aTime)
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
AnimationPlayerCollection::UpdateAnimationGeneration(
  nsPresContext* aPresContext)
{
  mAnimationGeneration =
    aPresContext->RestyleManager()->GetAnimationGeneration();
}

bool
AnimationPlayerCollection::HasCurrentAnimations()
{
  for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
    if (mPlayers[playerIdx]->IsCurrent()) {
      return true;
    }
  }

  return false;
}

}






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
#include "nsRuleProcessorData.h"
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
  , mIsObservingRefreshDriver(false)
{
  PR_INIT_CLIST(&mElementCollections);
}

CommonAnimationManager::~CommonAnimationManager()
{
  MOZ_ASSERT(!mPresContext, "Disconnect should have been called");
}

void
CommonAnimationManager::Disconnect()
{
  
  RemoveAllElementCollections();

  mPresContext = nullptr;
}

void
CommonAnimationManager::AddElementCollection(AnimationPlayerCollection*
                                               aCollection)
{
  if (!mIsObservingRefreshDriver) {
    NS_ASSERTION(aCollection->mNeedsRefreshes,
      "Added data which doesn't need refreshing?");
    
    mPresContext->RefreshDriver()->AddRefreshObserver(this, Flush_Style);
    mIsObservingRefreshDriver = true;
  }

  PR_INSERT_BEFORE(aCollection, &mElementCollections);
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

void
CommonAnimationManager::CheckNeedsRefresh()
{
  for (PRCList *l = PR_LIST_HEAD(&mElementCollections);
       l != &mElementCollections;
       l = PR_NEXT_LINK(l)) {
    if (static_cast<AnimationPlayerCollection*>(l)->mNeedsRefreshes) {
      if (!mIsObservingRefreshDriver) {
        mPresContext->RefreshDriver()->AddRefreshObserver(this, Flush_Style);
        mIsObservingRefreshDriver = true;
      }
      return;
    }
  }
  if (mIsObservingRefreshDriver) {
    mIsObservingRefreshDriver = false;
    mPresContext->RefreshDriver()->RemoveRefreshObserver(this, Flush_Style);
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
    const auto& info = sLayerAnimationInfo;
    for (size_t i = 0; i < ArrayLength(info); i++) {
      if (aProperty == info[i].mProperty) {
        ActiveLayerTracker::NotifyAnimated(frame, aProperty);
        break;
      }
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

 void
CommonAnimationManager::RulesMatching(ElementRuleProcessorData* aData)
{
  MOZ_ASSERT(aData->mPresContext == mPresContext,
             "pres context mismatch");
  nsIStyleRule *rule =
    GetAnimationRule(aData->mElement,
                     nsCSSPseudoElements::ePseudo_NotPseudoElement);
  if (rule) {
    aData->mRuleWalker->Forward(rule);
  }
}

 void
CommonAnimationManager::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  MOZ_ASSERT(aData->mPresContext == mPresContext,
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
CommonAnimationManager::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
CommonAnimationManager::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif

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
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();

  PRCList* next = PR_LIST_HEAD(&mElementCollections);
  while (next != &mElementCollections) {
    AnimationPlayerCollection* collection =
      static_cast<AnimationPlayerCollection*>(next);
    next = PR_NEXT_LINK(next);

    collection->EnsureStyleRuleFor(now, EnsureStyleRule_IsNotThrottled);

    dom::Element* elementToRestyle = collection->GetElementToRestyle();
    if (elementToRestyle) {
      nsRestyleHint rshint = collection->IsForTransitions()
        ? eRestyle_CSSTransitions : eRestyle_CSSAnimations;
      aTracker.AddPendingRestyle(elementToRestyle, rshint, nsChangeHint(0));
    }
  }
}

void
CommonAnimationManager::NotifyCollectionUpdated(AnimationPlayerCollection&
                                                  aCollection)
{
  CheckNeedsRefresh();
  mPresContext->ClearLastStyleUpdateForAllAnimations();
  mPresContext->RestyleManager()->IncrementAnimationGeneration();
  aCollection.UpdateAnimationGeneration(mPresContext);
  aCollection.PostRestyleForAnimation(mPresContext);
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
    MOZ_ASSERT(aComputedValue.GetUnit() ==
                 StyleAnimationValue::eUnit_Enumerated,
               "unexpected unit");
    aComputedValue.SetIntValue(aComputedValue.GetIntValue(),
                               StyleAnimationValue::eUnit_Visibility);
  }
  return result;
}

AnimationPlayerCollection*
CommonAnimationManager::GetAnimationPlayers(dom::Element *aElement,
                                            nsCSSPseudoElements::Type aPseudoType,
                                            bool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementCollections)) {
    
    return nullptr;
  }

  nsIAtom *propName;
  if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    propName = GetAnimationsAtom();
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_before) {
    propName = GetAnimationsBeforeAtom();
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_after) {
    propName = GetAnimationsAfterAtom();
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
      new AnimationPlayerCollection(aElement, propName, this);
    nsresult rv =
      aElement->SetProperty(propName, collection,
                            &AnimationPlayerCollection::PropertyDtor, false);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete collection;
      return nullptr;
    }
    if (propName == nsGkAtoms::animationsProperty ||
        propName == nsGkAtoms::transitionsProperty) {
      aElement->SetMayHaveAnimations();
    }

    AddElementCollection(collection);
  }

  return collection;
}

nsIStyleRule*
CommonAnimationManager::GetAnimationRule(mozilla::dom::Element* aElement,
                                         nsCSSPseudoElements::Type aPseudoType)
{
  MOZ_ASSERT(
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

  RestyleManager* restyleManager = mPresContext->RestyleManager();
  if (restyleManager->SkipAnimationRules()) {
    
    

    if (collection->mStyleRule && restyleManager->PostAnimationRestyles()) {
      collection->PostRestyleForAnimation(mPresContext);
    }

    return nullptr;
  }

  
  
  
  if (IsAnimationManager()) {
    NS_WARN_IF_FALSE(!collection->mNeedsRefreshes ||
                     collection->mStyleRuleRefreshTime ==
                       mPresContext->RefreshDriver()->MostRecentRefresh(),
                     "should already have refreshed style rule");
  } else {
    
    collection->mNeedsRefreshes = true;
    collection->EnsureStyleRuleFor(
      mPresContext->RefreshDriver()->MostRecentRefresh(),
      EnsureStyleRule_IsNotThrottled);
  }

  return collection->mStyleRule;
}

 const CommonAnimationManager::LayerAnimationRecord
  CommonAnimationManager::sLayerAnimationInfo[] =
    { { eCSSProperty_transform,
        nsDisplayItem::TYPE_TRANSFORM,
        nsChangeHint_UpdateTransformLayer },
      { eCSSProperty_opacity,
        nsDisplayItem::TYPE_OPACITY,
        nsChangeHint_UpdateOpacityLayer } };

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
        MOZ_ASSERT(ok, "could not store computed value");
      }
    }
  }
}

#ifdef DEBUG
 void
AnimValuesStyleRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  for (int32_t index = aIndent; --index >= 0; ) {
    str.AppendLiteral("  ");
  }
  str.AppendLiteral("[anim values] { ");
  for (uint32_t i = 0, i_end = mPropertyValuePairs.Length(); i < i_end; ++i) {
    const PropertyValuePair &pair = mPropertyValuePairs[i];
    str.Append(nsCSSProps::GetStringValue(pair.mProperty));
    str.AppendLiteral(": ");
    nsAutoString value;
    StyleAnimationValue::UncomputeValue(pair.mProperty, pair.mValue, value);
    AppendUTF16toUTF8(value, str);
    str.AppendLiteral("; ");
  }
  str.AppendLiteral("}\n");
  fprintf_stderr(out, "%s", str.get());
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
AnimationPlayerCollection::NotifyPlayerUpdated()
{
  
  mNeedsRefreshes = true;
  mStyleRuleRefreshTime = TimeStamp();

  mManager->NotifyCollectionUpdated(*this);
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
  MOZ_ASSERT(!collection->mCalledPropertyDtor, "can't call dtor twice");
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
      if (!mPlayers[playerIdx]->CanThrottle()) {
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
      mPlayers[playerIdx]->ComposeStyle(mStyleRule, properties,
                                        mNeedsRefreshes);
    }
  }

  mManager->CheckNeedsRefresh();
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

  
  if (!mStyleRuleRefreshTime.IsNull() &&
      (aTime - mStyleRuleRefreshTime) < TimeDuration::FromMilliseconds(200)) {
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


  const auto& info = css::CommonAnimationManager::sLayerAnimationInfo;
  for (size_t i = 0; i < ArrayLength(info); i++) {
    auto record = info[i];
    if (!HasAnimationOfProperty(record.mProperty)) {
      continue;
    }

    Layer* layer = FrameLayerBuilder::GetDedicatedLayer(
                     frame, record.mLayerType);
    if (!layer || mAnimationGeneration > layer->GetAnimationGeneration()) {
      return false;
    }

    if (record.mProperty == eCSSProperty_transform &&
        !CanThrottleTransformChanges(aTime)) {
      return false;
    }
  }

  return true;
}

void
AnimationPlayerCollection::UpdateAnimationGeneration(
  nsPresContext* aPresContext)
{
  mAnimationGeneration =
    aPresContext->RestyleManager()->GetAnimationGeneration();
}

bool
AnimationPlayerCollection::HasCurrentAnimations() const
{
  for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
    if (mPlayers[playerIdx]->HasCurrentSource()) {
      return true;
    }
  }

  return false;
}

bool
AnimationPlayerCollection::HasCurrentAnimationsForProperty(nsCSSProperty
                                                             aProperty) const
{
  for (size_t playerIdx = mPlayers.Length(); playerIdx-- != 0; ) {
    const Animation* anim = mPlayers[playerIdx]->GetSource();
    if (anim && anim->IsCurrent() && anim->HasAnimationOfProperty(aProperty)) {
      return true;
    }
  }

  return false;
}

}

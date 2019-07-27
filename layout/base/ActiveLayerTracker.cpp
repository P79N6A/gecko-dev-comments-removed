



#include "ActiveLayerTracker.h"

#include "mozilla/ArrayUtils.h"
#include "nsExpirationTracker.h"
#include "nsContainerFrame.h"
#include "nsIContent.h"
#include "nsRefreshDriver.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "nsAnimationManager.h"
#include "nsTransitionManager.h"
#include "nsDisplayList.h"

namespace mozilla {











class LayerActivity {
public:
  explicit LayerActivity(nsIFrame* aFrame)
    : mFrame(aFrame)
    , mContent(nullptr)
    , mOpacityRestyleCount(0)
    , mTransformRestyleCount(0)
    , mLeftRestyleCount(0)
    , mTopRestyleCount(0)
    , mRightRestyleCount(0)
    , mBottomRestyleCount(0)
    , mMarginLeftRestyleCount(0)
    , mMarginTopRestyleCount(0)
    , mMarginRightRestyleCount(0)
    , mMarginBottomRestyleCount(0)
    , mContentActive(false)
  {}
  ~LayerActivity();
  nsExpirationState* GetExpirationState() { return &mState; }
  uint8_t& RestyleCountForProperty(nsCSSProperty aProperty)
  {
    switch (aProperty) {
    case eCSSProperty_opacity: return mOpacityRestyleCount;
    case eCSSProperty_transform: return mTransformRestyleCount;
    case eCSSProperty_left: return mLeftRestyleCount;
    case eCSSProperty_top: return mTopRestyleCount;
    case eCSSProperty_right: return mRightRestyleCount;
    case eCSSProperty_bottom: return mBottomRestyleCount;
    case eCSSProperty_margin_left: return mMarginLeftRestyleCount;
    case eCSSProperty_margin_top: return mMarginTopRestyleCount;
    case eCSSProperty_margin_right: return mMarginRightRestyleCount;
    case eCSSProperty_margin_bottom: return mMarginBottomRestyleCount;
    default: MOZ_ASSERT(false); return mOpacityRestyleCount;
    }
  }

  
  
  
  
  nsIFrame* mFrame;
  nsIContent* mContent;

  nsExpirationState mState;
  
  uint8_t mOpacityRestyleCount;
  uint8_t mTransformRestyleCount;
  uint8_t mLeftRestyleCount;
  uint8_t mTopRestyleCount;
  uint8_t mRightRestyleCount;
  uint8_t mBottomRestyleCount;
  uint8_t mMarginLeftRestyleCount;
  uint8_t mMarginTopRestyleCount;
  uint8_t mMarginRightRestyleCount;
  uint8_t mMarginBottomRestyleCount;
  bool mContentActive;
};

class LayerActivityTracker final : public nsExpirationTracker<LayerActivity,4> {
public:
  
  enum { GENERATION_MS = 100 };
  LayerActivityTracker()
    : nsExpirationTracker<LayerActivity,4>(GENERATION_MS) {}
  ~LayerActivityTracker() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(LayerActivity* aObject);
};

static LayerActivityTracker* gLayerActivityTracker = nullptr;

LayerActivity::~LayerActivity()
{
  if (mFrame || mContent) {
    NS_ASSERTION(gLayerActivityTracker, "Should still have a tracker");
    gLayerActivityTracker->RemoveObject(this);
  }
}


NS_DECLARE_FRAME_PROPERTY(LayerActivityProperty, DeleteValue<LayerActivity>)

void
LayerActivityTracker::NotifyExpired(LayerActivity* aObject)
{
  RemoveObject(aObject);

  nsIFrame* f = aObject->mFrame;
  nsIContent* c = aObject->mContent;
  aObject->mFrame = nullptr;
  aObject->mContent = nullptr;

  MOZ_ASSERT((f == nullptr) != (c == nullptr),
             "A LayerActivity object should always have a reference to either its frame or its content");

  if (f) {
    
    
    if (f->PresContext()->GetContainerWeak()) {
      f->SchedulePaint();
    }
    f->RemoveStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY);
    f->Properties().Delete(LayerActivityProperty());
  } else {
    c->DeleteProperty(nsGkAtoms::LayerActivity);
  }
}

static LayerActivity*
GetLayerActivity(nsIFrame* aFrame)
{
  if (!aFrame->HasAnyStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY)) {
    return nullptr;
  }
  FrameProperties properties = aFrame->Properties();
  return static_cast<LayerActivity*>(properties.Get(LayerActivityProperty()));
}

static LayerActivity*
GetLayerActivityForUpdate(nsIFrame* aFrame)
{
  FrameProperties properties = aFrame->Properties();
  LayerActivity* layerActivity =
    static_cast<LayerActivity*>(properties.Get(LayerActivityProperty()));
  if (layerActivity) {
    gLayerActivityTracker->MarkUsed(layerActivity);
  } else {
    if (!gLayerActivityTracker) {
      gLayerActivityTracker = new LayerActivityTracker();
    }
    layerActivity = new LayerActivity(aFrame);
    gLayerActivityTracker->AddObject(layerActivity);
    aFrame->AddStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY);
    properties.Set(LayerActivityProperty(), layerActivity);
  }
  return layerActivity;
}

static void
IncrementMutationCount(uint8_t* aCount)
{
  *aCount = uint8_t(std::min(0xFF, *aCount + 1));
}

 void
ActiveLayerTracker::TransferActivityToContent(nsIFrame* aFrame, nsIContent* aContent)
{
  if (!aFrame->HasAnyStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY)) {
    return;
  }
  FrameProperties properties = aFrame->Properties();
  LayerActivity* layerActivity =
    static_cast<LayerActivity*>(properties.Remove(LayerActivityProperty()));
  aFrame->RemoveStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY);
  if (!layerActivity) {
    return;
  }
  layerActivity->mFrame = nullptr;
  layerActivity->mContent = aContent;
  aContent->SetProperty(nsGkAtoms::LayerActivity, layerActivity,
                        nsINode::DeleteProperty<LayerActivity>, true);
}

 void
ActiveLayerTracker::TransferActivityToFrame(nsIContent* aContent, nsIFrame* aFrame)
{
  LayerActivity* layerActivity = static_cast<LayerActivity*>(
    aContent->UnsetProperty(nsGkAtoms::LayerActivity));
  if (!layerActivity) {
    return;
  }
  layerActivity->mContent = nullptr;
  layerActivity->mFrame = aFrame;
  aFrame->AddStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY);
  aFrame->Properties().Set(LayerActivityProperty(), layerActivity);
}

 void
ActiveLayerTracker::NotifyRestyle(nsIFrame* aFrame, nsCSSProperty aProperty)
{
  LayerActivity* layerActivity = GetLayerActivityForUpdate(aFrame);
  uint8_t& mutationCount = layerActivity->RestyleCountForProperty(aProperty);
  IncrementMutationCount(&mutationCount);
}

 void
ActiveLayerTracker::NotifyOffsetRestyle(nsIFrame* aFrame)
{
  LayerActivity* layerActivity = GetLayerActivityForUpdate(aFrame);
  IncrementMutationCount(&layerActivity->mLeftRestyleCount);
  IncrementMutationCount(&layerActivity->mTopRestyleCount);
  IncrementMutationCount(&layerActivity->mRightRestyleCount);
  IncrementMutationCount(&layerActivity->mBottomRestyleCount);
}

 void
ActiveLayerTracker::NotifyAnimated(nsIFrame* aFrame, nsCSSProperty aProperty)
{
  LayerActivity* layerActivity = GetLayerActivityForUpdate(aFrame);
  uint8_t& mutationCount = layerActivity->RestyleCountForProperty(aProperty);
  
  mutationCount = 0xFF;
}

static bool
IsPresContextInScriptAnimationCallback(nsPresContext* aPresContext)
{
  if (aPresContext->RefreshDriver()->IsInRefresh()) {
    return true;
  }
  
  
  nsPIDOMWindow* win = aPresContext->Document()->GetInnerWindow();
  return win && win->IsRunningTimeout();
}

 void
ActiveLayerTracker::NotifyInlineStyleRuleModified(nsIFrame* aFrame,
                                                  nsCSSProperty aProperty)
{
  if (!IsPresContextInScriptAnimationCallback(aFrame->PresContext())) {
    return;
  }
  NotifyAnimated(aFrame, aProperty);
}

 bool
ActiveLayerTracker::IsStyleMaybeAnimated(nsIFrame* aFrame, nsCSSProperty aProperty)
{
  return IsStyleAnimated(nullptr, aFrame, aProperty);
}

 bool
ActiveLayerTracker::IsStyleAnimated(nsDisplayListBuilder* aBuilder,
                                    nsIFrame* aFrame, nsCSSProperty aProperty)
{
  
  if ((aFrame->StyleDisplay()->mWillChangeBitField & NS_STYLE_WILL_CHANGE_TRANSFORM) &&
      aProperty == eCSSProperty_transform &&
      (!aBuilder || aBuilder->IsInWillChangeBudget(aFrame, aFrame->GetSize()))) {
    return true;
  }
  if ((aFrame->StyleDisplay()->mWillChangeBitField & NS_STYLE_WILL_CHANGE_OPACITY) &&
      aProperty == eCSSProperty_opacity &&
      (!aBuilder || aBuilder->IsInWillChangeBudget(aFrame, aFrame->GetSize()))) {
    return true;
  }

  LayerActivity* layerActivity = GetLayerActivity(aFrame);
  if (layerActivity) {
    if (layerActivity->RestyleCountForProperty(aProperty) >= 2) {
      return true;
    }
  }
  if (aProperty == eCSSProperty_transform && aFrame->Preserves3D()) {
    return IsStyleAnimated(aBuilder, aFrame->GetParent(), aProperty);
  }
  return nsLayoutUtils::HasCurrentAnimationsForProperties(aFrame, &aProperty, 1);
}

 bool
ActiveLayerTracker::IsOffsetOrMarginStyleAnimated(nsIFrame* aFrame)
{
  LayerActivity* layerActivity = GetLayerActivity(aFrame);
  if (layerActivity) {
    if (layerActivity->mLeftRestyleCount >= 2 ||
        layerActivity->mTopRestyleCount >= 2 ||
        layerActivity->mRightRestyleCount >= 2 ||
        layerActivity->mBottomRestyleCount >= 2 ||
        layerActivity->mMarginLeftRestyleCount >= 2 ||
        layerActivity->mMarginTopRestyleCount >= 2 ||
        layerActivity->mMarginRightRestyleCount >= 2 ||
        layerActivity->mMarginBottomRestyleCount >= 2) {
      return true;
    }
  }
  
  
  
  
  
  return false;
}

 void
ActiveLayerTracker::NotifyContentChange(nsIFrame* aFrame)
{
  LayerActivity* layerActivity = GetLayerActivityForUpdate(aFrame);
  layerActivity->mContentActive = true;
}

 bool
ActiveLayerTracker::IsContentActive(nsIFrame* aFrame)
{
  LayerActivity* layerActivity = GetLayerActivity(aFrame);
  return layerActivity && layerActivity->mContentActive;
}

 void
ActiveLayerTracker::Shutdown()
{
  delete gLayerActivityTracker;
  gLayerActivityTracker = nullptr;
}

} 

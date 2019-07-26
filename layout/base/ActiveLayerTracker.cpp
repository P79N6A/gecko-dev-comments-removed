



#include "ActiveLayerTracker.h"

#include "nsExpirationTracker.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsRefreshDriver.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"

namespace mozilla {











class LayerActivity {
public:
  LayerActivity(nsIFrame* aFrame)
    : mFrame(aFrame)
    , mOpacityRestyleCount(0)
    , mTransformRestyleCount(0)
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
    default: MOZ_ASSERT(false); return mOpacityRestyleCount;
    }
  }

  nsIFrame* mFrame;
  nsExpirationState mState;
  
  uint8_t mOpacityRestyleCount;
  uint8_t mTransformRestyleCount;
  uint8_t mLeftRestyleCount;
  uint8_t mTopRestyleCount;
  uint8_t mRightRestyleCount;
  uint8_t mBottomRestyleCount;
  bool mContentActive;
};

class LayerActivityTracker MOZ_FINAL : public nsExpirationTracker<LayerActivity,4> {
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
  if (mFrame) {
    NS_ASSERTION(gLayerActivityTracker, "Should still have a tracker");
    gLayerActivityTracker->RemoveObject(this);
  }
}

static void DestroyLayerActivity(void* aPropertyValue)
{
  delete static_cast<LayerActivity*>(aPropertyValue);
}


NS_DECLARE_FRAME_PROPERTY(LayerActivityProperty, DestroyLayerActivity)

void
LayerActivityTracker::NotifyExpired(LayerActivity* aObject)
{
  RemoveObject(aObject);

  nsIFrame* f = aObject->mFrame;
  aObject->mFrame = nullptr;

  f->SchedulePaint();
  f->RemoveStateBits(NS_FRAME_HAS_LAYER_ACTIVITY_PROPERTY);
  f->Properties().Delete(LayerActivityProperty());
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
ActiveLayerTracker::IsStyleAnimated(nsIFrame* aFrame, nsCSSProperty aProperty)
{
  LayerActivity* layerActivity = GetLayerActivity(aFrame);
  if (layerActivity) {
    if (layerActivity->RestyleCountForProperty(aProperty) >= 2) {
      return true;
    }
  }
  if (aProperty == eCSSProperty_transform && aFrame->Preserves3D()) {
    return IsStyleAnimated(aFrame->GetParent(), aProperty);
  }
  return false;
}

 bool
ActiveLayerTracker::IsOffsetStyleAnimated(nsIFrame* aFrame)
{
  LayerActivity* layerActivity = GetLayerActivity(aFrame);
  if (layerActivity) {
    if (layerActivity->mLeftRestyleCount >= 2 ||
        layerActivity->mTopRestyleCount >= 2 ||
        layerActivity->mRightRestyleCount >= 2 ||
        layerActivity->mBottomRestyleCount >= 2) {
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

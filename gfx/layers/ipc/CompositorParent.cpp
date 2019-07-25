







































#include "CompositorParent.h"
#include "RenderTrace.h"
#include "ShadowLayersParent.h"
#include "LayerManagerOGL.h"
#include "nsIWidget.h"
#include "nsGkAtoms.h"
#include "RenderTrace.h"

#if defined(MOZ_WIDGET_ANDROID)
#include "AndroidBridge.h"
#include <android/log.h>
#endif

using base::Thread;

namespace mozilla {
namespace layers {

CompositorParent::CompositorParent(nsIWidget* aWidget, base::Thread* aCompositorThread)
  : mCompositorThread(aCompositorThread)
  , mWidget(aWidget)
  , mCurrentCompositeTask(NULL)
  , mPaused(false)
  , mIsFirstPaint(false)
  , mLayersUpdated(false)
{
  MOZ_COUNT_CTOR(CompositorParent);
}

CompositorParent::~CompositorParent()
{
  MOZ_COUNT_DTOR(CompositorParent);
}

void
CompositorParent::Destroy()
{
  NS_ABORT_IF_FALSE(ManagedPLayersParent().Length() == 0,
                    "CompositorParent destroyed before managed PLayersParent");

  
  mLayerManager = NULL;
}

bool
CompositorParent::RecvStop()
{
  mPaused = true;
  Destroy();
  return true;
}

bool
CompositorParent::RecvPause()
{
  PauseComposition();
  return true;
}

bool
CompositorParent::RecvResume()
{
  ResumeComposition();
  return true;
}

void
CompositorParent::ScheduleRenderOnCompositorThread()
{
  CancelableTask *renderTask = NewRunnableMethod(this, &CompositorParent::ScheduleComposition);
  mCompositorThread->message_loop()->PostTask(FROM_HERE, renderTask);
}

void
CompositorParent::PauseComposition()
{
  NS_ABORT_IF_FALSE(mCompositorThread->thread_id() == PlatformThread::CurrentId(),
                    "PauseComposition() can only be called on the compositor thread");
  if (!mPaused) {
    mPaused = true;

#ifdef MOZ_WIDGET_ANDROID
    static_cast<LayerManagerOGL*>(mLayerManager.get())->gl()->ReleaseSurface();
#endif
  }
}

void
CompositorParent::ResumeComposition()
{
  NS_ABORT_IF_FALSE(mCompositorThread->thread_id() == PlatformThread::CurrentId(),
                    "ResumeComposition() can only be called on the compositor thread");
  mPaused = false;

#ifdef MOZ_WIDGET_ANDROID
  static_cast<LayerManagerOGL*>(mLayerManager.get())->gl()->RenewSurface();
#endif
}

void
CompositorParent::SchedulePauseOnCompositorThread()
{
  CancelableTask *pauseTask = NewRunnableMethod(this,
                                                &CompositorParent::PauseComposition);
  mCompositorThread->message_loop()->PostTask(FROM_HERE, pauseTask);
}

void
CompositorParent::ScheduleResumeOnCompositorThread()
{
  CancelableTask *resumeTask = NewRunnableMethod(this,
                                                 &CompositorParent::ResumeComposition);
  mCompositorThread->message_loop()->PostTask(FROM_HERE, resumeTask);
}

void
CompositorParent::ScheduleComposition()
{
  if (mCurrentCompositeTask) {
    return;
  }

  bool initialComposition = mLastCompose.IsNull();
  TimeDuration delta;
  if (!initialComposition)
    delta = mozilla::TimeStamp::Now() - mLastCompose;

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  mExpectedComposeTime = mozilla::TimeStamp::Now() + TimeDuration::FromMilliseconds(15);
#endif

  mCurrentCompositeTask = NewRunnableMethod(this, &CompositorParent::Composite);

  
  
  if (!initialComposition && delta.ToMilliseconds() < 15) {
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeTime = mozilla::TimeStamp::Now() + TimeDuration::FromMilliseconds(15 - delta.ToMilliseconds());
#endif
    MessageLoop::current()->PostDelayedTask(FROM_HERE, mCurrentCompositeTask, 15 - delta.ToMilliseconds());
  } else {
    MessageLoop::current()->PostTask(FROM_HERE, mCurrentCompositeTask);
  }
}

void
CompositorParent::SetTransformation(float aScale, nsIntPoint aScrollOffset)
{
  mXScale = aScale;
  mYScale = aScale;
  mScrollOffset = aScrollOffset;
}

void
CompositorParent::Composite()
{
  NS_ABORT_IF_FALSE(mCompositorThread->thread_id() == PlatformThread::CurrentId(),
                    "Composite can only be called on the compositor thread");
  mCurrentCompositeTask = NULL;

  mLastCompose = mozilla::TimeStamp::Now();

  if (mPaused || !mLayerManager || !mLayerManager->GetRoot()) {
    return;
  }

#ifdef MOZ_WIDGET_ANDROID
  TransformShadowTree();
#endif

  Layer* aLayer = mLayerManager->GetRoot();
  mozilla::layers::RenderTraceLayers(aLayer, "0000");

  mLayerManager->EndEmptyTransaction();

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  if (mExpectedComposeTime + TimeDuration::FromMilliseconds(15) < mozilla::TimeStamp::Now()) {
    printf_stderr("Compositor: Composite took %i ms.\n",
                  15 + (int)(mozilla::TimeStamp::Now() - mExpectedComposeTime).ToMilliseconds());
  }
#endif
}



static void
SetShadowProperties(Layer* aLayer)
{
  
  ShadowLayer* shadow = aLayer->AsShadowLayer();
  shadow->SetShadowTransform(aLayer->GetTransform());
  shadow->SetShadowVisibleRegion(aLayer->GetVisibleRegion());
  shadow->SetShadowClipRect(aLayer->GetClipRect());

  for (Layer* child = aLayer->GetFirstChild();
      child; child = child->GetNextSibling()) {
    SetShadowProperties(child);
  }
}

#ifdef MOZ_WIDGET_ANDROID


Layer*
CompositorParent::GetPrimaryScrollableLayer()
{
  Layer* root = mLayerManager->GetRoot();

  
  
  
  
  
  Layer* discardLayer = root->GetFirstChild();

  while (discardLayer) {
    if (!discardLayer->AsContainerLayer()) {
      discardLayer->IntersectClipRect(nsIntRect());
      SetShadowProperties(discardLayer);
    }
    discardLayer = discardLayer->GetNextSibling();
  }

  nsTArray<Layer*> queue;
  queue.AppendElement(root);
  while (queue.Length()) {
    ContainerLayer* containerLayer = queue[0]->AsContainerLayer();
    queue.RemoveElementAt(0);
    if (!containerLayer) {
      continue;
    }

    const FrameMetrics& frameMetrics = containerLayer->GetFrameMetrics();
    if (frameMetrics.IsScrollable()) {
      return containerLayer;
    }

    Layer* child = containerLayer->GetFirstChild();
    while (child) {
      queue.AppendElement(child);
      child = child->GetNextSibling();
    }
  }

  return root;
}
#endif

void
CompositorParent::TransformShadowTree()
{
#ifdef MOZ_WIDGET_ANDROID
  Layer* layer = GetPrimaryScrollableLayer();
  ShadowLayer* shadow = layer->AsShadowLayer();
  ContainerLayer* container = layer->AsContainerLayer();

  const FrameMetrics* metrics = &container->GetFrameMetrics();
  const gfx3DMatrix& rootTransform = mLayerManager->GetRoot()->GetTransform();
  const gfx3DMatrix& currentTransform = layer->GetTransform();

  float rootScaleX = rootTransform.GetXScale();
  float rootScaleY = rootTransform.GetYScale();

  if (mIsFirstPaint && metrics) {
    nsIntPoint scrollOffset = metrics->mViewportScrollOffset;
    mContentSize = metrics->mContentSize;
    mozilla::AndroidBridge::Bridge()->SetFirstPaintViewport(scrollOffset.x, scrollOffset.y,
                                                            1/rootScaleX, mContentSize.width,
                                                            mContentSize.height);
    mIsFirstPaint = false;
  } else if (metrics && (metrics->mContentSize != mContentSize)) {
    mContentSize = metrics->mContentSize;
    mozilla::AndroidBridge::Bridge()->SetPageSize(1/rootScaleX, mContentSize.width,
                                                  mContentSize.height);
  }

  
  
  if (metrics) {
    
    nsIntRect displayPort = metrics->mDisplayPort;
    nsIntPoint scrollOffset = metrics->mViewportScrollOffset;
    displayPort.x += scrollOffset.x;
    displayPort.y += scrollOffset.y;

    mozilla::AndroidBridge::Bridge()->SyncViewportInfo(displayPort, 1/rootScaleX, mLayersUpdated,
                                                       mScrollOffset, mXScale, mYScale);
    mLayersUpdated = false;
  }

  
  
  
  
  
  
  if (metrics && metrics->IsScrollable()) {
    float tempScaleDiffX = rootScaleX * mXScale;
    float tempScaleDiffY = rootScaleY * mYScale;

    nsIntPoint metricsScrollOffset = metrics->mViewportScrollOffset;

    nsIntPoint scrollCompensation(
      (mScrollOffset.x / tempScaleDiffX - metricsScrollOffset.x) * mXScale,
      (mScrollOffset.y / tempScaleDiffY - metricsScrollOffset.y) * mYScale);
    ViewTransform treeTransform(-scrollCompensation, mXScale, mYScale);
    shadow->SetShadowTransform(gfx3DMatrix(treeTransform) * currentTransform);
  } else {
    ViewTransform treeTransform(nsIntPoint(0,0), mXScale, mYScale);
    shadow->SetShadowTransform(gfx3DMatrix(treeTransform) * currentTransform);
  }
#endif
}

void
CompositorParent::ShadowLayersUpdated(bool isFirstPaint)
{
  mIsFirstPaint = mIsFirstPaint || isFirstPaint;
  mLayersUpdated = true;
  const nsTArray<PLayersParent*>& shadowParents = ManagedPLayersParent();
  NS_ABORT_IF_FALSE(shadowParents.Length() <= 1,
                    "can only support at most 1 ShadowLayersParent");
  if (shadowParents.Length()) {
    Layer* root = static_cast<ShadowLayersParent*>(shadowParents[0])->GetRoot();
    mLayerManager->SetRoot(root);
    SetShadowProperties(root);
  }
  ScheduleComposition();
}

PLayersParent*
CompositorParent::AllocPLayers(const LayersBackend &backendType)
{
  if (backendType == LayerManager::LAYERS_OPENGL) {
    nsRefPtr<LayerManagerOGL> layerManager = new LayerManagerOGL(mWidget);
    mWidget = NULL;
    mLayerManager = layerManager;

    if (!layerManager->Initialize()) {
      NS_ERROR("Failed to init OGL Layers");
      return NULL;
    }

    ShadowLayerManager* slm = layerManager->AsShadowManager();
    if (!slm) {
      return NULL;
    }
    return new ShadowLayersParent(slm, this);
  } else {
    NS_ERROR("Unsupported backend selected for Async Compositor");
    return NULL;
  }
}

bool
CompositorParent::DeallocPLayers(PLayersParent* actor)
{
  delete actor;
  return true;
}

} 
} 










































#include "CompositorParent.h"
#include "ShadowLayersParent.h"
#include "LayerManagerOGL.h"
#include "nsIWidget.h"

#if defined(MOZ_WIDGET_ANDROID)
#include "AndroidBridge.h"
#endif

namespace mozilla {
namespace layers {

CompositorParent::CompositorParent(nsIWidget* aWidget)
  : mStopped(false), mWidget(aWidget)
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
  mStopped = true;
  Destroy();
  return true;
}


void
CompositorParent::ScheduleComposition()
{
  printf_stderr("Schedule composition\n");
  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  MessageLoop::current()->PostTask(FROM_HERE, composeTask);


#ifdef OMTC_TEST_ASYNC_SCROLLING
  static bool scrollScheduled = false;
  if (!scrollScheduled) {
    CancelableTask *composeTask2 = NewRunnableMethod(this,
                                                     &CompositorParent::TestScroll);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, composeTask2, 500);
    scrollScheduled = true;
  }
#endif
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
  if (mStopped || !mLayerManager) {
    return;
  }

  mLayerManager->EndEmptyTransaction();
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

static double GetXScale(const gfx3DMatrix& aTransform)
{
  return aTransform._11;
}

static double GetYScale(const gfx3DMatrix& aTransform)
{
  return aTransform._22;
}

static void ReverseTranslate(gfx3DMatrix& aTransform, ViewTransform& aViewTransform)
{
  aTransform._41 -= aViewTransform.mTranslation.x / aViewTransform.mXScale;
  aTransform._42 -= aViewTransform.mTranslation.y / aViewTransform.mYScale;
}

void
CompositorParent::TransformShadowTree(Layer* aLayer, const ViewTransform& aTransform,
                    float aTempScaleDiffX, float aTempScaleDiffY)
{
  ShadowLayer* shadow = aLayer->AsShadowLayer();

  gfx3DMatrix shadowTransform = aLayer->GetTransform();
  ViewTransform layerTransform = aTransform;

  ContainerLayer* container = aLayer->AsContainerLayer();

  if (container && container->GetFrameMetrics().IsScrollable()) {
    const FrameMetrics* metrics = &container->GetFrameMetrics();
    const gfx3DMatrix& currentTransform = aLayer->GetTransform();

    aTempScaleDiffX *= GetXScale(shadowTransform);
    aTempScaleDiffY *= GetYScale(shadowTransform);

    nsIntPoint metricsScrollOffset = metrics->mViewportScrollOffset;

    nsIntPoint scrollCompensation(
        (mScrollOffset.x / aTempScaleDiffX - metricsScrollOffset.x) * mXScale,
        (mScrollOffset.y / aTempScaleDiffY - metricsScrollOffset.y) * mYScale);
    ViewTransform treeTransform(-scrollCompensation, mXScale,
                                mYScale);
    shadowTransform = gfx3DMatrix(treeTransform) * currentTransform;
    layerTransform = treeTransform;
  }

  
  












  shadow->SetShadowTransform(shadowTransform);

  
  
  
  






}

void
CompositorParent::AsyncRender()
{
  if (mStopped || !mLayerManager) {
    return;
  }

  Layer* root = mLayerManager->GetRoot();
  ContainerLayer* container = root->AsContainerLayer();
  if (!container)
    return;

  FrameMetrics metrics = container->GetFrameMetrics();













    
  metrics.mScrollId = FrameMetrics::ROOT_SCROLL_ID;
  container->SetFrameMetrics(metrics);
  ViewTransform transform;
  TransformShadowTree(root, transform);
  Composite();
}

void
CompositorParent::ShadowLayersUpdated()
{
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


#ifdef OMTC_TEST_ASYNC_SCROLLING
void
CompositorParent::TestScroll()
{
  static int scrollFactor = 0;
  static bool fakeScrollDownwards = true;
  if (fakeScrollDownwards) {
    scrollFactor++;
    if (scrollFactor > 10) {
      scrollFactor = 10;
      fakeScrollDownwards = false;
    }
  } else {
    scrollFactor--;
    if (scrollFactor < 0) {
      scrollFactor = 0;
      fakeScrollDownwards = true;
    }
  }
  SetTransformation(1.0+2.0*scrollFactor/10, nsIntPoint(-25*scrollFactor,
      -25*scrollFactor));
  printf_stderr("AsyncRender scroll factor:%d\n", scrollFactor);
  AsyncRender();

  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::TestScroll);
  MessageLoop::current()->PostDelayedTask(FROM_HERE, composeTask, 1000/65);
}
#endif

PLayersParent*
CompositorParent::AllocPLayers(const LayersBackend &backendType)
{
#ifdef MOZ_WIDGET_ANDROID
  
  
  
#endif

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

#ifdef MOZ_WIDGET_ANDROID
void
CompositorParent::RegisterCompositorWithJava()
{
  mozilla::AndroidBridge::Bridge()->RegisterCompositor();
}
#endif

} 
} 


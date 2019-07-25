






































#include "CompositorParent.h"
#include "ShadowLayersParent.h"
#include "LayerManagerOGL.h"

namespace mozilla {
namespace layers {

CompositorParent::CompositorParent()
  : mLayerManager(NULL)
{

  MOZ_COUNT_CTOR(CompositorParent);
}

CompositorParent::~CompositorParent()
{
  printf("delete parent\n");
  delete mLayerManager;
  MOZ_COUNT_DTOR(CompositorParent);
}

bool
CompositorParent::AnswerInit()
{
  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  MessageLoop::current()->PostTask(FROM_HERE, composeTask);
  return true;
}

bool
CompositorParent::AnswerStop()
{
  if (mLayerManager) {
    delete mLayerManager;
    mLayerManager = NULL;
  }
  return true;
}

void
CompositorParent::RequestComposition()
{
  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  MessageLoop::current()->PostTask(FROM_HERE, composeTask);
}

void
CompositorParent::Composite()
{
  if (!mLayerManager) {
    CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
    MessageLoop::current()->PostDelayedTask(FROM_HERE, composeTask, 10);
    return;
  }

  mLayerManager->EndEmptyTransaction();
}

PLayersParent*
CompositorParent::AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget)
{
  if (widget.type() != WidgetDescriptor::TViewWidget) {
    NS_ERROR("Invalid widget descriptor\n");
    return NULL;
  }

  if (backend == LayerManager::LAYERS_OPENGL) {
    nsIWidget *nsWidget = (nsIWidget*)widget.get_ViewWidget().widgetPtr();
    nsRefPtr<LayerManagerOGL> layerManager = new LayerManagerOGL(nsWidget);

    if (!layerManager->Initialize()) {
      NS_ERROR("Failed to init OGL Layers");
      return NULL;
    }

    ShadowLayerManager* slm = layerManager->AsShadowManager();
    if (!slm) {
      return NULL;
    }

    void *glContext = layerManager->gl()->GetNativeData(mozilla::gl::GLContext::NativeGLContext);
    NativeContext nativeContext = NativeContext((uintptr_t)glContext);
    SendNativeContextCreated(nativeContext);

    mLayerManager = layerManager;

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


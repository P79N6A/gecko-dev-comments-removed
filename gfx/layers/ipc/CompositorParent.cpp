






































#include "CompositorParent.h"
#include "ShadowLayersParent.h"
#include "LayerManagerOGL.h"

namespace mozilla {
namespace layers {

CompositorParent::CompositorParent()
{

  MOZ_COUNT_CTOR(CompositorParent);
}

CompositorParent::~CompositorParent()
{
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

void
CompositorParent::Composite()
{
  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  MessageLoop::current()->PostDelayedTask(FROM_HERE, composeTask, 10);

  if (!mLayerManager)
    return;

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


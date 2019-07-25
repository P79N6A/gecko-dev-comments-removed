






































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
  MOZ_COUNT_DTOR(CompositorParent);
}

bool
CompositorParent::AnswerInit()
{
  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  MessageLoop::current()->PostTask(FROM_HERE, composeTask);
  return true;
}

LayerManagerOGL* lm = NULL;

void
CompositorParent::Composite()
{
  CancelableTask *composeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  MessageLoop::current()->PostTask(FROM_HERE, composeTask);
  if (lm) {
    lm->EndEmptyTransaction();
  }
}

PLayersParent*
CompositorParent::AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget)
{
  if (widget.type() != WidgetDescriptor::TMacChildViewWidget) {
    NS_ERROR("Invalid widget descriptor\n");
    return NULL;
  }

  if (backend == LayerManager::LAYERS_OPENGL) {
    nsRefPtr<LayerManagerOGL> layerManager = new
      LayerManagerOGL((nsIWidget*)widget.get_MacChildViewWidget().widgetPtr());
    lm = layerManager;
    if (!layerManager->Initialize()) {
      NS_ERROR("Failed to init OGL Layers");
      return NULL;
    }

    ShadowLayerManager* slm = layerManager->AsShadowManager();
    if (!slm) {
      return NULL;
    }

    return new ShadowLayersParent(slm);
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


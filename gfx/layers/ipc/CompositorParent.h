







































#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h

#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayersParent.h"
#include "ShadowLayersHost.h"

class LayerManager;

namespace mozilla {
namespace layers {

class CompositorParent : public PCompositorParent,
                         public ShadowLayersHost
{
  NS_INLINE_DECL_REFCOUNTING(CompositorParent)
public:
  CompositorParent();
  virtual ~CompositorParent();

  bool RecvInit();
  bool RecvStop();

  void RequestComposition();

  virtual mozilla::layout::RenderFrameParent* GetRenderFrameParent() { return NULL; }
  virtual CompositorParent* GetCompositorParent() { return this; }

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget);

  virtual bool DeallocPLayers(PLayersParent* aLayers);

private:
  void Composite();

  nsRefPtr<LayerManager> mLayerManager;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 

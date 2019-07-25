






































#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "mozilla/layers/PCompositorChild.h"

namespace mozilla {
namespace layers {

class LayerManager;
class CompositorParent;

class CompositorChild : public PCompositorChild
{
  NS_INLINE_DECL_REFCOUNTING(CompositorChild)
public:
  CompositorChild(LayerManager *aLayerManager);
  virtual ~CompositorChild();

  void Destroy();

protected:
  virtual PLayersChild* AllocPLayers(const LayersBackend &aBackend);
  virtual bool DeallocPLayers(PLayersChild *aChild);

private:
  nsRefPtr<LayerManager> mLayerManager;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 

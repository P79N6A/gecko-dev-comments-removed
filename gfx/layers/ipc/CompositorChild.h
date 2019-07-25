






































#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "mozilla/layers/PCompositorChild.h"

namespace mozilla {
namespace layers {

class CompositorChild : public PCompositorChild
{

public:
  CompositorChild();
  virtual ~CompositorChild();

protected:
  virtual PLayersChild* AllocPLayers(const LayersBackend &backend);
  virtual bool DeallocPLayers(PLayersChild *aChild);
};

} 
} 

#endif 

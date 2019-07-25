







































#ifndef mozilla_layers_ShadowLayersChild_h
#define mozilla_layers_ShadowLayersChild_h

#include "mozilla/layers/PLayersChild.h"

namespace mozilla {
namespace layers {

class ShadowLayersChild : public PLayersChild
{
public:
  ShadowLayersChild() { }
  ~ShadowLayersChild() { }

  






  void Destroy();

protected:
  NS_OVERRIDE virtual PLayerChild* AllocPLayer();
  NS_OVERRIDE virtual bool DeallocPLayer(PLayerChild* actor);
};

} 
} 

#endif 









































#ifndef mozilla_layers_ShadowLayersChild_h
#define mozilla_layers_ShadowLayersChild_h

#include "mozilla/layers/PLayersChild.h"
#include "mozilla/layers/ShadowLayerChild.h"

namespace mozilla {
namespace layers {

class ShadowLayersChild : public PLayersChild
{
public:
  ShadowLayersChild() { }
  ~ShadowLayersChild() { }

protected:
  NS_OVERRIDE virtual PLayerChild* AllocPLayer() {
    
    NS_RUNTIMEABORT("not reached");
    return NULL;
  }

  NS_OVERRIDE virtual bool DeallocPLayer(PLayerChild* actor) {
    delete actor;
    return true;
  }
};

} 
} 

#endif 








#ifndef mozilla_dom_RenderFrameChild_h
#define mozilla_dom_RenderFrameChild_h

#include "mozilla/layout/PRenderFrameChild.h"

namespace mozilla {
namespace layout {

class RenderFrameChild : public PRenderFrameChild
{
public:
  RenderFrameChild() {}
  virtual ~RenderFrameChild() {}

  void Destroy();

protected:
  NS_OVERRIDE
  virtual PLayersChild* AllocPLayers(LayerManager::LayersBackend* aBackendType,
                                     int* aMaxTextureSize, uint64_t* aId);
  NS_OVERRIDE
  virtual bool DeallocPLayers(PLayersChild* aLayers);
};

} 
} 

#endif  

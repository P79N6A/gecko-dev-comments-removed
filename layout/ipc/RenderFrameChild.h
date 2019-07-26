






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
  virtual PLayerTransactionChild* AllocPLayerTransactionChild() MOZ_OVERRIDE;
  virtual bool DeallocPLayerTransactionChild(PLayerTransactionChild* aLayers) MOZ_OVERRIDE;
};

} 
} 

#endif  

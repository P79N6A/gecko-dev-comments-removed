






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

  void CancelDefaultPanZoom();
  void DetectScrollableSubframe();

  void Destroy();

protected:
  virtual PLayerTransactionChild* AllocPLayerTransaction() MOZ_OVERRIDE;
  virtual bool DeallocPLayerTransaction(PLayerTransactionChild* aLayers) MOZ_OVERRIDE;
};

} 
} 

#endif  

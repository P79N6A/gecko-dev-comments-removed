






#include "RenderFrameChild.h"
#include "mozilla/layers/LayerTransactionChild.h"

using mozilla::layers::PLayerTransactionChild;
using mozilla::layers::LayerTransactionChild;

namespace mozilla {
namespace layout {

void
RenderFrameChild::Destroy()
{
  size_t numChildren = ManagedPLayerTransactionChild().Length();
  NS_ABORT_IF_FALSE(0 == numChildren || 1 == numChildren,
                    "render frame must only have 0 or 1 layer forwarder");

  if (numChildren) {
    LayerTransactionChild* layers =
      static_cast<LayerTransactionChild*>(ManagedPLayerTransactionChild()[0]);
    layers->Destroy();
    
  }

  Send__delete__(this);
  
}

PLayerTransactionChild*
RenderFrameChild::AllocPLayerTransactionChild()
{
  return new LayerTransactionChild();
}

bool
RenderFrameChild::DeallocPLayerTransactionChild(PLayerTransactionChild* aLayers)
{
  delete aLayers;
  return true;
}

}  
}  

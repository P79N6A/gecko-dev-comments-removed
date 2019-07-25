







































#include "RenderFrameChild.h"
#include "mozilla/layers/ShadowLayersChild.h"

using mozilla::layers::PLayersChild;
using mozilla::layers::ShadowLayersChild;

namespace mozilla {
namespace layout {

void
RenderFrameChild::Destroy()
{
  size_t numChildren = ManagedPLayersChild().Length();
  NS_ABORT_IF_FALSE(0 == numChildren || 1 == numChildren,
                    "render frame must only have 0 or 1 layer forwarder");

  if (numChildren) {
    ShadowLayersChild* layers =
      static_cast<ShadowLayersChild*>(ManagedPLayersChild()[0]);
    layers->Destroy();
    
  }

  Send__delete__(this);
  
}

PLayersChild*
RenderFrameChild::AllocPLayers()
{
  return new ShadowLayersChild();
}

bool
RenderFrameChild::DeallocPLayers(PLayersChild* aLayers)
{
  delete aLayers;
  return true;
}

}  
}  

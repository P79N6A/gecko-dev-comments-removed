







































#include "ShadowLayerChild.h"
#include "ShadowLayersChild.h"

namespace mozilla {
namespace layers {

void
ShadowLayersChild::Destroy()
{
  NS_ABORT_IF_FALSE(0 == ManagedPLayerChild().Length(),
                    "layers should have been cleaned up by now");
  
  
  
  
  
}

PLayerChild*
ShadowLayersChild::AllocPLayer()
{
  
  NS_RUNTIMEABORT("not reached");
  return NULL;
}

bool
ShadowLayersChild::DeallocPLayer(PLayerChild* actor)
{
  delete actor;
  return true;
}

}  
}  

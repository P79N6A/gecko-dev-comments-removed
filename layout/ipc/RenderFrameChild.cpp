






#include "RenderFrameChild.h"
#include "mozilla/layers/LayerTransactionChild.h"

using mozilla::layers::PLayerTransactionChild;
using mozilla::layers::LayerTransactionChild;

namespace mozilla {
namespace layout {

void
RenderFrameChild::ActorDestroy(ActorDestroyReason why)
{
  mWasDestroyed = true;
}

void
RenderFrameChild::Destroy()
{
  if (mWasDestroyed) {
    return;
  }

  Send__delete__(this);
  
}

} 
} 

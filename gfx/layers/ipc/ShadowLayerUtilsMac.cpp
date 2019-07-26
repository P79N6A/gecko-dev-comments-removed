






#include "mozilla/gfx/Point.h"
#include "mozilla/layers/PLayerTransaction.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/CompositorTypes.h"

#include "gfx2DGlue.h"
#include "gfxPlatform.h"

#include "gfxSharedQuartzSurface.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
}

 void
LayerManagerComposite::PlatformSyncBeforeReplyUpdate()
{
}

 bool
LayerManagerComposite::SupportsDirectTexturing()
{
  return false;
}


} 
} 

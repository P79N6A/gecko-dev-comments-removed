




#include "PersistentBufferProvider.h"

#include "Layers.h"
#include "mozilla/gfx/Logging.h"
#include "pratom.h"
#include "gfxPlatform.h"

namespace mozilla {

using namespace gfx;

namespace layers {

PersistentBufferProviderBasic::PersistentBufferProviderBasic(LayerManager* aManager, gfx::IntSize aSize,
                                                             gfx::SurfaceFormat aFormat, gfx::BackendType aBackend)
{
  mDrawTarget = gfxPlatform::GetPlatform()->CreateDrawTargetForBackend(aBackend, aSize, aFormat);
}

}
}
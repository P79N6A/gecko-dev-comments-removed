




#include "mozilla/layers/PLayerTransactionParent.h"
#include "BasicCanvasLayer.h"
#include "gfxImageSurface.h"
#include "GLContext.h"
#include "gfxUtils.h"
#include "gfxPlatform.h"
#include "mozilla/Preferences.h"
#include "BasicLayersImpl.h"
#include "SurfaceStream.h"
#include "SharedSurfaceGL.h"
#include "SharedSurfaceEGL.h"
#ifdef MOZ_B2G
#include "SharedSurfaceGralloc.h"
#endif
#include "GeckoProfiler.h"

#include "nsXULAppAPI.h"

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {

void
BasicCanvasLayer::Paint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (IsHidden())
    return;

  FirePreTransactionCallback();
  UpdateSurface();
  FireDidTransactionCallback();

  PaintWithOpacity(aContext, GetEffectiveOpacity(), aMaskLayer, GetOperator());
}

already_AddRefed<CanvasLayer>
BasicLayerManager::CreateCanvasLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<CanvasLayer> layer = new BasicCanvasLayer(this);
  return layer.forget();
}

}
}

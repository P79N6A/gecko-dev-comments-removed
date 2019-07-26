




#include "BasicCanvasLayer.h"
#include "basic/BasicLayers.h"          
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "gfx2DGlue.h"

class gfxContext;

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {

void
BasicCanvasLayer::Paint(DrawTarget* aTarget, SourceSurface* aMaskSurface)
{
  if (IsHidden())
    return;

  FirePreTransactionCallback();
  UpdateTarget();
  FireDidTransactionCallback();

  CompositionOp mixBlendMode = GetEffectiveMixBlendMode();
  PaintWithOpacity(aTarget,
                   GetEffectiveOpacity(),
                   aMaskSurface,
                   mixBlendMode != CompositionOp::OP_OVER ? mixBlendMode : GetOperator());
}

void
BasicCanvasLayer::DeprecatedPaint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (IsHidden())
    return;

  FirePreTransactionCallback();
  DeprecatedUpdateSurface();
  FireDidTransactionCallback();

  gfxContext::GraphicsOperator mixBlendMode = DeprecatedGetEffectiveMixBlendMode();
  DeprecatedPaintWithOpacity(aContext,
                             GetEffectiveOpacity(),
                             aMaskLayer,
                             mixBlendMode != gfxContext::OPERATOR_OVER ?
                               mixBlendMode :
                               DeprecatedGetOperator());
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

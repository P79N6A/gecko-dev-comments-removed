




#include "BasicCanvasLayer.h"
#include "basic/BasicLayers.h"          
#include "basic/BasicLayersImpl.h"      
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
BasicCanvasLayer::Paint(DrawTarget* aDT,
                        const Point& aDeviceOffset,
                        Layer* aMaskLayer)
{
  if (IsHidden())
    return;

  if (IsDirty()) {
    Painted();

    FirePreTransactionCallback();
    UpdateTarget();
    FireDidTransactionCallback();
  }

  if (!mSurface) {
    return;
  }

  const bool needsYFlip = (mOriginPos == gl::OriginPos::BottomLeft);

  Matrix oldTM;
  if (needsYFlip) {
    oldTM = aDT->GetTransform();
    aDT->SetTransform(Matrix(oldTM).
                        PreTranslate(0.0f, mBounds.height).
                        PreScale(1.0f, -1.0f));
  }

  FillRectWithMask(aDT, aDeviceOffset,
                   Rect(0, 0, mBounds.width, mBounds.height),
                   mSurface, ToFilter(mFilter),
                   DrawOptions(GetEffectiveOpacity(), GetEffectiveOperator(this)),
                   aMaskLayer);

  if (needsYFlip) {
    aDT->SetTransform(oldTM);
  }
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

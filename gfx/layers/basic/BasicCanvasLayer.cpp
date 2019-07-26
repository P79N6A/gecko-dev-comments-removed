




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
BasicCanvasLayer::Paint(DrawTarget* aDT, Layer* aMaskLayer)
{
  if (IsHidden())
    return;

  FirePreTransactionCallback();
  UpdateTarget();
  FireDidTransactionCallback();

  Matrix m;
  if (mNeedsYFlip) {
    m = aDT->GetTransform();
    Matrix newTransform = m;
    newTransform.Translate(0.0f, mBounds.height);
    newTransform.Scale(1.0f, -1.0f);
    aDT->SetTransform(newTransform);
  }

  FillRectWithMask(aDT,
                   Rect(0, 0, mBounds.width, mBounds.height),
                   mSurface, ToFilter(mFilter),
                   DrawOptions(GetEffectiveOpacity(), GetEffectiveOperator(this)),
                   aMaskLayer);

  if (mNeedsYFlip) {
    aDT->SetTransform(m);
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

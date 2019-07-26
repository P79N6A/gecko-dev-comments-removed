




#include "BasicLayersImpl.h"            
#include "Layers.h"                     
#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "gfxContext.h"                 
#include "gfxRect.h"                    
#include "gfx2DGlue.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class BasicColorLayer : public ColorLayer, public BasicImplData {
public:
  BasicColorLayer(BasicLayerManager* aLayerManager) :
    ColorLayer(aLayerManager,
               static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(BasicColorLayer);
  }
  virtual ~BasicColorLayer()
  {
    MOZ_COUNT_DTOR(BasicColorLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ColorLayer::SetVisibleRegion(aRegion);
  }

  virtual void DeprecatedPaint(gfxContext* aContext, Layer* aMaskLayer) MOZ_OVERRIDE
  {
    if (IsHidden()) {
      return;
    }

    gfxRect snapped(mBounds.x, mBounds.y, mBounds.width, mBounds.height);
    if (aContext->UserToDevicePixelSnapped(snapped, true)) {
      gfxMatrix mat = aContext->CurrentMatrix();
      mat.Invert();
      snapped = mat.TransformBounds(snapped);
    }

    FillRectWithMask(aContext->GetDrawTarget(),
                     Rect(snapped.x, snapped.y, snapped.width, snapped.height),
                     ToColor(mColor),
                     DrawOptions(GetEffectiveOpacity(), GetEffectiveOperator(this)),
                     aMaskLayer);
  }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

already_AddRefed<ColorLayer>
BasicLayerManager::CreateColorLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ColorLayer> layer = new BasicColorLayer(this);
  return layer.forget();
}

}
}

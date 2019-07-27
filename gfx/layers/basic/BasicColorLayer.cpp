




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
#include "mozilla/gfx/PathHelpers.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class BasicColorLayer : public ColorLayer, public BasicImplData {
public:
  explicit BasicColorLayer(BasicLayerManager* aLayerManager) :
    ColorLayer(aLayerManager,
               static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(BasicColorLayer);
  }

protected:
  virtual ~BasicColorLayer()
  {
    MOZ_COUNT_DTOR(BasicColorLayer);
  }

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ColorLayer::SetVisibleRegion(aRegion);
  }

  virtual void Paint(DrawTarget* aDT,
                     const gfx::Point& aDeviceOffset,
                     Layer* aMaskLayer) MOZ_OVERRIDE
  {
    if (IsHidden()) {
      return;
    }

    Rect snapped(mBounds.x, mBounds.y, mBounds.width, mBounds.height);
    if (UserToDevicePixelSnapped(snapped, aDT->GetTransform())) {
      Matrix mat = aDT->GetTransform();
      mat.Invert();
      snapped = mat.TransformBounds(snapped);
    }

    FillRectWithMask(aDT, aDeviceOffset, snapped, ToColor(mColor),
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

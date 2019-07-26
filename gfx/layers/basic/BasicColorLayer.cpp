




#include "BasicLayersImpl.h"            
#include "Layers.h"                     
#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "gfxContext.h"                 
#include "gfxRect.h"                    
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              

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

  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer)
  {
    if (IsHidden())
      return;
    gfxContextAutoSaveRestore contextSR(aContext);
    gfxContext::GraphicsOperator mixBlendMode = GetEffectiveMixBlendMode();
    AutoSetOperator setOptimizedOperator(aContext, mixBlendMode != gfxContext::OPERATOR_OVER ? mixBlendMode : GetOperator());

    aContext->SetColor(mColor);

    nsIntRect bounds = GetBounds();
    aContext->NewPath();
    aContext->SnappedRectangle(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height));

    FillWithMask(aContext, GetEffectiveOpacity(), aMaskLayer);
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

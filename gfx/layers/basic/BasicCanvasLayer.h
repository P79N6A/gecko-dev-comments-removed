




#ifndef GFX_BASICCANVASLAYER_H
#define GFX_BASICCANVASLAYER_H

#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "CopyableCanvasLayer.h"        
#include "Layers.h"                     
#include "nsDebug.h"                    
#include "nsRegion.h"                   

class gfxContext;

namespace mozilla {
namespace layers {

class BasicCanvasLayer : public CopyableCanvasLayer,
                         public BasicImplData
{
public:
  explicit BasicCanvasLayer(BasicLayerManager* aLayerManager) :
    CopyableCanvasLayer(aLayerManager,
                        static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  { }
  
  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    CanvasLayer::SetVisibleRegion(aRegion);
  }
  
  virtual void Paint(gfx::DrawTarget* aDT,
                     const gfx::Point& aDeviceOffset,
                     Layer* aMaskLayer) MOZ_OVERRIDE;
 
protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

}
}

#endif

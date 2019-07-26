




#ifndef GFX_BASICCANVASLAYER_H
#define GFX_BASICCANVASLAYER_H

#include "BasicLayersImpl.h"
#include "nsXULAppAPI.h"
#include "BasicLayers.h"
#include "BasicImplData.h"
#include "mozilla/layers/CanvasClient.h"
#include "mozilla/Preferences.h"
#include "CopyableCanvasLayer.h"

#include "gfxPlatform.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class CanvasClient2D;
class CanvasClientWebGL;

class BasicCanvasLayer : public CopyableCanvasLayer,
                         public BasicImplData
{
public:
  BasicCanvasLayer(BasicLayerManager* aLayerManager) :
    CopyableCanvasLayer(aLayerManager,
                        static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  { }
  
  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    CanvasLayer::SetVisibleRegion(aRegion);
  }
  
  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer);
 
protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

}
}

#endif

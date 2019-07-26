




#include "ClientLayerManager.h"         
#include "Layers.h"                     
#include "mozilla/layers/LayerTransaction.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class ClientColorLayer : public ColorLayer, 
                         public ClientLayer {
public:
  ClientColorLayer(ClientLayerManager* aLayerManager) :
    ColorLayer(aLayerManager,
               static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(ClientColorLayer);
  }
  virtual ~ClientColorLayer()
  {
    MOZ_COUNT_DTOR(ClientColorLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ColorLayer::SetVisibleRegion(aRegion);
  }

  virtual void RenderLayer()
  {
    if (GetMaskLayer()) {
      ToClientLayer(GetMaskLayer())->RenderLayer();
    }
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ColorLayerAttributes(GetColor(), GetBounds());
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
  {
    ClientLayer::Disconnect();
  }

protected:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }
};

already_AddRefed<ColorLayer>
ClientLayerManager::CreateColorLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ClientColorLayer> layer =
    new ClientColorLayer(this);
  CREATE_SHADOW(Color);
  return layer.forget();
}

}
}

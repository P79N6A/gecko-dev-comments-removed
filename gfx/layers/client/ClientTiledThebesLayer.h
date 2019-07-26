



#ifndef GFX_CLIENTTILEDTHEBESLAYER_H
#define GFX_CLIENTTILEDTHEBESLAYER_H

#include "ClientLayerManager.h"         
#include "Layers.h"                     
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/TiledContentClient.h"
#include "nsDebug.h"                    
#include "nsRegion.h"                   

class gfxContext;

namespace mozilla {
namespace layers {

class ShadowableLayer;
class SpecificLayerAttributes;
















class ClientTiledThebesLayer : public ThebesLayer,
                               public ClientLayer
{
  typedef ThebesLayer Base;

public:
  ClientTiledThebesLayer(ClientLayerManager* const aManager);
  ~ClientTiledThebesLayer();

  
  virtual Layer* AsLayer() { return this; }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) {
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mValidRegion.Sub(mValidRegion, aRegion);
    mLowPrecisionValidRegion.Sub(mLowPrecisionValidRegion, aRegion);
  }

  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs);
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
  {
    ClientLayer::Disconnect();
  }

  virtual void RenderLayer();

private:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }

  



  void BeginPaint();

  




  void EndPaint(bool aFinish);

  RefPtr<TiledContentClient> mContentClient;
  nsIntRegion mLowPrecisionValidRegion;
  BasicTiledLayerPaintData mPaintData;
};

} 
} 

#endif

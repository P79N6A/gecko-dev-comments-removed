



#ifndef GFX_CLIENTTILEDTHEBESLAYER_H
#define GFX_CLIENTTILEDTHEBESLAYER_H

#include "mozilla/layers/ShadowLayers.h"
#include "ClientLayerManager.h"
#include "mozilla/layers/TiledContentClient.h" 

namespace mozilla {
namespace layers {

class BasicTiledLayerBuffer;
















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

  
  virtual void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              bool aDidSelfCopy,
              LayerManager::DrawThebesLayerCallback aCallback,
              void* aCallbackData)
  { NS_RUNTIMEABORT("Not reached."); }


  



  void BeginPaint();

  




  void EndPaint(bool aFinish);

  RefPtr<TiledContentClient> mContentClient;
  nsIntRegion mLowPrecisionValidRegion;
  BasicTiledLayerPaintData mPaintData;
};

} 
} 

#endif

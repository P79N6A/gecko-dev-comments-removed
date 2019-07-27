



#ifndef GFX_CLIENTTILEDPAINTEDLAYER_H
#define GFX_CLIENTTILEDPAINTEDLAYER_H

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
















class ClientTiledPaintedLayer : public PaintedLayer,
                               public ClientLayer
{
  typedef PaintedLayer Base;

public:
  explicit ClientTiledPaintedLayer(ClientLayerManager* const aManager,
                                  ClientLayerManager::PaintedLayerCreationHint aCreationHint = LayerManager::NONE);

protected:
  ~ClientTiledPaintedLayer();

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) MOZ_OVERRIDE;

public:
  
  virtual const char* Name() const MOZ_OVERRIDE { return "TiledPaintedLayer"; }

  
  virtual Layer* AsLayer() MOZ_OVERRIDE { return this; }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE {
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mInvalidRegion.SimplifyOutward(20);
    mValidRegion.Sub(mValidRegion, mInvalidRegion);
    mLowPrecisionValidRegion.Sub(mLowPrecisionValidRegion, mInvalidRegion);
  }

  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) MOZ_OVERRIDE;
  virtual ShadowableLayer* AsShadowableLayer() MOZ_OVERRIDE { return this; }

  virtual void Disconnect() MOZ_OVERRIDE
  {
    ClientLayer::Disconnect();
  }

  virtual void RenderLayer() MOZ_OVERRIDE;

  virtual void ClearCachedResources() MOZ_OVERRIDE;

  




  void GetAncestorLayers(LayerMetricsWrapper* aOutScrollAncestor,
                         LayerMetricsWrapper* aOutDisplayPortAncestor);

private:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }

  



  void BeginPaint();

  



  bool UseFastPath();

  


  bool IsScrollingOnCompositor(const FrameMetrics& aParentMetrics);

  




  bool UseProgressiveDraw();

  



  bool RenderHighPrecision(nsIntRegion& aInvalidRegion,
                           const nsIntRegion& aVisibleRegion,
                           LayerManager::DrawPaintedLayerCallback aCallback,
                           void* aCallbackData);

  



  bool RenderLowPrecision(nsIntRegion& aInvalidRegion,
                          const nsIntRegion& aVisibleRegion,
                          LayerManager::DrawPaintedLayerCallback aCallback,
                          void* aCallbackData);

  



  void EndPaint();

  RefPtr<TiledContentClient> mContentClient;
  nsIntRegion mLowPrecisionValidRegion;
  BasicTiledLayerPaintData mPaintData;
};

} 
} 

#endif

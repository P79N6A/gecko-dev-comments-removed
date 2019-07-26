




#ifndef GFX_BASICTHEBESLAYER_H
#define GFX_BASICTHEBESLAYER_H

#include "mozilla/layers/PLayersParent.h"
#include "BasicLayersImpl.h"
#include "mozilla/layers/ContentClient.h"

namespace mozilla {
namespace layers {

class BasicThebesLayer : public ThebesLayer, public BasicImplData {
public:
  typedef ThebesLayerBuffer::PaintState PaintState;
  typedef ThebesLayerBuffer::ContentType ContentType;

  BasicThebesLayer(BasicLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager, static_cast<BasicImplData*>(this)),
    mContentClient(nullptr)
  {
    MOZ_COUNT_CTOR(BasicThebesLayer);
  }
  virtual ~BasicThebesLayer()
  {
    MOZ_COUNT_DTOR(BasicThebesLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ThebesLayer::SetVisibleRegion(aRegion);
  }
  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mInvalidRegion.SimplifyOutward(10);
    mValidRegion.Sub(mValidRegion, mInvalidRegion);
  }

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback);

  virtual void ClearCachedResources()
  {
    if (mContentClient) {
      mContentClient->Clear();
    }
    mValidRegion.SetEmpty();
  }
  
  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    if (!BasicManager()->IsRetained()) {
      
      
      mEffectiveTransform = GetLocalTransform()*aTransformToSurface;
      if (gfxPoint(0,0) != mResidualTranslation) {
        mResidualTranslation = gfxPoint(0,0);
        mValidRegion.SetEmpty();
      }
      ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
      return;
    }
    ThebesLayer::ComputeEffectiveTransforms(aTransformToSurface);
  }

  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

protected:
  virtual void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              bool aDidSelfCopy,
              LayerManager::DrawThebesLayerCallback aCallback,
              void* aCallbackData)
  {
    if (!aCallback) {
      BasicManager()->SetTransactionIncomplete();
      return;
    }
    aCallback(this, aContext, aExtendedRegionToDraw, aRegionToInvalidate,
              aCallbackData);
    
    
    
    
    nsIntRegion tmp;
    tmp.Or(mVisibleRegion, aExtendedRegionToDraw);
    mValidRegion.Or(mValidRegion, tmp);
  }

  RefPtr<ContentClient> mContentClient;
};

class BasicShadowableThebesLayer : public BasicThebesLayer,
                                   public BasicShadowableLayer
{
  typedef BasicThebesLayer Base;
public:
  BasicShadowableThebesLayer(BasicShadowLayerManager* aManager)
    : BasicThebesLayer(aManager)
  {
    MOZ_COUNT_CTOR(BasicShadowableThebesLayer);
  }
  virtual ~BasicShadowableThebesLayer();

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback);

  virtual void ClearCachedResources() MOZ_OVERRIDE
  {
    BasicThebesLayer::ClearCachedResources();
    DestroyBackBuffer();
    
    
    
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ThebesLayerAttributes(GetValidRegion());
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual CompositableClient* GetCompositableClient() MOZ_OVERRIDE
  {
    return mContentClient;
  }

  virtual void Disconnect();

  virtual BasicShadowableThebesLayer* AsThebes() { return this; }

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  virtual void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              bool aDidSelfCopy,
              LayerManager::DrawThebesLayerCallback aCallback,
              void* aCallbackData) MOZ_OVERRIDE;

  void DestroyBackBuffer()
  {
    mContentClient = nullptr;
  }
};

}
}

#endif

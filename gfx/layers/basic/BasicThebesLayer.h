




#ifndef GFX_BASICTHEBESLAYER_H
#define GFX_BASICTHEBESLAYER_H

#include "mozilla/layers/PLayersParent.h"
#include "BasicBuffers.h"

namespace mozilla {
namespace layers {

class BasicThebesLayer : public ThebesLayer, public BasicImplData {
public:
  typedef BasicThebesLayerBuffer Buffer;

  BasicThebesLayer(BasicLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager, static_cast<BasicImplData*>(this)),
    mBuffer(this)
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

  virtual void ClearCachedResources() { mBuffer.Clear(); mValidRegion.SetEmpty(); }
  
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(Buffer::ContentType aType, const nsIntSize& aSize);

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

  
  virtual void SyncFrontBufferToBackBuffer() {}

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

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

  Buffer mBuffer;
};

struct AutoBufferTracker;

class BasicShadowableThebesLayer : public BasicThebesLayer,
                                   public BasicShadowableLayer
{
  friend struct AutoBufferTracker;

  typedef BasicThebesLayer Base;

public:
  BasicShadowableThebesLayer(BasicShadowLayerManager* aManager)
    : BasicThebesLayer(aManager)
    , mBufferTracker(nullptr)
    , mIsNewBuffer(false)
    , mFrontAndBackBufferDiffer(false)
  {
    MOZ_COUNT_CTOR(BasicShadowableThebesLayer);
  }
  virtual ~BasicShadowableThebesLayer();

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback);

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ThebesLayerAttributes(GetValidRegion());
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  void SetBackBufferAndAttrs(const OptionalThebesBuffer& aBuffer,
                             const nsIntRegion& aValidRegion,
                             const OptionalThebesBuffer& aReadOnlyFrontBuffer,
                             const nsIntRegion& aFrontUpdatedRegion);

  virtual void Disconnect();

  virtual BasicShadowableThebesLayer* AsThebes() { return this; }

  virtual void SyncFrontBufferToBackBuffer();

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

  virtual already_AddRefed<gfxASurface>
  CreateBuffer(Buffer::ContentType aType, const nsIntSize& aSize) MOZ_OVERRIDE;

  void DestroyBackBuffer()
  {
    if (IsSurfaceDescriptorValid(mBackBuffer)) {
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);
    }
  }

  
  
  
  SurfaceDescriptor mBackBuffer;
  nsIntRect mBackBufferRect;
  nsIntPoint mBackBufferRectRotation;

  
  
  
  
  AutoBufferTracker* mBufferTracker;

  bool mIsNewBuffer;
  OptionalThebesBuffer mROFrontBuffer;
  nsIntRegion mFrontUpdatedRegion;
  bool mFrontAndBackBufferDiffer;
};

}
}

#endif

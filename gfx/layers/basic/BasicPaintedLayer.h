




#ifndef GFX_BASICPAINTEDLAYER_H
#define GFX_BASICPAINTEDLAYER_H

#include "Layers.h"                     
#include "RotatedBuffer.h"              
#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "gfxPoint.h"                   
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/layers/ContentClient.h"  
#include "mozilla/mozalloc.h"           
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
class gfxContext;

namespace mozilla {
namespace layers {

class ReadbackProcessor;

class BasicPaintedLayer : public PaintedLayer, public BasicImplData {
public:
  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  explicit BasicPaintedLayer(BasicLayerManager* aLayerManager) :
    PaintedLayer(aLayerManager, static_cast<BasicImplData*>(this)),
    mContentClient(nullptr)
  {
    MOZ_COUNT_CTOR(BasicPaintedLayer);
  }

protected:
  virtual ~BasicPaintedLayer()
  {
    MOZ_COUNT_DTOR(BasicPaintedLayer);
  }

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion) override
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    PaintedLayer::SetVisibleRegion(aRegion);
  }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) override
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mInvalidRegion.SimplifyOutward(20);
    mValidRegion.Sub(mValidRegion, mInvalidRegion);
  }

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
                           LayerManager::DrawPaintedLayerCallback aCallback,
                           void* aCallbackData) override;

  virtual void Validate(LayerManager::DrawPaintedLayerCallback aCallback,
                        void* aCallbackData,
                        ReadbackProcessor* aReadback) override;

  virtual void ClearCachedResources() override
  {
    if (mContentClient) {
      mContentClient->Clear();
    }
    mValidRegion.SetEmpty();
  }

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override
  {
    if (!BasicManager()->IsRetained()) {
      
      
      mEffectiveTransform = GetLocalTransform() * aTransformToSurface;
      if (gfxPoint(0,0) != mResidualTranslation) {
        mResidualTranslation = gfxPoint(0,0);
        mValidRegion.SetEmpty();
      }
      ComputeEffectiveTransformForMaskLayers(aTransformToSurface);
      return;
    }
    PaintedLayer::ComputeEffectiveTransforms(aTransformToSurface);
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
              DrawRegionClip aClip,
              LayerManager::DrawPaintedLayerCallback aCallback,
              void* aCallbackData)
  {
    if (!aCallback) {
      BasicManager()->SetTransactionIncomplete();
      return;
    }
    aCallback(this, aContext, aExtendedRegionToDraw, aClip,
              aRegionToInvalidate, aCallbackData);
    
    
    
    
    nsIntRegion tmp;
    tmp.Or(mVisibleRegion, aExtendedRegionToDraw);
    mValidRegion.Or(mValidRegion, tmp);
  }

  RefPtr<ContentClientBasic> mContentClient;
};

}
}

#endif

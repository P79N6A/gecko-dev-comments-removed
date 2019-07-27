




#ifndef GFX_BASICTHEBESLAYER_H
#define GFX_BASICTHEBESLAYER_H

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

class BasicThebesLayer : public ThebesLayer, public BasicImplData {
public:
  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  explicit BasicThebesLayer(BasicLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager,
                static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST())),
    mContentClient(nullptr)
  {
    MOZ_COUNT_CTOR(BasicThebesLayer);
  }

protected:
  virtual ~BasicThebesLayer()
  {
    MOZ_COUNT_DTOR(BasicThebesLayer);
  }

public:
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
    mInvalidRegion.SimplifyOutward(20);
    mValidRegion.Sub(mValidRegion, mInvalidRegion);
  }

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData);

  virtual void Validate(LayerManager::DrawThebesLayerCallback aCallback,
                        void* aCallbackData,
                        ReadbackProcessor* aReadback) MOZ_OVERRIDE;

  virtual void ClearCachedResources()
  {
    if (mContentClient) {
      mContentClient->Clear();
    }
    mValidRegion.SetEmpty();
  }

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface)
  {
    if (!BasicManager()->IsRetained()) {
      
      
      mEffectiveTransform = GetLocalTransform() * aTransformToSurface;
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
              DrawRegionClip aClip,
              LayerManager::DrawThebesLayerCallback aCallback,
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






#ifndef GFX_CLIENTPAINTEDLAYER_H
#define GFX_CLIENTPAINTEDLAYER_H

#include "ClientLayerManager.h"         
#include "Layers.h"                     
#include "RotatedBuffer.h"              
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/ContentClient.h"  
#include "mozilla/mozalloc.h"           
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
#include "mozilla/layers/PLayerTransaction.h" 

class gfxContext;

namespace mozilla {
namespace layers {

class CompositableClient;
class ShadowableLayer;
class SpecificLayerAttributes;

class ClientPaintedLayer : public PaintedLayer,
                          public ClientLayer {
public:
  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  explicit ClientPaintedLayer(ClientLayerManager* aLayerManager,
                             LayerManager::PaintedLayerCreationHint aCreationHint = LayerManager::NONE) :
    PaintedLayer(aLayerManager, static_cast<ClientLayer*>(this), aCreationHint),
    mContentClient(nullptr)
  {
    MOZ_COUNT_CTOR(ClientPaintedLayer);
  }

protected:
  virtual ~ClientPaintedLayer()
  {
    if (mContentClient) {
      mContentClient->OnDetach();
      mContentClient = nullptr;
    }
    MOZ_COUNT_DTOR(ClientPaintedLayer);
  }

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    PaintedLayer::SetVisibleRegion(aRegion);
  }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mInvalidRegion.SimplifyOutward(20);
    mValidRegion.Sub(mValidRegion, mInvalidRegion);
  }

  virtual void RenderLayer() MOZ_OVERRIDE { RenderLayerWithReadback(nullptr); }

  virtual void RenderLayerWithReadback(ReadbackProcessor *aReadback) MOZ_OVERRIDE;

  virtual void ClearCachedResources() MOZ_OVERRIDE
  {
    if (mContentClient) {
      mContentClient->Clear();
    }
    mValidRegion.SetEmpty();
    DestroyBackBuffer();
  }
  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) MOZ_OVERRIDE
  {
    aAttrs = PaintedLayerAttributes(GetValidRegion());
  }
  
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }
  
  virtual Layer* AsLayer() MOZ_OVERRIDE { return this; }
  virtual ShadowableLayer* AsShadowableLayer() MOZ_OVERRIDE { return this; }

  virtual CompositableClient* GetCompositableClient() MOZ_OVERRIDE
  {
    return mContentClient;
  }

  virtual void Disconnect() MOZ_OVERRIDE
  {
    mContentClient = nullptr;
    ClientLayer::Disconnect();
  }

protected:
  void PaintThebes();

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) MOZ_OVERRIDE;

  void DestroyBackBuffer()
  {
    mContentClient = nullptr;
  }

  RefPtr<ContentClient> mContentClient;
};

}
}

#endif

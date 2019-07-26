




#ifndef GFX_CLIENTTHEBESLAYER_H
#define GFX_CLIENTTHEBESLAYER_H

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

class ClientThebesLayer : public ThebesLayer, 
                          public ClientLayer {
public:
  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  ClientThebesLayer(ClientLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager,
                static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST())),
    mContentClient(nullptr)
  {
    MOZ_COUNT_CTOR(ClientThebesLayer);
  }
  virtual ~ClientThebesLayer()
  {
    if (mContentClient) {
      mContentClient->OnDetach();
      mContentClient = nullptr;
    }
    MOZ_COUNT_DTOR(ClientThebesLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ThebesLayer::SetVisibleRegion(aRegion);
  }
  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mValidRegion.Sub(mValidRegion, mInvalidRegion);
  }

  virtual void RenderLayer();

  virtual void ClearCachedResources()
  {
    if (mContentClient) {
      mContentClient->Clear();
    }
    mValidRegion.SetEmpty();
    DestroyBackBuffer();
  }
  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ThebesLayerAttributes(GetValidRegion());
  }
  
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }
  
  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual CompositableClient* GetCompositableClient() MOZ_OVERRIDE
  {
    return mContentClient;
  }

  virtual void Disconnect()
  {
    mContentClient = nullptr;
    ClientLayer::Disconnect();
  }

protected:
  void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              bool aDidSelfCopy,
              DrawRegionClip aClip);

  void PaintThebes();
  
  void DestroyBackBuffer()
  {
    mContentClient = nullptr;
  }

  RefPtr<ContentClient> mContentClient;
};

}
}

#endif

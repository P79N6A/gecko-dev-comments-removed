




#ifndef GFX_CLIENTCANVASLAYER_H
#define GFX_CLIENTCANVASLAYER_H

#include "mozilla/layers/CanvasClient.h"  
#include "ClientLayerManager.h"         
#include "CopyableCanvasLayer.h"        
#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   

namespace mozilla {
namespace gl {
class SharedSurface;
class SurfaceFactory;
}

namespace layers {

class CompositableClient;
class ShadowableLayer;

class ClientCanvasLayer : public CopyableCanvasLayer,
                          public ClientLayer
{
  typedef CanvasClient::CanvasClientType CanvasClientType;
public:
  explicit ClientCanvasLayer(ClientLayerManager* aLayerManager) :
    CopyableCanvasLayer(aLayerManager, static_cast<ClientLayer*>(this))
  {
    MOZ_COUNT_CTOR(ClientCanvasLayer);
  }

protected:
  virtual ~ClientCanvasLayer();

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    CanvasLayer::SetVisibleRegion(aRegion);
  }

  virtual void Initialize(const Data& aData) MOZ_OVERRIDE;

  virtual void RenderLayer() MOZ_OVERRIDE;

  virtual void ClearCachedResources() MOZ_OVERRIDE
  {
    if (mCanvasClient) {
      mCanvasClient->Clear();
    }
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) MOZ_OVERRIDE
  {
    aAttrs = CanvasLayerAttributes(mFilter, mBounds);
  }

  virtual Layer* AsLayer()  MOZ_OVERRIDE { return this; }
  virtual ShadowableLayer* AsShadowableLayer()  MOZ_OVERRIDE { return this; }

  virtual void Disconnect() MOZ_OVERRIDE
  {
    mCanvasClient = nullptr;
    ClientLayer::Disconnect();
  }

  virtual CompositableClient* GetCompositableClient() MOZ_OVERRIDE
  {
    return mCanvasClient;
  }
protected:
  ClientLayerManager* ClientManager()
  {
    return static_cast<ClientLayerManager*>(mManager);
  }

  CanvasClientType GetCanvasClientType();

  RefPtr<CanvasClient> mCanvasClient;

  UniquePtr<gl::SurfaceFactory> mFactory;

  friend class DeprecatedCanvasClient2D;
  friend class CanvasClient2D;
  friend class CanvasClientSharedSurface;
};
}
}

#endif

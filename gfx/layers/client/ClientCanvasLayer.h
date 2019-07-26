




#ifndef GFX_CLIENTCANVASLAYER_H
#define GFX_CLIENTCANVASLAYER_H

#include "mozilla/layers/CanvasClient.h"  
#include "ClientLayerManager.h"         
#include "CopyableCanvasLayer.h"        
#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayerTransaction.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class CompositableClient;
class ShadowableLayer;

class ClientCanvasLayer : public CopyableCanvasLayer,
                          public ClientLayer
{
  typedef CanvasClient::CanvasClientType CanvasClientType;
public:
  ClientCanvasLayer(ClientLayerManager* aLayerManager) :
    CopyableCanvasLayer(aLayerManager, static_cast<ClientLayer*>(this))
  {
    MOZ_COUNT_CTOR(ClientCanvasLayer);
  }
  virtual ~ClientCanvasLayer()
  {
    MOZ_COUNT_DTOR(ClientCanvasLayer);
    if (mCanvasClient) {
      mCanvasClient->Detach();
      mCanvasClient = nullptr;
    }
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(ClientManager()->InConstruction(),
                 "Can only set properties in construction phase");
    CanvasLayer::SetVisibleRegion(aRegion);
  }
  
  virtual void Initialize(const Data& aData);

  virtual void RenderLayer();
  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = CanvasLayerAttributes(mFilter, mBounds);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }
  
  virtual void Disconnect()
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
  
  CanvasClientType GetCanvasClientType()
  {
    if (mGLContext) {
      return CanvasClient::CanvasClientGLContext;
    }
    return CanvasClient::CanvasClientSurface;
  }

  RefPtr<CanvasClient> mCanvasClient;

  friend class DeprecatedCanvasClient2D;
  friend class CanvasClient2D;
  friend class DeprecatedCanvasClientSurfaceStream;
};
}
}

#endif

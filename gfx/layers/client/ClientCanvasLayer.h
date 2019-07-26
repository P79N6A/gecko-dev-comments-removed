




#ifndef GFX_CLIENTCANVASLAYER_H
#define GFX_CLIENTCANVASLAYER_H

#include "ClientLayerManager.h"
#include "nsXULAppAPI.h"
#include "gfxASurface.h"
#include "mozilla/Preferences.h"
#include "mozilla/layers/LayerTransaction.h"
#include "mozilla/layers/CanvasClient.h"
#include "CopyableCanvasLayer.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class CanvasClient2D;
class CanvasClientWebGL;

class ClientCanvasLayer : public CopyableCanvasLayer,
                          public ClientLayer
{
public:
  ClientCanvasLayer(ClientLayerManager* aLayerManager) :
    CopyableCanvasLayer(aLayerManager, static_cast<ClientLayer*>(this))
  {
    MOZ_COUNT_CTOR(ClientCanvasLayer);
  }
  virtual ~ClientCanvasLayer()
  {
    MOZ_COUNT_DTOR(ClientCanvasLayer);
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
  
  CompositableType GetCompositableClientType()
  {
    if (mGLContext && XRE_GetProcessType() == GeckoProcessType_Default) {
      return BUFFER_IMAGE_BUFFERED;
    }
    return BUFFER_IMAGE_SINGLE;
  }

  RefPtr<CanvasClient> mCanvasClient;

  friend class CanvasClient2D;
  friend class CanvasClientWebGL;
};
}
}

#endif

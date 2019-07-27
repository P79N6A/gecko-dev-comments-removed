




#ifndef GFX_CLIENTREADBACKLAYER_H
#define GFX_CLIENTREADBACKLAYER_H

#include "ClientLayerManager.h"
#include "ReadbackLayer.h"

namespace mozilla {
namespace layers {

class ClientReadbackLayer :
  public ReadbackLayer,
  public ClientLayer
{
public:
  explicit ClientReadbackLayer(ClientLayerManager *aManager)
    : ReadbackLayer(aManager, nullptr)
  {
      mImplData = static_cast<ClientLayer*>(this);
  }

  virtual ShadowableLayer* AsShadowableLayer() override { return this; }
  virtual Layer* AsLayer() override { return this; }
  virtual void RenderLayer() override {}
};

} 
} 

#endif 

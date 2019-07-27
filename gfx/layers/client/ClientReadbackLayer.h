




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

  virtual Layer* AsLayer() { return this; }
  virtual void RenderLayer() {}
};

} 
} 

#endif 

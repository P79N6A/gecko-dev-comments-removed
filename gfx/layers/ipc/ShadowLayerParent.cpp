






#include "LayerTransactionParent.h"
#include "ShadowLayerParent.h"
#include "ShadowLayers.h"

#include "BasicLayers.h"

namespace mozilla {
namespace layers {

ShadowLayerParent::ShadowLayerParent() : mLayer(nullptr)
{
}

ShadowLayerParent::~ShadowLayerParent()
{
}

void
ShadowLayerParent::Bind(Layer* layer)
{
  mLayer = layer;
}

void
ShadowLayerParent::Destroy()
{
  
  
  
  
  if (mLayer) {
    mLayer->Disconnect();
  }
}

ContainerLayer*
ShadowLayerParent::AsContainer() const
{
  return static_cast<ContainerLayer*>(AsLayer());
}

void
ShadowLayerParent::ActorDestroy(ActorDestroyReason why)
{
  switch (why) {
  case AncestorDeletion:
    NS_RUNTIMEABORT("shadow layer deleted out of order!");
    return;                     

  case Deletion:
    
    if (mLayer) {
      mLayer->Disconnect();
    }
    break;

  case AbnormalShutdown:
    if (mLayer) {
      mLayer->Disconnect();
    }
    break;

  case NormalShutdown:
    
    
    break;

  case FailedConstructor:
    NS_RUNTIMEABORT("FailedConstructor isn't possible in PLayerTransaction");
    return;                     
  }

  mLayer = nullptr;
}

} 
} 

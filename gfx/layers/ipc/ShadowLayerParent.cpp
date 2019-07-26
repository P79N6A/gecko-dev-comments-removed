






#include "ShadowLayerParent.h"
#include "Layers.h"                     
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            

#include "mozilla/layers/ThebesLayerComposite.h"
#include "mozilla/layers/CanvasLayerComposite.h"
#include "mozilla/layers/ColorLayerComposite.h"
#include "mozilla/layers/ImageLayerComposite.h"
#include "mozilla/layers/ContainerLayerComposite.h"

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

ContainerLayerComposite*
ShadowLayerParent::AsContainerLayerComposite() const
{
  return mLayer && mLayer->GetType() == Layer::TYPE_CONTAINER
         ? static_cast<ContainerLayerComposite*>(mLayer.get())
         : nullptr;
}

CanvasLayerComposite*
ShadowLayerParent::AsCanvasLayerComposite() const
{
  return mLayer && mLayer->GetType() == Layer::TYPE_CANVAS
         ? static_cast<CanvasLayerComposite*>(mLayer.get())
         : nullptr;
}

ColorLayerComposite*
ShadowLayerParent::AsColorLayerComposite() const
{
  return mLayer && mLayer->GetType() == Layer::TYPE_COLOR
         ? static_cast<ColorLayerComposite*>(mLayer.get())
         : nullptr;
}

ImageLayerComposite*
ShadowLayerParent::AsImageLayerComposite() const
{
  return mLayer && mLayer->GetType() == Layer::TYPE_IMAGE
         ? static_cast<ImageLayerComposite*>(mLayer.get())
         : nullptr;
}

RefLayerComposite*
ShadowLayerParent::AsRefLayerComposite() const
{
  return mLayer && mLayer->GetType() == Layer::TYPE_REF
         ? static_cast<RefLayerComposite*>(mLayer.get())
         : nullptr;
}

ThebesLayerComposite*
ShadowLayerParent::AsThebesLayerComposite() const
{
  return mLayer && mLayer->GetType() == Layer::TYPE_THEBES
         ? static_cast<ThebesLayerComposite*>(mLayer.get())
         : nullptr;
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

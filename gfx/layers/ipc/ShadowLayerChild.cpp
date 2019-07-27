






#include "ShadowLayerChild.h"
#include "Layers.h"                     
#include "ShadowLayers.h"               

namespace mozilla {
namespace layers {

ShadowLayerChild::ShadowLayerChild(ShadowableLayer* aLayer)
  : mLayer(aLayer)
{ }

ShadowLayerChild::~ShadowLayerChild()
{ }

void
ShadowLayerChild::ActorDestroy(ActorDestroyReason why)
{
  MOZ_ASSERT(AncestorDeletion != why,
             "shadowable layer should have been cleaned up by now");

  if (AbnormalShutdown == why) {
    
    
    
    mLayer->AsLayer()->Disconnect();
    mLayer = nullptr;
  }
}

}  
}  

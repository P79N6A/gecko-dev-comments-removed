







































#ifndef mozilla_layers_ShadowLayersParent_h
#define mozilla_layers_ShadowLayersParent_h

#include "mozilla/layers/PLayersParent.h"

namespace mozilla {

namespace layout {
class RenderFrameParent;
}

namespace layers {

class Layer;
class ShadowLayerManager;

class ShadowLayersParent : public PLayersParent
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef InfallibleTArray<Edit> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;

public:
  ShadowLayersParent(ShadowLayerManager* aManager);
  ~ShadowLayersParent();

  ShadowLayerManager* layer_manager() const { return mLayerManager; }

  ContainerLayer* GetRoot() const { return mRoot; }

protected:
  NS_OVERRIDE virtual bool RecvUpdate(const EditArray& cset,
                                      EditReplyArray* reply);

  NS_OVERRIDE virtual PLayerParent* AllocPLayer();
  NS_OVERRIDE virtual bool DeallocPLayer(PLayerParent* actor);

private:
  RenderFrameParent* Frame();

  nsRefPtr<ShadowLayerManager> mLayerManager;
  
  
  nsRefPtr<ContainerLayer> mRoot;
};

} 
} 

#endif 

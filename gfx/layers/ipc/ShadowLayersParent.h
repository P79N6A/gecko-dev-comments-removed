







































#ifndef mozilla_layers_ShadowLayersParent_h
#define mozilla_layers_ShadowLayersParent_h

#include "mozilla/layers/PLayersParent.h"
#include "ShadowLayerParent.h"

namespace mozilla {
namespace layers {

class ShadowLayerManager;

class ShadowLayersParent : public PLayersParent
{
  typedef nsTArray<Edit> EditArray;
  typedef nsTArray<EditReply> EditReplyArray;

public:
  ShadowLayersParent(ShadowLayerManager* aManager);
  ~ShadowLayersParent();

  ShadowLayerManager* layer_manager() const { return mLayerManager; }

protected:
  NS_OVERRIDE virtual bool RecvUpdate(const EditArray& cset,
                                      EditReplyArray* reply);

  NS_OVERRIDE virtual PLayerParent* AllocPLayer() {
    return new ShadowLayerParent();
  }

  NS_OVERRIDE virtual bool DeallocPLayer(PLayerParent* actor) {
    delete actor;
    return true;
  }

private:
  nsRefPtr<ShadowLayerManager> mLayerManager;
};

} 
} 

#endif 

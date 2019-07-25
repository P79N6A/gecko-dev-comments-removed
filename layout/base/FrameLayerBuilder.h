




































#ifndef FRAMELAYERBUILDER_H_
#define FRAMELAYERBUILDER_H_

#include "Layers.h"

class nsDisplayListBuilder;
class nsDisplayList;
class nsDisplayItem;

namespace mozilla {

class FrameLayerBuilder {
public:
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::ThebesLayer ThebesLayer;
  typedef mozilla::layers::LayerManager LayerManager;

  




  already_AddRefed<Layer> MakeContainerLayerFor(nsDisplayListBuilder* aBuilder,
                                                LayerManager* aManager,
                                                nsDisplayItem* aContainer,
                                                const nsDisplayList& aChildren);

  



  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);
};

}

#endif 

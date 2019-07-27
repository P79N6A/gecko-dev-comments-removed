




#ifndef GFX_TEST_LAYERS_H
#define GFX_TEST_LAYERS_H

#include "Layers.h"
#include "nsTArray.h"
















already_AddRefed<mozilla::layers::Layer> CreateLayerTree(
    const char* aLayerTreeDescription,
    nsIntRegion* aVisibleRegions,
    const gfx3DMatrix* aTransforms,
    nsRefPtr<mozilla::layers::LayerManager>& aLayerManager,
    nsTArray<nsRefPtr<mozilla::layers::Layer> >& aLayersOut);


#endif


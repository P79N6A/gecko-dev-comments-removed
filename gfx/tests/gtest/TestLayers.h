




#ifndef GFX_TEST_LAYERS_H
#define GFX_TEST_LAYERS_H

#include "Layers.h"
#include "nsTArray.h"
















already_AddRefed<mozilla::layers::Layer> CreateLayerTree(
    const char* aLayerTreeDescription,
    nsIntRegion* aVisibleRegions,
    const mozilla::gfx::Matrix4x4* aTransforms,
    nsRefPtr<mozilla::layers::LayerManager>& aLayerManager,
    nsTArray<nsRefPtr<mozilla::layers::Layer> >& aLayersOut);


#endif


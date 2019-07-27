




#ifndef GFX_LAYER_TREE_INVALIDATION_H
#define GFX_LAYER_TREE_INVALIDATION_H

#include "nsRegion.h"                   
#include "mozilla/UniquePtr.h"          
#include "mozilla/gfx/Point.h"

class nsPresContext;

namespace mozilla {
namespace layers {

class Layer;
class ContainerLayer;








typedef void (*NotifySubDocInvalidationFunc)(ContainerLayer* aLayer,
                                             const nsIntRegion& aRegion);





struct LayerProperties
{
  virtual ~LayerProperties() {}

  






  static UniquePtr<LayerProperties> CloneFrom(Layer* aRoot);

  


  static void ClearInvalidations(Layer* aRoot);

  








  virtual nsIntRegion ComputeDifferences(Layer* aRoot,
                                         NotifySubDocInvalidationFunc aCallback,
                                         bool* aGeometryChanged = nullptr) = 0;

  virtual void MoveBy(const gfx::IntPoint& aOffset) = 0;
};

} 
} 

#endif 

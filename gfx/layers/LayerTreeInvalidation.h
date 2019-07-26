




#ifndef GFX_LAYER_TREE_INVALIDATION_H
#define GFX_LAYER_TREE_INVALIDATION_H

#include "nsRegion.h"

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

  






  static LayerProperties* CloneFrom(Layer* aRoot);

  


  static void ClearInvalidations(Layer* aRoot);

  








  virtual nsIntRegion ComputeDifferences(Layer* aRoot, 
                                         NotifySubDocInvalidationFunc aCallback) = 0;
  
  
  virtual void MoveBy(const nsIntPoint& aOffset) = 0;
};

} 
} 

#endif 

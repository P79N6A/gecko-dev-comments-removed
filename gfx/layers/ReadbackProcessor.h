




#ifndef GFX_READBACKPROCESSOR_H
#define GFX_READBACKPROCESSOR_H

#include <stdint.h>                     
#include "nsRect.h"                     
#include "nsTArray.h"                   
 
class nsIntRegion;

namespace mozilla {
namespace layers {

class ContainerLayer;
class ReadbackLayer;
class ThebesLayer;

class ReadbackProcessor {
public:
  











  void BuildUpdates(ContainerLayer* aContainer);

  struct Update {
    


    ReadbackLayer* mLayer;
    





    nsIntRect      mUpdateRect;
    


    uint64_t       mSequenceCounter;
  };
  










  void GetThebesLayerUpdates(ThebesLayer* aLayer,
                             nsTArray<Update>* aUpdates,
                             nsIntRegion* aUpdateRegion = nullptr);

  ~ReadbackProcessor();

protected:
  void BuildUpdatesForLayer(ReadbackLayer* aLayer);

  nsTArray<Update> mAllUpdates;
};

}
}
#endif 

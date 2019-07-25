




#ifndef GFX_READBACKPROCESSOR_H
#define GFX_READBACKPROCESSOR_H

#include "ThebesLayerBuffer.h"
#include "nsTArray.h"

namespace mozilla {
namespace layers {

class ContainerLayer;
class ReadbackLayer;

class ReadbackProcessor {
public:
  











  void BuildUpdates(ContainerLayer* aContainer);

  struct Update {
    


    ReadbackLayer* mLayer;
    





    nsIntRect      mUpdateRect;
    


    PRUint64       mSequenceCounter;
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

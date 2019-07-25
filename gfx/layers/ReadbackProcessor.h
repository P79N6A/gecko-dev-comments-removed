




































#ifndef GFX_READBACKPROCESSOR_H
#define GFX_READBACKPROCESSOR_H

#include "ReadbackLayer.h"
#include "ThebesLayerBuffer.h"

namespace mozilla {
namespace layers {

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
                             nsIntRegion* aUpdateRegion = nsnull);

  ~ReadbackProcessor();

protected:
  void BuildUpdatesForLayer(ReadbackLayer* aLayer);

  nsTArray<Update> mAllUpdates;
};

}
}
#endif 

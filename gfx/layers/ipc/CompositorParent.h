






































#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h

#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayersParent.h"

namespace mozilla {
namespace layers {

class CompositorParent : public PCompositorParent
{

public:
  CompositorParent() {}
  virtual ~CompositorParent() {}

  bool AnswerInit() {
    printf("Answer init\n");
    return true;
  }

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend &backend) {
    printf("Alloc PLayers :)\n");
    
    
       return nsnull;
    

    
  }

  virtual bool DeallocPLayers(PLayersParent* aLayers) {
    delete aLayers;
    return true;
   }
};

} 
} 

#endif 

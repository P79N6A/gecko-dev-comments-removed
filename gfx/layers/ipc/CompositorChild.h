





#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "mozilla/layers/PCompositorChild.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace layers {

class LayerManager;
class CompositorParent;

class CompositorChild : public PCompositorChild
{
  NS_INLINE_DECL_REFCOUNTING(CompositorChild)
public:
  CompositorChild(LayerManager *aLayerManager);
  virtual ~CompositorChild();

  void Destroy();

  




  static PCompositorChild*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  static PCompositorChild* Get();

  static bool ChildProcessHasCompositor() { return sCompositor != nullptr; }
protected:
  virtual PLayersChild* AllocPLayers(const LayersBackend& aBackendHint,
                                     const uint64_t& aId,
                                     LayersBackend* aBackend,
                                     int* aMaxTextureSize);
  virtual bool DeallocPLayers(PLayersChild *aChild);

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  nsRefPtr<LayerManager> mLayerManager;

  
  
  
  static CompositorChild* sCompositor;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 

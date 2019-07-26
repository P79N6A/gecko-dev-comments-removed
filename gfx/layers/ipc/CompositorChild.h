





#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "mozilla/layers/PCompositorChild.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace layers {

class LayerManager;
class CompositorParent;
struct TextureFactoryIdentifier;

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
  virtual PLayerTransactionChild*
    AllocPLayerTransaction(const LayersBackend& aBackendHint,
                           const uint64_t& aId,
                           TextureFactoryIdentifier* aTextureFactoryIdentifier) MOZ_OVERRIDE;

  virtual bool DeallocPLayerTransaction(PLayerTransactionChild *aChild) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  nsRefPtr<LayerManager> mLayerManager;
  nsCOMPtr<nsIObserver> mMemoryPressureObserver;

  
  
  
  static CompositorChild* sCompositor;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 

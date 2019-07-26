





#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "base/basictypes.h"            
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/PCompositorChild.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            

class nsIObserver;

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

  




  static bool
  Create(Transport* aTransport, ProcessId aOtherProcess);

  static PCompositorChild* Get();

  static bool ChildProcessHasCompositor() { return sCompositor != nullptr; }
protected:
  virtual PLayerTransactionChild*
    AllocPLayerTransactionChild(const nsTArray<LayersBackend>& aBackendHints,
                                const uint64_t& aId,
                                TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                bool* aSuccess) MOZ_OVERRIDE;

  virtual bool DeallocPLayerTransactionChild(PLayerTransactionChild *aChild) MOZ_OVERRIDE;

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

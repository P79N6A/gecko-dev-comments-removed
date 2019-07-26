





#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "base/basictypes.h"            
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/PCompositorChild.h"
#include "nsAutoPtr.h"                  
#include "nsClassHashtable.h"           
#include "nsCOMPtr.h"                   
#include "nsHashKeys.h"                 
#include "nsISupportsImpl.h"            

class nsIObserver;

namespace mozilla {
namespace layers {

class ClientLayerManager;
class CompositorParent;
class FrameMetrics;

class CompositorChild MOZ_FINAL : public PCompositorChild
{
  NS_INLINE_DECL_REFCOUNTING(CompositorChild)
public:
  CompositorChild(ClientLayerManager *aLayerManager);

  void Destroy();

  




  bool LookupCompositorFrameMetrics(const FrameMetrics::ViewID aId, FrameMetrics&);

  




  static PCompositorChild*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  static CompositorChild* Get();

  static bool ChildProcessHasCompositor() { return sCompositor != nullptr; }

  virtual bool RecvInvalidateAll() MOZ_OVERRIDE;

private:
  
  virtual ~CompositorChild();

  virtual PLayerTransactionChild*
    AllocPLayerTransactionChild(const nsTArray<LayersBackend>& aBackendHints,
                                const uint64_t& aId,
                                TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                bool* aSuccess) MOZ_OVERRIDE;

  virtual bool DeallocPLayerTransactionChild(PLayerTransactionChild *aChild) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool RecvSharedCompositorFrameMetrics(const mozilla::ipc::SharedMemoryBasic::Handle& metrics,
                                                const CrossProcessMutexHandle& handle,
                                                const uint32_t& aAPZCId) MOZ_OVERRIDE;

  virtual bool RecvReleaseSharedCompositorFrameMetrics(const ViewID& aId,
                                                       const uint32_t& aAPZCId) MOZ_OVERRIDE;

  
  class SharedFrameMetricsData {
  public:
    SharedFrameMetricsData(
        const mozilla::ipc::SharedMemoryBasic::Handle& metrics,
        const CrossProcessMutexHandle& handle,
        const uint32_t& aAPZCId);

    ~SharedFrameMetricsData();

    void CopyFrameMetrics(FrameMetrics* aFrame);
    FrameMetrics::ViewID GetViewID();
    uint32_t GetAPZCId();

  private:
    
    
    mozilla::ipc::SharedMemoryBasic* mBuffer;
    CrossProcessMutex* mMutex;
    
    uint32_t mAPZCId;
  };

  nsRefPtr<ClientLayerManager> mLayerManager;

  
  
  nsClassHashtable<nsUint64HashKey, SharedFrameMetricsData> mFrameMetricsTable;

  
  
  
  static CompositorChild* sCompositor;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 

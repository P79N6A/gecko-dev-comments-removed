





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
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"
#include "nsWeakReference.h"

namespace mozilla {

namespace dom {
  class TabChild;
}

namespace layers {

using mozilla::dom::TabChild;

class ClientLayerManager;
class CompositorParent;
struct FrameMetrics;

class CompositorChild final : public PCompositorChild
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(CompositorChild)

public:
  explicit CompositorChild(ClientLayerManager *aLayerManager);

  void Destroy();

  




  bool LookupCompositorFrameMetrics(const FrameMetrics::ViewID aId, FrameMetrics&);

  




  static PCompositorChild*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  



  bool OpenSameProcess(CompositorParent* aParent);

  static CompositorChild* Get();

  static bool ChildProcessHasCompositor() { return sCompositor != nullptr; }

  void AddOverfillObserver(ClientLayerManager* aLayerManager);

  virtual bool
  RecvDidComposite(const uint64_t& aId, const uint64_t& aTransactionId) override;

  virtual bool
  RecvInvalidateAll() override;

  virtual bool
  RecvOverfill(const uint32_t &aOverfill) override;

  virtual bool
  RecvUpdatePluginConfigurations(const nsIntPoint& aContentOffset,
                                 const nsIntRegion& aVisibleRegion,
                                 nsTArray<PluginWindowData>&& aPlugins) override;

  virtual bool
  RecvUpdatePluginVisibility(nsTArray<uintptr_t>&& aWindowList) override;

  





  void RequestNotifyAfterRemotePaint(TabChild* aTabChild);

  void CancelNotifyAfterRemotePaint(TabChild* aTabChild);

  
  
  
  
  
  
  bool SendWillStop();
  bool SendPause();
  bool SendResume();
  bool SendNotifyChildCreated(const uint64_t& id);
  bool SendAdoptChild(const uint64_t& id);
  bool SendMakeSnapshot(const SurfaceDescriptor& inSnapshot, const nsIntRect& dirtyRect);
  bool SendFlushRendering();
  bool SendGetTileSize(int32_t* tileWidth, int32_t* tileHeight);
  bool SendStartFrameTimeRecording(const int32_t& bufferSize, uint32_t* startIndex);
  bool SendStopFrameTimeRecording(const uint32_t& startIndex, nsTArray<float>* intervals);
  bool SendNotifyRegionInvalidated(const nsIntRegion& region);
  bool SendRequestNotifyAfterRemotePaint();

private:
  
  virtual ~CompositorChild();

  virtual PLayerTransactionChild*
    AllocPLayerTransactionChild(const nsTArray<LayersBackend>& aBackendHints,
                                const uint64_t& aId,
                                TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                bool* aSuccess) override;

  virtual bool DeallocPLayerTransactionChild(PLayerTransactionChild *aChild) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvSharedCompositorFrameMetrics(const mozilla::ipc::SharedMemoryBasic::Handle& metrics,
                                                const CrossProcessMutexHandle& handle,
                                                const uint64_t& aLayersId,
                                                const uint32_t& aAPZCId) override;

  virtual bool RecvReleaseSharedCompositorFrameMetrics(const ViewID& aId,
                                                       const uint32_t& aAPZCId) override;

  virtual bool
  RecvRemotePaintIsReady() override;

  
  class SharedFrameMetricsData {
  public:
    SharedFrameMetricsData(
        const mozilla::ipc::SharedMemoryBasic::Handle& metrics,
        const CrossProcessMutexHandle& handle,
        const uint64_t& aLayersId,
        const uint32_t& aAPZCId);

    ~SharedFrameMetricsData();

    void CopyFrameMetrics(FrameMetrics* aFrame);
    FrameMetrics::ViewID GetViewID();
    uint64_t GetLayersId() const;
    uint32_t GetAPZCId();

  private:
    
    
    nsRefPtr<mozilla::ipc::SharedMemoryBasic> mBuffer;
    CrossProcessMutex* mMutex;
    uint64_t mLayersId;
    
    uint32_t mAPZCId;
  };

  static PLDHashOperator RemoveSharedMetricsForLayersId(const uint64_t& aKey,
                                                        nsAutoPtr<SharedFrameMetricsData>& aData,
                                                        void* aLayerTransactionChild);

  nsRefPtr<ClientLayerManager> mLayerManager;
  
  
  nsRefPtr<CompositorParent> mCompositorParent;

  
  
  nsClassHashtable<nsUint64HashKey, SharedFrameMetricsData> mFrameMetricsTable;

  
  
  
  static CompositorChild* sCompositor;

  
  
  nsWeakPtr mWeakTabChild;      

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);

  
  nsAutoTArray<ClientLayerManager*,0> mOverfillObservers;

  
  bool mCanSend;
};

} 
} 

#endif 

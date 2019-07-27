






#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONPARENT_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONPARENT_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableTransactionParent.h"
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/PLayerTransactionParent.h"
#include "nsAutoPtr.h"                  
#include "nsTArrayForwardDeclare.h"     

namespace mozilla {

namespace ipc {
class Shmem;
}

namespace layout {
class RenderFrameParent;
}

namespace layers {

class Layer;
class LayerManagerComposite;
class ShadowLayerParent;
class CompositableParent;
class ShadowLayersManager;

class LayerTransactionParent final : public PLayerTransactionParent,
                                     public CompositableParentManager
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef InfallibleTArray<Edit> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;
  typedef InfallibleTArray<AsyncChildMessageData> AsyncChildMessageArray;
  typedef InfallibleTArray<PluginWindowData> PluginsArray;

public:
  LayerTransactionParent(LayerManagerComposite* aManager,
                         ShadowLayersManager* aLayersManager,
                         uint64_t aId);

protected:
  ~LayerTransactionParent();

public:
  void Destroy();

  LayerManagerComposite* layer_manager() const { return mLayerManager; }

  uint64_t GetId() const { return mId; }
  Layer* GetRoot() const { return mRoot; }

  
  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) override {
    return PLayerTransactionParent::AllocShmem(aSize, aType, aShmem);
  }

  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) override {
    return PLayerTransactionParent::AllocUnsafeShmem(aSize, aType, aShmem);
  }

  virtual void DeallocShmem(ipc::Shmem& aShmem) override
  {
    PLayerTransactionParent::DeallocShmem(aShmem);
  }

  virtual LayersBackend GetCompositorBackendType() const override;

  virtual bool IsSameProcess() const override;

  const uint64_t& GetPendingTransactionId() { return mPendingTransaction; }
  void SetPendingTransactionId(uint64_t aId) { mPendingTransaction = aId; }

  
  virtual void SendFenceHandleIfPresent(PTextureParent* aTexture,
                                        CompositableHost* aCompositableHost) override;

  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureParent* aTexture,
                               const FenceHandle& aFence) override;

  virtual void SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage) override;

  virtual base::ProcessId GetChildProcessId() override
  {
    return OtherPid();
  }

  virtual void ReplyRemoveTexture(const OpReplyRemoveTexture& aReply) override;

protected:
  virtual bool RecvShutdown() override;

  virtual bool RecvUpdate(EditArray&& cset,
                          const uint64_t& aTransactionId,
                          const TargetConfig& targetConfig,
                          PluginsArray&& aPlugins,
                          const bool& isFirstPaint,
                          const bool& scheduleComposite,
                          const uint32_t& paintSequenceNumber,
                          const bool& isRepeatTransaction,
                          const mozilla::TimeStamp& aTransactionStart,
                          EditReplyArray* reply) override;

  virtual bool RecvUpdateNoSwap(EditArray&& cset,
                                const uint64_t& aTransactionId,
                                const TargetConfig& targetConfig,
                                PluginsArray&& aPlugins,
                                const bool& isFirstPaint,
                                const bool& scheduleComposite,
                                const uint32_t& paintSequenceNumber,
                                const bool& isRepeatTransaction,
                                const mozilla::TimeStamp& aTransactionStart) override;

  virtual bool RecvClearCachedResources() override;
  virtual bool RecvForceComposite() override;
  virtual bool RecvSetTestSampleTime(const TimeStamp& aTime) override;
  virtual bool RecvLeaveTestMode() override;
  virtual bool RecvGetOpacity(PLayerParent* aParent,
                              float* aOpacity) override;
  virtual bool RecvGetAnimationTransform(PLayerParent* aParent,
                                         MaybeTransform* aTransform)
                                         override;
  virtual bool RecvSetAsyncScrollOffset(const FrameMetrics::ViewID& aId,
                                        const int32_t& aX, const int32_t& aY) override;
  virtual bool RecvGetAPZTestData(APZTestData* aOutData) override;
  virtual bool RecvRequestProperty(const nsString& aProperty, float* aValue) override;
  virtual bool RecvSetConfirmedTargetAPZC(const uint64_t& aBlockId,
                                          nsTArray<ScrollableLayerGuid>&& aTargets) override;

  virtual PLayerParent* AllocPLayerParent() override;
  virtual bool DeallocPLayerParent(PLayerParent* actor) override;

  virtual PCompositableParent* AllocPCompositableParent(const TextureInfo& aInfo) override;
  virtual bool DeallocPCompositableParent(PCompositableParent* actor) override;

  virtual PTextureParent* AllocPTextureParent(const SurfaceDescriptor& aSharedData,
                                              const TextureFlags& aFlags) override;
  virtual bool DeallocPTextureParent(PTextureParent* actor) override;

  virtual bool
  RecvChildAsyncMessages(InfallibleTArray<AsyncChildMessageData>&& aMessages) override;

  virtual void ActorDestroy(ActorDestroyReason why) override;

  bool Attach(ShadowLayerParent* aLayerParent,
              CompositableHost* aCompositable,
              bool aIsAsyncVideo);

  void AddIPDLReference() {
    MOZ_ASSERT(mIPCOpen == false);
    mIPCOpen = true;
    AddRef();
  }
  void ReleaseIPDLReference() {
    MOZ_ASSERT(mIPCOpen == true);
    mIPCOpen = false;
    Release();
  }
  friend class CompositorParent;
  friend class CrossProcessCompositorParent;
  friend class layout::RenderFrameParent;

private:
  nsRefPtr<LayerManagerComposite> mLayerManager;
  ShadowLayersManager* mShadowLayersManager;
  
  
  nsRefPtr<Layer> mRoot;
  
  
  
  
  uint64_t mId;

  uint64_t mPendingTransaction;
  
  
  
  
  
  
  
  
  
  
  
  
  

  bool mDestroyed;

  bool mIPCOpen;
};

} 
} 

#endif 








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

class LayerTransactionParent : public PLayerTransactionParent,
                               public CompositableParentManager
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef InfallibleTArray<Edit> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;
  typedef InfallibleTArray<AsyncChildMessageData> AsyncChildMessageArray;

public:
  LayerTransactionParent(LayerManagerComposite* aManager,
                         ShadowLayersManager* aLayersManager,
                         uint64_t aId,
                         ProcessId aOtherProcess);

protected:
  ~LayerTransactionParent();

public:
  void Destroy();

  LayerManagerComposite* layer_manager() const { return mLayerManager; }

  uint64_t GetId() const { return mId; }
  Layer* GetRoot() const { return mRoot; }

  
  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) {
    return PLayerTransactionParent::AllocShmem(aSize, aType, aShmem);
  }

  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) {
    return PLayerTransactionParent::AllocUnsafeShmem(aSize, aType, aShmem);
  }

  virtual void DeallocShmem(ipc::Shmem& aShmem) MOZ_OVERRIDE
  {
    PLayerTransactionParent::DeallocShmem(aShmem);
  }

  virtual LayersBackend GetCompositorBackendType() const MOZ_OVERRIDE;

  virtual bool IsSameProcess() const MOZ_OVERRIDE;

  const uint64_t& GetPendingTransactionId() { return mPendingTransaction; }
  void SetPendingTransactionId(uint64_t aId) { mPendingTransaction = aId; }

  
  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureParent* aTexture,
                               const FenceHandle& aFence) MOZ_OVERRIDE;

  virtual void SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage) MOZ_OVERRIDE;

  virtual base::ProcessId GetChildProcessId() MOZ_OVERRIDE
  {
    return mChildProcessId;
  }

protected:
  virtual bool RecvUpdate(const EditArray& cset,
                          const uint64_t& aTransactionId,
                          const TargetConfig& targetConfig,
                          const bool& isFirstPaint,
                          const bool& scheduleComposite,
                          const uint32_t& paintSequenceNumber,
                          const bool& isRepeatTransaction,
                          const mozilla::TimeStamp& aTransactionStart,
                          EditReplyArray* reply) MOZ_OVERRIDE;

  virtual bool RecvUpdateNoSwap(const EditArray& cset,
                                const uint64_t& aTransactionId,
                                const TargetConfig& targetConfig,
                                const bool& isFirstPaint,
                                const bool& scheduleComposite,
                                const uint32_t& paintSequenceNumber,
                                const bool& isRepeatTransaction,
                                const mozilla::TimeStamp& aTransactionStart) MOZ_OVERRIDE;

  virtual bool RecvClearCachedResources() MOZ_OVERRIDE;
  virtual bool RecvForceComposite() MOZ_OVERRIDE;
  virtual bool RecvSetTestSampleTime(const TimeStamp& aTime) MOZ_OVERRIDE;
  virtual bool RecvLeaveTestMode() MOZ_OVERRIDE;
  virtual bool RecvGetOpacity(PLayerParent* aParent,
                              float* aOpacity) MOZ_OVERRIDE;
  virtual bool RecvGetAnimationTransform(PLayerParent* aParent,
                                         MaybeTransform* aTransform)
                                         MOZ_OVERRIDE;
  virtual bool RecvSetAsyncScrollOffset(PLayerParent* aLayer, const FrameMetrics::ViewID& aId,
                                        const int32_t& aX, const int32_t& aY) MOZ_OVERRIDE;
  virtual bool RecvGetAPZTestData(APZTestData* aOutData);

  virtual PLayerParent* AllocPLayerParent() MOZ_OVERRIDE;
  virtual bool DeallocPLayerParent(PLayerParent* actor) MOZ_OVERRIDE;

  virtual PCompositableParent* AllocPCompositableParent(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPCompositableParent(PCompositableParent* actor) MOZ_OVERRIDE;

  virtual PTextureParent* AllocPTextureParent(const SurfaceDescriptor& aSharedData,
                                              const TextureFlags& aFlags) MOZ_OVERRIDE;
  virtual bool DeallocPTextureParent(PTextureParent* actor) MOZ_OVERRIDE;

  virtual bool
  RecvChildAsyncMessages(const InfallibleTArray<AsyncChildMessageData>& aMessages) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

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
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  base::ProcessId mChildProcessId;

  bool mDestroyed;

  bool mIPCOpen;
};

} 
} 

#endif 

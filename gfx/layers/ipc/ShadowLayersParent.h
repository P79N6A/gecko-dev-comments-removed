






#ifndef mozilla_layers_ShadowLayersParent_h
#define mozilla_layers_ShadowLayersParent_h

#include "mozilla/layers/PLayersParent.h"
#include "ShadowLayers.h"
#include "ShadowLayersManager.h"
#include "CompositableTransactionParent.h"

namespace mozilla {

namespace layout {
class RenderFrameParent;
}

namespace layers {

class Layer;
class ShadowLayerManager;
class ShadowLayerParent;
class CompositableParent;

class ShadowLayersParent : public PLayersParent,
                           public CompositableParentManager
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef InfallibleTArray<Edit> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;

public:
  ShadowLayersParent(ShadowLayerManager* aManager,
                     ShadowLayersManager* aLayersManager,
                     uint64_t aId);
  ~ShadowLayersParent();

  void Destroy();

  ShadowLayerManager* layer_manager() const { return mLayerManager; }

  uint64_t GetId() const { return mId; }
  ContainerLayer* GetRoot() const { return mRoot; }

  
  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) {
    return PLayersParent::AllocShmem(aSize, aType, aShmem);
  }

  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) {
    return PLayersParent::AllocUnsafeShmem(aSize, aType, aShmem);
  }

  virtual void DeallocShmem(ipc::Shmem& aShmem) MOZ_OVERRIDE
  {
    PLayersParent::DeallocShmem(aShmem);
  }


protected:
  virtual bool RecvUpdate(const EditArray& cset,
                          const TargetConfig& targetConfig,
                          const bool& isFirstPaint,
                          EditReplyArray* reply) MOZ_OVERRIDE;

  virtual bool RecvUpdateNoSwap(const EditArray& cset,
                                const TargetConfig& targetConfig,
                                const bool& isFirstPaint) MOZ_OVERRIDE;

  virtual bool RecvClearCachedResources() MOZ_OVERRIDE;

  virtual PGrallocBufferParent*
  AllocPGrallocBuffer(const gfxIntSize& aSize, const gfxContentType& aContent,
                      MaybeMagicGrallocBufferHandle* aOutHandle) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferParent* actor) MOZ_OVERRIDE;

  virtual PLayerParent* AllocPLayer() MOZ_OVERRIDE;
  virtual bool DeallocPLayer(PLayerParent* actor) MOZ_OVERRIDE;

  virtual PCompositableParent* AllocPCompositable(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPCompositable(PCompositableParent* actor) MOZ_OVERRIDE;
  
  void Attach(ShadowLayerParent* aLayerParent, CompositableParent* aCompositable);

private:
  nsRefPtr<ShadowLayerManager> mLayerManager;
  ShadowLayersManager* mShadowLayersManager;
  
  
  nsRefPtr<ContainerLayer> mRoot;
  
  
  
  
  uint64_t mId;
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mDestroyed;
};

} 
} 

#endif 








#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONPARENT_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONPARENT_H

#include "mozilla/layers/PLayerTransactionParent.h"
#include "ShadowLayers.h"
#include "ShadowLayersManager.h"
#include "CompositableTransactionParent.h"

namespace mozilla {

namespace layout {
class RenderFrameParent;
}

namespace layers {

class Layer;
class LayerManagerComposite;
class ShadowLayerParent;
class CompositableParent;

class LayerTransactionParent : public PLayerTransactionParent,
                               public CompositableParentManager
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef InfallibleTArray<Edit> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;

public:
  LayerTransactionParent(LayerManagerComposite* aManager,
                         ShadowLayersManager* aLayersManager,
                         uint64_t aId);
  ~LayerTransactionParent();

  void Destroy();

  LayerManagerComposite* layer_manager() const { return mLayerManager; }

  uint64_t GetId() const { return mId; }
  ContainerLayer* GetRoot() const { return mRoot; }

  
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
  nsRefPtr<LayerManagerComposite> mLayerManager;
  ShadowLayersManager* mShadowLayersManager;
  
  
  nsRefPtr<ContainerLayer> mRoot;
  
  
  
  
  uint64_t mId;
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mDestroyed;
};

} 
} 

#endif 

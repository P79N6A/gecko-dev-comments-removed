






#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONPARENT_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONPARENT_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableTransactionParent.h"
#include "gfxPoint.h"                   
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/PLayerTransactionParent.h"
#include "nsAutoPtr.h"                  
#include "nsTArrayForwardDeclare.h"     

class gfx3DMatrix;

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

public:
  LayerTransactionParent(LayerManagerComposite* aManager,
                         ShadowLayersManager* aLayersManager,
                         uint64_t aId);
  ~LayerTransactionParent();

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


protected:
  virtual bool RecvUpdate(const EditArray& cset,
                          const TargetConfig& targetConfig,
                          const bool& isFirstPaint,
                          EditReplyArray* reply) MOZ_OVERRIDE;

  virtual bool RecvUpdateNoSwap(const EditArray& cset,
                                const TargetConfig& targetConfig,
                                const bool& isFirstPaint) MOZ_OVERRIDE;

  virtual bool RecvClearCachedResources() MOZ_OVERRIDE;
  virtual bool RecvGetOpacity(PLayerParent* aParent,
                              float* aOpacity) MOZ_OVERRIDE;
  virtual bool RecvGetTransform(PLayerParent* aParent,
                                gfx3DMatrix* aTransform) MOZ_OVERRIDE;

  virtual PGrallocBufferParent*
  AllocPGrallocBufferParent(const gfx::IntSize& aSize,
                      const uint32_t& aFormat, const uint32_t& aUsage,
                      MaybeMagicGrallocBufferHandle* aOutHandle) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBufferParent(PGrallocBufferParent* actor) MOZ_OVERRIDE;

  virtual PLayerParent* AllocPLayerParent() MOZ_OVERRIDE;
  virtual bool DeallocPLayerParent(PLayerParent* actor) MOZ_OVERRIDE;

  virtual PCompositableParent* AllocPCompositableParent(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPCompositableParent(PCompositableParent* actor) MOZ_OVERRIDE;

  void Attach(ShadowLayerParent* aLayerParent,
              CompositableParent* aCompositable,
              bool aIsAsyncVideo);

private:
  nsRefPtr<LayerManagerComposite> mLayerManager;
  ShadowLayersManager* mShadowLayersManager;
  
  
  nsRefPtr<Layer> mRoot;
  
  
  
  
  uint64_t mId;
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mDestroyed;
};

} 
} 

#endif 

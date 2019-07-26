






#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H

#include <stdint.h>                     
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/PLayerTransactionChild.h"
#include "mozilla/RefPtr.h"

namespace mozilla {

namespace layout {
class RenderFrameChild;
}

namespace layers {

class LayerTransactionChild : public PLayerTransactionChild
                            , public AtomicRefCounted<LayerTransactionChild>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(LayerTransactionChild)
  






  void Destroy();

  bool IPCOpen() const { return mIPCOpen; }

protected:
  LayerTransactionChild()
    : mIPCOpen(false)
  {}
  ~LayerTransactionChild() { }
  friend class AtomicRefCounted<LayerTransactionChild>;
  friend class detail::RefCounted<LayerTransactionChild, detail::AtomicRefCount>;

  virtual PGrallocBufferChild*
  AllocPGrallocBufferChild(const IntSize&,
                           const uint32_t&, const uint32_t&,
                           MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBufferChild(PGrallocBufferChild* actor) MOZ_OVERRIDE;

  virtual PLayerChild* AllocPLayerChild() MOZ_OVERRIDE;
  virtual bool DeallocPLayerChild(PLayerChild* actor) MOZ_OVERRIDE;

  virtual PCompositableChild* AllocPCompositableChild(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPCompositableChild(PCompositableChild* actor) MOZ_OVERRIDE;

  virtual PTextureChild* AllocPTextureChild(const SurfaceDescriptor& aSharedData,
                                            const TextureFlags& aFlags) MOZ_OVERRIDE;
  virtual bool DeallocPTextureChild(PTextureChild* actor) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

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
  friend class CompositorChild;
  friend class layout::RenderFrameChild;

  bool mIPCOpen;
};

} 
} 

#endif 

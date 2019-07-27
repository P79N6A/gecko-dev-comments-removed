






#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H

#include <stdint.h>                     
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/AsyncTransactionTracker.h" 
#include "mozilla/layers/PLayerTransactionChild.h"
#include "mozilla/RefPtr.h"

namespace mozilla {

namespace layout {
class RenderFrameChild;
class ShadowLayerForwarder;
}

namespace layers {

class LayerTransactionChild : public PLayerTransactionChild
                            , public AsyncTransactionTrackersHolder
{
  typedef InfallibleTArray<AsyncParentMessageData> AsyncParentMessageArray;
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(LayerTransactionChild)
  







  void Destroy();

  bool IPCOpen() const { return mIPCOpen && !mDestroyed; }

  void SetForwarder(ShadowLayerForwarder* aForwarder)
  {
    mForwarder = aForwarder;
  }

  uint64_t GetId() const { return mId; }

protected:
  explicit LayerTransactionChild(const uint64_t& aId)
    : mForwarder(nullptr)
    , mIPCOpen(false)
    , mDestroyed(false)
    , mId(aId)
  {}
  ~LayerTransactionChild() { }

  virtual PLayerChild* AllocPLayerChild() override;
  virtual bool DeallocPLayerChild(PLayerChild* actor) override;

  virtual PCompositableChild* AllocPCompositableChild(const TextureInfo& aInfo) override;
  virtual bool DeallocPCompositableChild(PCompositableChild* actor) override;

  virtual PTextureChild* AllocPTextureChild(const SurfaceDescriptor& aSharedData,
                                            const TextureFlags& aFlags) override;
  virtual bool DeallocPTextureChild(PTextureChild* actor) override;

  virtual bool
  RecvParentAsyncMessages(InfallibleTArray<AsyncParentMessageData>&& aMessages) override;

  virtual void ActorDestroy(ActorDestroyReason why) override;

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

  ShadowLayerForwarder* mForwarder;
  bool mIPCOpen;
  bool mDestroyed;
  uint64_t mId;
};

} 
} 

#endif 

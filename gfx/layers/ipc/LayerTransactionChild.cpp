






#include "ShadowLayerChild.h"
#include "LayerTransactionChild.h"
#include "ShadowLayerUtils.h"
#include "mozilla/layers/CompositableClient.h"

namespace mozilla {
namespace layers {

void
LayerTransactionChild::Destroy()
{
  NS_ABORT_IF_FALSE(0 == ManagedPLayerChild().Length(),
                    "layers should have been cleaned up by now");
  PLayerTransactionChild::Send__delete__(this);
  
}

PGrallocBufferChild*
LayerTransactionChild::AllocPGrallocBufferChild(const gfxIntSize&,
                                           const uint32_t&,
                                           const uint32_t&,
                                           MaybeMagicGrallocBufferHandle*)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  return GrallocBufferActor::Create();
#else
  NS_RUNTIMEABORT("No gralloc buffers for you");
  return nullptr;
#endif
}

bool
LayerTransactionChild::DeallocPGrallocBufferChild(PGrallocBufferChild* actor)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  delete actor;
  return true;
#else
  NS_RUNTIMEABORT("Um, how did we get here?");
  return false;
#endif
}

PLayerChild*
LayerTransactionChild::AllocPLayerChild()
{
  
  NS_RUNTIMEABORT("not reached");
  return nullptr;
}

bool
LayerTransactionChild::DeallocPLayerChild(PLayerChild* actor)
{
  delete actor;
  return true;
}

PCompositableChild*
LayerTransactionChild::AllocPCompositableChild(const TextureInfo& aInfo)
{
  return new CompositableChild();
}

bool
LayerTransactionChild::DeallocPCompositableChild(PCompositableChild* actor)
{
  delete actor;
  return true;
}

void
LayerTransactionChild::ActorDestroy(ActorDestroyReason why)
{
  if (why == AbnormalShutdown) {
    NS_RUNTIMEABORT("ActorDestroy by IPC channel failure at LayerTransactionChild");
  }
}

}  
}  

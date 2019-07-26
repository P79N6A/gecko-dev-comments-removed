






#include "LayerTransactionChild.h"
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/PCompositableChild.h"  
#include "mozilla/layers/PLayerChild.h"  
#include "mozilla/mozalloc.h"           
#include "nsDebug.h"                    
#include "nsTArray.h"                   

namespace mozilla {
namespace layers {

class PGrallocBufferChild;

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
#ifdef MOZ_B2G
  
  
  
  
  if (why == AbnormalShutdown) {
    NS_RUNTIMEABORT("ActorDestroy by IPC channel failure at LayerTransactionChild");
  }
#endif
}

}  
}  

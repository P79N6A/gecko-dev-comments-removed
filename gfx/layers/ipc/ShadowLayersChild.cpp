






#include "ShadowLayerChild.h"
#include "ShadowLayersChild.h"
#include "ShadowLayerUtils.h"
#include "mozilla/layers/CompositableClient.h"

namespace mozilla {
namespace layers {

void
ShadowLayersChild::Destroy()
{
  NS_ABORT_IF_FALSE(0 == ManagedPLayerChild().Length(),
                    "layers should have been cleaned up by now");
  PLayersChild::Send__delete__(this);
  
}

PGrallocBufferChild*
ShadowLayersChild::AllocPGrallocBuffer(const gfxIntSize&,
                                       const gfxContentType&,
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
ShadowLayersChild::DeallocPGrallocBuffer(PGrallocBufferChild* actor)
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
ShadowLayersChild::AllocPLayer()
{
  
  NS_RUNTIMEABORT("not reached");
  return nullptr;
}

bool
ShadowLayersChild::DeallocPLayer(PLayerChild* actor)
{
  delete actor;
  return true;
}

PCompositableChild*
ShadowLayersChild::AllocPCompositable(const TextureInfo& aInfo)
{
  return new CompositableChild();
}

bool
ShadowLayersChild::DeallocPCompositable(PCompositableChild* actor)
{
  delete actor;
  return true;
}

}  
}  








#include <d3d10_1.h>
#include <dxgi.h>

#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/PLayerTransaction.h"
#include "ShadowLayers.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {



bool
ISurfaceAllocator::PlatformAllocSurfaceDescriptor(const gfxIntSize&,
                                                  gfxContentType,
                                                  uint32_t,
                                                  SurfaceDescriptor*)
{
  return false;
}

 already_AddRefed<gfxASurface>
ShadowLayerForwarder::PlatformOpenDescriptor(OpenMode,
                                             const SurfaceDescriptor&)
{
  return nullptr;
}

 bool
ShadowLayerForwarder::PlatformCloseDescriptor(const SurfaceDescriptor&)
{
  return false;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceContentType(
  const SurfaceDescriptor&,
  OpenMode,
  gfxContentType*,
  gfxASurface**)
{
  return false;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceSize(
  const SurfaceDescriptor&,
  OpenMode,
  gfxIntSize*,
  gfxASurface**)
{
  return false;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceImageFormat(
  const SurfaceDescriptor&,
  OpenMode,
  gfxImageFormat*,
  gfxASurface**)
{
  return false;
}

bool
ShadowLayerForwarder::PlatformDestroySharedSurface(SurfaceDescriptor*)
{
  return false;
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
}

bool
ISurfaceAllocator::PlatformDestroySharedSurface(SurfaceDescriptor*)
{
  return false;
}

 already_AddRefed<TextureImage>
LayerManagerComposite::OpenDescriptorForDirectTexturing(GLContext*,
                                                        const SurfaceDescriptor&,
                                                        GLenum)
{
  return nullptr;
}

 bool
LayerManagerComposite::SupportsDirectTexturing()
{
  return true;
}

 void
LayerManagerComposite::PlatformSyncBeforeReplyUpdate()
{
}

bool
GetDescriptor(ID3D10Texture2D* aTexture, SurfaceDescriptorD3D10* aDescr)
{
  NS_ABORT_IF_FALSE(aTexture && aDescr, "Params must be nonnull");

  HRESULT hr;
  IDXGIResource* dr = nullptr;
  hr = aTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&dr);
  if (!SUCCEEDED(hr) || !dr)
    return false;

  hr = dr->GetSharedHandle(reinterpret_cast<HANDLE*>(&aDescr->handle()));
  return !!SUCCEEDED(hr);
}

already_AddRefed<ID3D10Texture2D>
OpenForeign(ID3D10Device* aDevice, const SurfaceDescriptorD3D10& aDescr)
{
  HRESULT hr;
  ID3D10Texture2D* tex = nullptr;
  hr = aDevice->OpenSharedResource(reinterpret_cast<HANDLE>(aDescr.handle()),
                                   __uuidof(ID3D10Texture2D),
                                   (void**)&tex);
  if (!SUCCEEDED(hr) || !tex)
    return nullptr;

  return nsRefPtr<ID3D10Texture2D>(tex).forget();
}

} 
} 

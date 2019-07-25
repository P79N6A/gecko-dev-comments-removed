






#include <d3d10_1.h>
#include <dxgi.h>

#include "mozilla/layers/PLayers.h"
#include "ShadowLayers.h"

namespace mozilla {
namespace layers {



bool
ShadowLayerForwarder::PlatformAllocBuffer(const gfxIntSize&,
                                          gfxASurface::gfxContentType,
                                          uint32_t,
                                          SurfaceDescriptor*)
{
  return false;
}

 already_AddRefed<gfxASurface>
ShadowLayerForwarder::PlatformOpenDescriptor(OpenMode,
                                             const SurfaceDescriptor&)
{
  return nsnull;
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
ShadowLayerForwarder::PlatformDestroySharedSurface(SurfaceDescriptor*)
{
  return false;
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
}

bool
ShadowLayerManager::PlatformDestroySharedSurface(SurfaceDescriptor*)
{
  return false;
}

 already_AddRefed<TextureImage>
ShadowLayerManager::OpenDescriptorForDirectTexturing(GLContext*,
                                                     const SurfaceDescriptor&,
                                                     GLenum)
{
  return nsnull;
}

 void
ShadowLayerManager::PlatformSyncBeforeReplyUpdate()
{
}

bool
GetDescriptor(ID3D10Texture2D* aTexture, SurfaceDescriptorD3D10* aDescr)
{
  NS_ABORT_IF_FALSE(aTexture && aDescr, "Params must be nonnull");

  HRESULT hr;
  IDXGIResource* dr = nsnull;
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
  ID3D10Texture2D* tex = nsnull;
  hr = aDevice->OpenSharedResource(reinterpret_cast<HANDLE>(aDescr.handle()),
                                   __uuidof(ID3D10Texture2D),
                                   (void**)&tex);
  if (!SUCCEEDED(hr) || !tex)
    return nsnull;

  
  return nsRefPtr<ID3D10Texture2D>(tex).forget();
}

} 
} 

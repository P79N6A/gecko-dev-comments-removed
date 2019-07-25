







































#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"
 
#include "gfxPlatform.h"

#include "gfxXlibSurface.h"
#include "mozilla/X11Util.h"

namespace mozilla {
namespace layers {



static PRBool
UsingXCompositing()
{
  return (gfxASurface::SurfaceTypeXlib ==
          gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType());
}



static XRenderPictFormat*
GetXRenderPictFormatFromId(Display* aDisplay, PictFormat aFormatId)
{
  XRenderPictFormat tmplate;
  tmplate.id = aFormatId;
  return XRenderFindFormat(aDisplay, PictFormatID, &tmplate, 0);
}

static PRBool
TakeAndDestroyXlibSurface(SurfaceDescriptor* aSurface)
{
  nsRefPtr<gfxXlibSurface> surf =
    aSurface->get_SurfaceDescriptorX11().OpenForeign();
  surf->TakePixmap();
  *aSurface = SurfaceDescriptor();
  
  return PR_TRUE;
}

SurfaceDescriptorX11::SurfaceDescriptorX11(gfxXlibSurface* aSurf)
  : mId(aSurf->XDrawable())
  , mSize(aSurf->GetSize())
  , mFormat(aSurf->XRenderFormat()->id)
{ }

SurfaceDescriptorX11::SurfaceDescriptorX11(const int aXid, const int aXrenderPictID, const gfxIntSize& aSize)
  : mId(aXid)
  , mSize(aSize)
  , mFormat(aXrenderPictID)
{ }

already_AddRefed<gfxXlibSurface>
SurfaceDescriptorX11::OpenForeign() const
{
  Display* display = DefaultXDisplay();
  Screen* screen = DefaultScreenOfDisplay(display);

  XRenderPictFormat* format = GetXRenderPictFormatFromId(display, mFormat);
  nsRefPtr<gfxXlibSurface> surf =
    new gfxXlibSurface(screen, mId, format, mSize);
  return surf->CairoStatus() ? nsnull : surf.forget();
}

PRBool
ShadowLayerForwarder::PlatformAllocDoubleBuffer(const gfxIntSize& aSize,
                                                gfxASurface::gfxContentType aContent,
                                                SurfaceDescriptor* aFrontBuffer,
                                                SurfaceDescriptor* aBackBuffer)
{
  return (PlatformAllocBuffer(aSize, aContent, aFrontBuffer) &&
          PlatformAllocBuffer(aSize, aContent, aBackBuffer));
}

PRBool
ShadowLayerForwarder::PlatformAllocBuffer(const gfxIntSize& aSize,
                                          gfxASurface::gfxContentType aContent,
                                          SurfaceDescriptor* aBuffer)
{
  if (!UsingXCompositing()) {
    
    
    
    return PR_FALSE;
  }

  gfxPlatform* platform = gfxPlatform::GetPlatform();
  nsRefPtr<gfxASurface> buffer = platform->CreateOffscreenSurface(aSize, aContent);
  if (!buffer ||
      buffer->GetType() != gfxASurface::SurfaceTypeXlib) {
    NS_ERROR("creating Xlib front/back surfaces failed!");
    return PR_FALSE;
  }

  gfxXlibSurface* bufferX = static_cast<gfxXlibSurface*>(buffer.get());
  
  bufferX->ReleasePixmap();

  *aBuffer = SurfaceDescriptorX11(bufferX);
  return PR_TRUE;
}

 already_AddRefed<gfxASurface>
ShadowLayerForwarder::PlatformOpenDescriptor(const SurfaceDescriptor& aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorX11 != aSurface.type()) {
    return nsnull;
  }
  return aSurface.get_SurfaceDescriptorX11().OpenForeign();
}

PRBool
ShadowLayerForwarder::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorX11 != aSurface->type()) {
    return PR_FALSE;
  }
  return TakeAndDestroyXlibSurface(aSurface);
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
  if (UsingXCompositing()) {
    
    
    
    
    XSync(DefaultXDisplay(), False);
  }
}

 void
ShadowLayerManager::PlatformSyncBeforeReplyUpdate()
{
  if (UsingXCompositing()) {
    
    
    
    
    
    XSync(DefaultXDisplay(), False);
  }
}

PRBool
ShadowLayerManager::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorX11 != aSurface->type()) {
    return PR_FALSE;
  }
  return TakeAndDestroyXlibSurface(aSurface);
}

} 
} 

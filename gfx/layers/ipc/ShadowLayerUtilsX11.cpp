







































#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"
 
#include "gfxPlatform.h"

#include "gfxXlibSurface.h"
#include "mozilla/X11Util.h"

namespace mozilla {
namespace layers {



static XRenderPictFormat*
GetXRenderPictFormatFromId(Display* aDisplay, PictFormat aFormatId)
{
  XRenderPictFormat tmplate;
  tmplate.id = aFormatId;
  return XRenderFindFormat(aDisplay, PictFormatID, &tmplate, 0);
}



static already_AddRefed<gfxXlibSurface>
CreateSimilar(gfxXlibSurface* aReference,
              gfxASurface::gfxContentType aType, const gfxIntSize& aSize)
{
  Display* display = aReference->XDisplay();
  XRenderPictFormat* xrenderFormat = nsnull;

  
  if (aReference->GetContentType() == aType) {
    xrenderFormat = aReference->XRenderFormat();
  } else {
    
    gfxASurface::gfxImageFormat format;
    switch (aType) {
    case gfxASurface::CONTENT_COLOR:
      
      format = gfxASurface::ImageFormatRGB24; break;
    case gfxASurface::CONTENT_ALPHA:
      format = gfxASurface::ImageFormatA8; break;
    case gfxASurface::CONTENT_COLOR_ALPHA:
      format = gfxASurface::ImageFormatARGB32; break;
    default:
      NS_NOTREACHED("unknown gfxContentType");
    }
    xrenderFormat = gfxXlibSurface::FindRenderFormat(display, format);
  }
  NS_ABORT_IF_FALSE(xrenderFormat, "should have a render format by now");

  return gfxXlibSurface::Create(aReference->XScreen(), xrenderFormat,
                                aSize, aReference->XDrawable());
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
  gfxASurface* reference = gfxPlatform::GetPlatform()->ScreenReferenceSurface();
  NS_ABORT_IF_FALSE(reference->GetType() == gfxASurface::SurfaceTypeXlib,
                    "can't alloc an Xlib surface on non-X11");
  gfxXlibSurface* xlibRef = static_cast<gfxXlibSurface*>(reference);

  nsRefPtr<gfxXlibSurface> front = CreateSimilar(xlibRef, aContent, aSize);
  nsRefPtr<gfxXlibSurface> back = CreateSimilar(xlibRef, aContent, aSize);
  if (!front || !back) {
    NS_ERROR("creating Xlib front/back surfaces failed!");
    return PR_FALSE;
  }

  
  front->ReleasePixmap();
  back->ReleasePixmap();

  *aFrontBuffer = SurfaceDescriptorX11(front);
  *aBackBuffer = SurfaceDescriptorX11(back);
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
  XSync(DefaultXDisplay(), False);
}

 void
ShadowLayerManager::PlatformSyncBeforeReplyUpdate()
{
  XSync(DefaultXDisplay(), False);
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

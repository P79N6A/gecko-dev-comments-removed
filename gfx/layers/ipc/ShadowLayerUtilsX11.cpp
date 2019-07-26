






#include "ShadowLayerUtilsX11.h"
#include <X11/X.h>                      
#include <X11/Xlib.h>                   
#include <X11/extensions/Xrender.h>     
#include <X11/extensions/render.h>      
#include "cairo-xlib.h"
#include <stdint.h>                     
#include "GLDefs.h"                     
#include "gfxASurface.h"                
#include "gfxPlatform.h"                
#include "gfxXlibSurface.h"             
#include "gfx2DGlue.h"                  
#include "mozilla/X11Util.h"            
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/ShadowLayers.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "prenv.h"                      

using namespace mozilla::gl;

namespace mozilla {
namespace gl {
class GLContext;
class TextureImage;
}

namespace layers {



static bool
UsingXCompositing()
{
  if (!PR_GetEnv("MOZ_LAYERS_ENABLE_XLIB_SURFACES")) {
      return false;
  }
  return (gfxSurfaceType::Xlib ==
          gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType());
}



static XRenderPictFormat*
GetXRenderPictFormatFromId(Display* aDisplay, PictFormat aFormatId)
{
  XRenderPictFormat tmplate;
  tmplate.id = aFormatId;
  return XRenderFindFormat(aDisplay, PictFormatID, &tmplate, 0);
}

SurfaceDescriptorX11::SurfaceDescriptorX11(gfxXlibSurface* aSurf)
  : mId(aSurf->XDrawable())
  , mSize(aSurf->GetSize().ToIntSize())
{
  const XRenderPictFormat *pictFormat = aSurf->XRenderFormat();
  if (pictFormat) {
    mFormat = pictFormat->id;
  } else {
    mFormat = cairo_xlib_surface_get_visual(aSurf->CairoSurface())->visualid;
  }
}

SurfaceDescriptorX11::SurfaceDescriptorX11(Drawable aDrawable, XID aFormatID,
                                           const gfx::IntSize& aSize)
  : mId(aDrawable)
  , mFormat(aFormatID)
  , mSize(aSize)
{ }

already_AddRefed<gfxXlibSurface>
SurfaceDescriptorX11::OpenForeign() const
{
  Display* display = DefaultXDisplay();
  Screen* screen = DefaultScreenOfDisplay(display);

  nsRefPtr<gfxXlibSurface> surf;
  XRenderPictFormat* pictFormat = GetXRenderPictFormatFromId(display, mFormat);
  if (pictFormat) {
    surf = new gfxXlibSurface(screen, mId, pictFormat, gfx::ThebesIntSize(mSize));
  } else {
    Visual* visual;
    int depth;
    FindVisualAndDepth(display, mFormat, &visual, &depth);
    if (!visual)
      return nullptr;

    surf = new gfxXlibSurface(display, mId, visual, gfx::ThebesIntSize(mSize));
  }
  return surf->CairoStatus() ? nullptr : surf.forget();
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
  if (UsingXCompositing()) {
    
    
    
    
    FinishX(DefaultXDisplay());
  }
}

 void
LayerManagerComposite::PlatformSyncBeforeReplyUpdate()
{
  if (UsingXCompositing()) {
    
    
    
    
    
    FinishX(DefaultXDisplay());
  }
}

 bool
LayerManagerComposite::SupportsDirectTexturing()
{
  return false;
}

} 
} 

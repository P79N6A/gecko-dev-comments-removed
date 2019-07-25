




































#include "gfxDirectFBSurface.h"

#include "cairo-directfb.h"

gfxDirectFBSurface::gfxDirectFBSurface(IDirectFB *dfb, IDirectFBSurface *dfbs)
    : mDFB(nsnull), mDFBSurface(nsnull)
{
    dfb->AddRef( dfb );
    dfbs->AddRef( dfbs );

    cairo_surface_t *surf = cairo_directfb_surface_create(dfb, dfbs);

    mDFB = dfb;
    mDFBSurface = dfbs;

    Init(surf);
}

gfxDirectFBSurface::gfxDirectFBSurface(IDirectFBSurface *dfbs)
    : mDFB(nsnull), mDFBSurface(nsnull)
{
    DFBResult ret;

    dfbs->AddRef( dfbs );

    
    ret = DirectFBCreate( &mDFB );
    if (ret) {
         D_DERROR( (DirectResult) ret, "gfxDirectFBSurface: DirectFBCreate() failed!\n" );
         return;
    }

    cairo_surface_t *surf = cairo_directfb_surface_create(mDFB, dfbs);

    mDFBSurface = dfbs;

    Init(surf);
}

gfxDirectFBSurface::gfxDirectFBSurface(cairo_surface_t *csurf)
{
    mDFB = nsnull;
    mDFBSurface = nsnull;

    Init(csurf, true);
}

gfxDirectFBSurface::gfxDirectFBSurface(const gfxIntSize& size, gfxImageFormat format) :
    mDFB(nsnull), mDFBSurface(nsnull)
{
     DFBResult             ret;
     DFBSurfaceDescription desc;

     if (!CheckSurfaceSize(size) || size.width <= 0 || size.height <= 0)
          return;

     
     ret = DirectFBCreate( &mDFB );
     if (ret) {
          D_DERROR( (DirectResult) ret, "gfxDirectFBSurface: DirectFBCreate() failed!\n" );
          return;
     }

     desc.flags  = (DFBSurfaceDescriptionFlags)( DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT );
     desc.width  = size.width;
     desc.height = size.height;

     switch (format) {
          case gfxASurface::ImageFormatARGB32:
               desc.pixelformat = DSPF_ARGB;
               break;

          case gfxASurface::ImageFormatRGB24:
               desc.pixelformat = DSPF_RGB32;
               break;

          case gfxASurface::ImageFormatA8:
               desc.pixelformat = DSPF_A8;
               break;

          case gfxASurface::ImageFormatA1:
               desc.pixelformat = DSPF_A1;
               break;

          default:
               D_BUG( "unknown format" );
               return;
     }

     ret = mDFB->CreateSurface( mDFB, &desc, &mDFBSurface );
     if (ret) {
          D_DERROR( (DirectResult) ret, "gfxDirectFBSurface: "
                                        "IDirectFB::CreateSurface( %dx%d ) failed!\n", desc.width, desc.height );
          return;
     }

     cairo_surface_t *surface = cairo_directfb_surface_create(mDFB, mDFBSurface);

     Init(surface);
}

gfxDirectFBSurface::~gfxDirectFBSurface()
{
     if (mDFBSurface)
          mDFBSurface->Release( mDFBSurface );

     if (mDFB)
          mDFB->Release( mDFB );
}


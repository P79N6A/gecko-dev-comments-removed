




#include "gfxPSSurface.h"

#include "cairo.h"
#include "cairo-ps.h"

static cairo_status_t
write_func(void *closure, const unsigned char *data, unsigned int length)
{
    nsCOMPtr<nsIOutputStream> out = reinterpret_cast<nsIOutputStream*>(closure);
    do {
        uint32_t wrote = 0;
        if (NS_FAILED(out->Write((const char*)data, length, &wrote)))
            break;
        data += wrote; length -= wrote;
    } while (length > 0);
    NS_ASSERTION(length == 0, "not everything was written to the file");
    return CAIRO_STATUS_SUCCESS;
}

gfxPSSurface::gfxPSSurface(nsIOutputStream *aStream, const gfxSize& aSizeInPoints, PageOrientation aOrientation)
    : mStream(aStream), mXDPI(-1), mYDPI(-1), mOrientation(aOrientation)
{
    mSize = gfxIntSize(aSizeInPoints.width, aSizeInPoints.height);

    
    
    
    
    
    gfxIntSize cairoSize;
    if (mOrientation == PORTRAIT) {
        cairoSize = mSize;
    } else {
        cairoSize = gfxIntSize(mSize.height, mSize.width);
    }
    cairo_surface_t* ps_surface = cairo_ps_surface_create_for_stream(write_func, (void*)mStream, cairoSize.width, cairoSize.height);
    cairo_ps_surface_restrict_to_level(ps_surface, CAIRO_PS_LEVEL_2);
    Init(ps_surface);
}

gfxPSSurface::~gfxPSSurface()
{
}

nsresult
gfxPSSurface::BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName)
{
    if (mOrientation == PORTRAIT) {
      cairo_ps_surface_dsc_comment (mSurface, "%%Orientation: Portrait");
    } else {
      cairo_ps_surface_dsc_comment (mSurface, "%%Orientation: Landscape");
    }
    return NS_OK;
}

nsresult
gfxPSSurface::EndPrinting()
{
    return NS_OK;
}

nsresult
gfxPSSurface::AbortPrinting()
{
    return NS_OK;
}

nsresult
gfxPSSurface::BeginPage()
{
    return NS_OK;
}

nsresult
gfxPSSurface::EndPage()
{
    cairo_surface_show_page(CairoSurface());
    return NS_OK;
}

void
gfxPSSurface::Finish()
{
    gfxASurface::Finish();
    mStream->Close();
}

void
gfxPSSurface::SetDPI(double xDPI, double yDPI)
{
    mXDPI = xDPI;
    mYDPI = yDPI;
    cairo_surface_set_fallback_resolution(CairoSurface(), xDPI, yDPI);
}

void
gfxPSSurface::GetDPI(double *xDPI, double *yDPI)
{
    *xDPI = mXDPI;
    *yDPI = mYDPI;
}


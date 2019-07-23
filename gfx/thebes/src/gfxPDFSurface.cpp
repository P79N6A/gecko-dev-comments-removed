




































#include "gfxPDFSurface.h"

#include "cairo.h"
#include "cairo-pdf.h"

static cairo_status_t
write_func(void *closure, const unsigned char *data, unsigned int length)
{
    nsCOMPtr<nsIOutputStream> out = reinterpret_cast<nsIOutputStream*>(closure);
    do {
        PRUint32 wrote = 0;
        if (NS_FAILED(out->Write((const char*)data, length, &wrote)))
            break;
        data += wrote; length -= wrote;
    } while (length > 0);
    NS_ASSERTION(length == 0, "not everything was written to the file");
    return CAIRO_STATUS_SUCCESS;
}

gfxPDFSurface::gfxPDFSurface(nsIOutputStream *aStream, const gfxSize& aSizeInPoints)
    : mStream(aStream), mXDPI(-1), mYDPI(-1), mSize(aSizeInPoints)
{
    Init(cairo_pdf_surface_create_for_stream(write_func, (void*)mStream, mSize.width, mSize.height));
}

gfxPDFSurface::~gfxPDFSurface()
{
}

nsresult
gfxPDFSurface::BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName)
{
    return NS_OK;
}

nsresult
gfxPDFSurface::EndPrinting()
{
    return NS_OK;
}

nsresult
gfxPDFSurface::AbortPrinting()
{
    return NS_OK;
}

nsresult
gfxPDFSurface::BeginPage()
{
    return NS_OK;
}

nsresult
gfxPDFSurface::EndPage()
{
    cairo_t *cx = cairo_create(CairoSurface());
    cairo_show_page(cx);
    cairo_destroy(cx);
    return NS_OK;
}

void
gfxPDFSurface::Finish()
{
    gfxASurface::Finish();
    mStream->Close();
}

void
gfxPDFSurface::SetDPI(double xDPI, double yDPI)
{
    mXDPI = xDPI;
    mYDPI = yDPI;
    cairo_surface_set_fallback_resolution(CairoSurface(), xDPI, yDPI);
}

void
gfxPDFSurface::GetDPI(double *xDPI, double *yDPI)
{
    *xDPI = mXDPI;
    *yDPI = mYDPI;
}

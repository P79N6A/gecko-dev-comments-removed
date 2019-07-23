




































#include "gfxWindowsSurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"

#include "cairo.h"
#include "cairo-win32.h"

#include "nsString.h"

gfxWindowsSurface::gfxWindowsSurface(HWND wnd) :
    mOwnsDC(PR_TRUE), mForPrinting(PR_FALSE), mWnd(wnd)
{
    mDC = ::GetDC(mWnd);
    Init(cairo_win32_surface_create(mDC));
}

gfxWindowsSurface::gfxWindowsSurface(HDC dc, PRUint32 flags) :
    mOwnsDC(PR_FALSE), mForPrinting(PR_FALSE), mDC(dc), mWnd(nsnull)
{
    if (flags & FLAG_TAKE_DC)
        mOwnsDC = PR_TRUE;

#ifdef NS_PRINTING
    if (flags & FLAG_FOR_PRINTING) {
        Init(cairo_win32_printing_surface_create(mDC));
        mForPrinting = PR_TRUE;
    } else
#endif
        Init(cairo_win32_surface_create(mDC));
}

gfxWindowsSurface::gfxWindowsSurface(const gfxIntSize& size, gfxImageFormat imageFormat) :
    mOwnsDC(PR_FALSE), mForPrinting(PR_FALSE), mWnd(nsnull)
{
    if (!CheckSurfaceSize(size))
        return;

    cairo_surface_t *surf = cairo_win32_surface_create_with_dib((cairo_format_t)imageFormat,
                                                                size.width, size.height);
    Init(surf);

    if (CairoStatus() == 0)
        mDC = cairo_win32_surface_get_dc(CairoSurface());
    else
        mDC = nsnull;
}

gfxWindowsSurface::gfxWindowsSurface(HDC dc, const gfxIntSize& size, gfxImageFormat imageFormat) :
    mOwnsDC(PR_FALSE), mForPrinting(PR_FALSE), mWnd(nsnull)
{
    if (!CheckSurfaceSize(size))
        return;

    cairo_surface_t *surf = cairo_win32_surface_create_with_ddb(dc, (cairo_format_t)imageFormat,
                                                                size.width, size.height);
    Init(surf);

    if (CairoStatus() == 0)
        mDC = cairo_win32_surface_get_dc(CairoSurface());
    else
        mDC = nsnull;
}


gfxWindowsSurface::gfxWindowsSurface(cairo_surface_t *csurf) :
    mOwnsDC(PR_FALSE), mForPrinting(PR_FALSE), mWnd(nsnull)
{
    if (cairo_surface_status(csurf) == 0)
        mDC = cairo_win32_surface_get_dc(csurf);
    else
        mDC = nsnull;

    if (cairo_surface_get_type(csurf) == CAIRO_SURFACE_TYPE_WIN32_PRINTING)
        mForPrinting = PR_TRUE;

    Init(csurf, PR_TRUE);
}

gfxWindowsSurface::~gfxWindowsSurface()
{
    if (mOwnsDC) {
        if (mWnd)
            ::ReleaseDC(mWnd, mDC);
        else
            ::DeleteDC(mDC);
    }
}

already_AddRefed<gfxImageSurface>
gfxWindowsSurface::GetImageSurface()
{
    if (!mSurfaceValid) {
        NS_WARNING ("GetImageSurface on an invalid (null) surface; who's calling this without checking for surface errors?");
        return nsnull;
    }

    NS_ASSERTION(CairoSurface() != nsnull, "CairoSurface() shouldn't be nsnull when mSurfaceValid is TRUE!");

    if (mForPrinting)
        return nsnull;

    cairo_surface_t *isurf = cairo_win32_surface_get_image(CairoSurface());
    if (!isurf)
        return nsnull;

    nsRefPtr<gfxASurface> asurf = gfxASurface::Wrap(isurf);
    gfxImageSurface *imgsurf = (gfxImageSurface*) asurf.get();
    NS_ADDREF(imgsurf);
    return imgsurf;
}

already_AddRefed<gfxWindowsSurface>
gfxWindowsSurface::OptimizeToDDB(HDC dc, const gfxIntSize& size, gfxImageFormat format)
{
    if (mForPrinting)
        return nsnull;

    if (format != ImageFormatRGB24)
        return nsnull;

    nsRefPtr<gfxWindowsSurface> wsurf = new gfxWindowsSurface(dc, size, format);
    if (wsurf->CairoStatus() != 0)
        return nsnull;

    gfxContext tmpCtx(wsurf);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx.SetSource(this);
    tmpCtx.Paint();

    gfxWindowsSurface *raw = (gfxWindowsSurface*) (wsurf.get());
    NS_ADDREF(raw);

    
    
    cairo_win32_surface_set_can_convert_to_dib(raw->CairoSurface(), TRUE);

    return raw;
}

nsresult
gfxWindowsSurface::BeginPrinting(const nsAString& aTitle,
                                 const nsAString& aPrintToFileName)
{
#ifdef NS_PRINTING
#define DOC_TITLE_LENGTH 30
    DOCINFOW docinfo;

    nsString titleStr(aTitle);
    if (titleStr.Length() > DOC_TITLE_LENGTH) {
        titleStr.SetLength(DOC_TITLE_LENGTH-3);
        titleStr.AppendLiteral("...");
    }

    nsString docName(aPrintToFileName);
    docinfo.cbSize = sizeof(docinfo);
    docinfo.lpszDocName = titleStr.Length() > 0 ? titleStr.get() : L"Mozilla Document";
    docinfo.lpszOutput = docName.Length() > 0 ? docName.get() : nsnull;
    docinfo.lpszDatatype = NULL;
    docinfo.fwType = 0;

    ::StartDocW(mDC, &docinfo);

    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::EndPrinting()
{
#ifdef NS_PRINTING
    int result = ::EndDoc(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;

    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::AbortPrinting()
{
#ifdef NS_PRINTING
    int result = ::AbortDoc(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::BeginPage()
{
#ifdef NS_PRINTING
    int result = ::StartPage(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::EndPage()
{
#ifdef NS_PRINTING
    if (mForPrinting)
        cairo_surface_show_page(CairoSurface());
    int result = ::EndPage(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

PRInt32
gfxWindowsSurface::GetDefaultContextFlags() const
{
    if (mForPrinting)
        return gfxContext::FLAG_SIMPLIFY_OPERATORS |
               gfxContext::FLAG_DISABLE_SNAPPING;

    return 0;
}

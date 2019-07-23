






































#include "gfxOS2Platform.h"
#include "gfxOS2Surface.h"
#include "gfxImageSurface.h"
#include "gfxOS2Fonts.h"

#include "gfxFontconfigUtils.h"





gfxFontconfigUtils *gfxOS2Platform::sFontconfigUtils = nsnull;

gfxOS2Platform::gfxOS2Platform()
    : mDC(NULL), mPS(NULL), mBitmap(NULL)
{
#ifdef DEBUG_thebes
    printf("gfxOS2Platform::gfxOS2Platform()\n");
#endif
    
    
    cairo_os2_init();
#ifdef DEBUG_thebes
    printf("  cairo_os2_init() was called\n");
#endif
    if (!sFontconfigUtils) {
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();
    }
}

gfxOS2Platform::~gfxOS2Platform()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Platform::~gfxOS2Platform()\n");
#endif
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nsnull;

    if (mBitmap) {
        GpiSetBitmap(mPS, NULL);
        GpiDeleteBitmap(mBitmap);
    }
    if (mPS) {
        GpiDestroyPS(mPS);
    }
    if (mDC) {
        DevCloseDC(mDC);
    }
    
    cairo_os2_fini();
#ifdef DEBUG_thebes
    printf("  cairo_os2_fini() was called\n");
#endif
}

already_AddRefed<gfxASurface>
gfxOS2Platform::CreateOffscreenSurface(const gfxIntSize& aSize,
                                       gfxASurface::gfxImageFormat aImageFormat)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Platform::CreateOffscreenSurface(%d/%d, %d)\n",
           aSize.width, aSize.height, aImageFormat);
#endif
    gfxASurface *newSurface = nsnull;

    
    
    if (aImageFormat == gfxASurface::ImageFormatARGB32 ||
        aImageFormat == gfxASurface::ImageFormatRGB24)
    {
        
        DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
        SIZEL sizel = { 0, 0 }; 
        mDC = DevOpenDC(0, OD_MEMORY, "*", 5, (PDEVOPENDATA)&dop, NULLHANDLE);
        if (mDC != DEV_ERROR) {
            mPS = GpiCreatePS(0, mDC, &sizel, PU_PELS | GPIT_MICRO | GPIA_ASSOC);
            if (mPS != GPI_ERROR) {
                
                

                
                BITMAPINFOHEADER2 hdr = { 0 };
                hdr.cbFix = sizeof(BITMAPINFOHEADER2);
                hdr.cx = aSize.width;
                hdr.cy = aSize.height;
                hdr.cPlanes = 1;

                
                LONG lBitCount = 0;
                DevQueryCaps(mDC, CAPS_COLOR_BITCOUNT, 1, &lBitCount);
                hdr.cBitCount = (USHORT)lBitCount;

                mBitmap = GpiCreateBitmap(mPS, &hdr, 0, 0, 0);
                if (mBitmap != GPI_ERROR) {
                    
                    GpiSetBitmap(mPS, mBitmap);
                }
            } 
        } 
        newSurface = new gfxOS2Surface(mPS, aSize);
    } else if (aImageFormat == gfxASurface::ImageFormatA8 ||
               aImageFormat == gfxASurface::ImageFormatA1) {
        newSurface = new gfxImageSurface(aSize, aImageFormat);
    } else {
        return nsnull;
    }

    NS_IF_ADDREF(newSurface);
    return newSurface;
}

nsresult
gfxOS2Platform::GetFontList(const nsACString& aLangGroup,
                            const nsACString& aGenericFamily,
                            nsStringArray& aListOfFonts)
{
#ifdef DEBUG_thebes
    char *langgroup = ToNewCString(aLangGroup),
         *family = ToNewCString(aGenericFamily);
    printf("gfxOS2Platform::GetFontList(%s, %s, ..)\n",
           langgroup, family);
    free(langgroup);
    free(family);
#endif
    return sFontconfigUtils->GetFontList(aLangGroup, aGenericFamily,
                                         aListOfFonts);
    
}

nsresult gfxOS2Platform::UpdateFontList()
{
    return sFontconfigUtils->UpdateFontList();
}

nsresult
gfxOS2Platform::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure, PRBool& aAborted)
{
#ifdef DEBUG_thebes
    char *fontname = ToNewCString(aFontName);
    printf("gfxOS2Platform::ResolveFontName(%s, ...)\n", fontname);
    free(fontname);
#endif
    return sFontconfigUtils->ResolveFontName(aFontName, aCallback, aClosure,
                                             aAborted);
    
    
}

gfxFontGroup *
gfxOS2Platform::CreateFontGroup(const nsAString &aFamilies,
				const gfxFontStyle *aStyle)
{
    return new gfxOS2FontGroup(aFamilies, aStyle);
}

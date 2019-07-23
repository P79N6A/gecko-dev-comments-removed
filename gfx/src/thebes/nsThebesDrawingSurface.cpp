





































#include "nsThebesDrawingSurface.h"
#include "nsThebesDeviceContext.h"

#include "nsMemory.h"

#include "gfxPlatform.h"

#include "gfxImageSurface.h"

#ifdef MOZ_ENABLE_GTK2
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "gfxXlibSurface.h"
#endif

#include <stdlib.h>

NS_IMPL_ISUPPORTS1(nsThebesDrawingSurface, nsIDrawingSurface)

nsThebesDrawingSurface::nsThebesDrawingSurface()
{
}

nsThebesDrawingSurface::~nsThebesDrawingSurface()
{
    
    
    mSurface = nsnull;
}

nsresult
nsThebesDrawingSurface::Init(nsThebesDeviceContext *aDC, gfxASurface *aSurface)
{
    mDC = aDC;
    mSurface = aSurface;

    
    mWidth = 0;
    mHeight = 0;

    return NS_OK;
}

nsresult
nsThebesDrawingSurface::Init(nsThebesDeviceContext *aDC, PRUint32 aWidth, PRUint32 aHeight, PRBool aFastAccess)
{
    NS_ASSERTION(mSurface == nsnull, "Surface already initialized!");
    NS_ASSERTION(aWidth > 0 && aHeight > 0, "Invalid surface dimensions!");

    PR_LOG(gThebesGFXLog, PR_LOG_DEBUG, ("## %p nsThebesDrawingSurface::Init aDC %p w %d h %d fast %d\n", this, aDC, aWidth, aHeight, aFastAccess));

    mWidth = aWidth;
    mHeight = aHeight;
    mDC = aDC;

    
    
    
    mSurface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(aWidth, aHeight), gfxImageSurface::ImageFormatARGB32);

    return NS_OK;
}

nsresult
nsThebesDrawingSurface::Init (nsThebesDeviceContext *aDC, nsIWidget *aWidget)
{
    NS_ERROR("Should never be called.");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsThebesDrawingSurface::Init (nsThebesDeviceContext *aDC, nsNativeWidget aWidget)
{
    NS_ERROR("Should never be called.");
    return NS_ERROR_NOT_IMPLEMENTED;
}

#ifdef MOZ_ENABLE_GTK2
static nsresult ConvertPixmapsGTK(GdkPixmap* aPmBlack, GdkPixmap* aPmWhite,
                                  const nsIntSize& aSize, PRUint8* aData);
#endif

nsresult
nsThebesDrawingSurface::Init (nsThebesDeviceContext *aDC,
                              void* aNativePixmapBlack,
                              void* aNativePixmapWhite,
                              const nsIntSize& aSrcSize)
{
    PR_LOG(gThebesGFXLog, PR_LOG_DEBUG, ("## %p nsThebesDrawingSurface::Init aDC %p nativeBlack %p nativeWhite %p size [%d,%d]\n", this, aDC, aNativePixmapBlack, aNativePixmapWhite, aSrcSize.width, aSrcSize.height));

    mWidth = aSrcSize.width;
    mHeight = aSrcSize.height;

#ifdef MOZ_ENABLE_GTK2
    nsresult rv;
    nsRefPtr<gfxImageSurface> imgSurf = new gfxImageSurface(gfxIntSize(mWidth, mHeight), gfxImageSurface::ImageFormatARGB32);

    GdkPixmap* pmBlack = NS_STATIC_CAST(GdkPixmap*, aNativePixmapBlack);
    GdkPixmap* pmWhite = NS_STATIC_CAST(GdkPixmap*, aNativePixmapWhite);

    rv = ConvertPixmapsGTK(pmBlack, pmWhite, aSrcSize, imgSurf->Data());
    if (NS_FAILED(rv))
        return rv;

    mSurface = imgSurf;
#else
    NS_ERROR ("nsThebesDrawingSurface init from black/white pixmaps; either implement this or fix the caller");
#endif

    return NS_OK;
}

nsresult
nsThebesDrawingSurface::PushFilter(const nsIntRect& aRect, PRBool aAreaIsOpaque, float aOpacity)
{
    return NS_OK;
}

void
nsThebesDrawingSurface::PopFilter()
{
    
}

NS_IMETHODIMP
nsThebesDrawingSurface::Lock (PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                             void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                             PRUint32 aFlags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsThebesDrawingSurface::Unlock (void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsThebesDrawingSurface::GetDimensions (PRUint32 *aWidth, PRUint32 *aHeight)
{
    if (mWidth == 0 && mHeight == 0)
        NS_ERROR("nsThebesDrawingSurface::GetDimensions on a surface for which we don't know width/height!");
    *aWidth = mWidth;
    *aHeight = mHeight;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesDrawingSurface::IsOffscreen(PRBool *aOffScreen)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsThebesDrawingSurface::IsPixelAddressable(PRBool *aAddressable)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsThebesDrawingSurface::GetPixelFormat(nsPixelFormat *aFormat)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}







static PRInt32 BuildARGB(PRUint8 aR, PRUint8 aG, PRUint8 aB, PRUint8 aA)
{
    return (aA << 24) | (aR << 16) | (aG << 8) | aB;
}

#ifdef MOZ_ENABLE_GTK2
static nsresult ConvertPixmapsGTK(GdkPixmap* aPmBlack, GdkPixmap* aPmWhite,
                                  const nsIntSize& aSize, PRUint8* aData)
{
    GdkImage *imgBlack = gdk_image_get(aPmBlack, 0, 0,
                                       aSize.width,
                                       aSize.height);
    GdkImage *imgWhite = gdk_image_get(aPmWhite, 0, 0,
                                       aSize.width,
                                       aSize.height);

    if (!imgBlack || !imgWhite)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint8* blackData = (PRUint8*)GDK_IMAGE_XIMAGE(imgBlack)->data;
    PRUint8* whiteData = (PRUint8*)GDK_IMAGE_XIMAGE(imgWhite)->data;
    PRInt32 bpp = GDK_IMAGE_XIMAGE(imgBlack)->bits_per_pixel;
    PRInt32 stride = GDK_IMAGE_XIMAGE(imgBlack)->bytes_per_line;
    PRInt32* argb = (PRInt32*)aData;

    if (bpp == 24 || bpp == 32) {
        PRInt32 pixSize = bpp/8;
        for (PRInt32 y = 0; y < aSize.height; ++y) {
            PRUint8* blackRow = blackData + y*stride;
            PRUint8* whiteRow = whiteData + y*stride;
            for (PRInt32 x = 0; x < aSize.width; ++x) {
                PRUint8 alpha = 0;
                if (blackRow[0] == whiteRow[0] &&
                    blackRow[1] == whiteRow[1] &&
                    blackRow[2] == whiteRow[2]) {
                    alpha = 0xFF;
                }
                *argb++ = BuildARGB(blackRow[2], blackRow[1],
                                    blackRow[0], alpha);
                blackRow += pixSize;
                whiteRow += pixSize;
            }
        }
    } else {
        NS_ERROR("Unhandled bpp!");
    }

    gdk_image_unref(imgBlack);
    gdk_image_unref(imgWhite);

    return NS_OK;
}
#endif









































#include "nsDrawingSurfaceQt.h"
#include "nsRenderingContextQt.h"
#include <qpaintdevicemetrics.h>

#include "qtlog.h"

NS_IMPL_ISUPPORTS1(nsDrawingSurfaceQt, nsIDrawingSurface)

nsDrawingSurfaceQt::nsDrawingSurfaceQt()
{
    mPaintDevice = nsnull;
    mGC          = nsnull;
    mDepth       = -1;
    mWidth       = 0;
    mHeight      = 0;
    mFlags       = 0;
    mLockWidth   = 0;
    mLockHeight  = 0;
    mLockFlags   = 0;
    mLocked      = PR_FALSE;

    
    
    mPixFormat.mRedMask = 0;
    mPixFormat.mGreenMask = 0;
    mPixFormat.mBlueMask = 0;
    mPixFormat.mAlphaMask = 0;

    mPixFormat.mRedShift = 0;
    mPixFormat.mGreenShift = 0;
    mPixFormat.mBlueShift = 0;
    mPixFormat.mAlphaShift = 0;
}

nsDrawingSurfaceQt::~nsDrawingSurfaceQt()
{
    if (mGC && mGC->isActive()) {
        mGC->end();
    }

    delete mGC;
    mGC = nsnull;

    if (mPaintDevice) {
        if (mIsOffscreen && !mPaintDevice->paintingActive() && mPaintDevice != &mPixmap)
            delete mPaintDevice;
        mPaintDevice = nsnull;
    }
}

NS_IMETHODIMP nsDrawingSurfaceQt::Lock(PRInt32 aX,PRInt32 aY,
                                       PRUint32 aWidth,PRUint32 aHeight,
                                       void **aBits,PRInt32 *aStride,
                                       PRInt32 *aWidthBytes,PRUint32 aFlags)
{
    if (mLocked) {
        NS_ASSERTION(0, "nested lock attempt");
        return NS_ERROR_FAILURE;
    }
    if (mPixmap.isNull()) {
        NS_ASSERTION(0, "NULL pixmap in lock attempt");
        return NS_ERROR_FAILURE;
    }
    mLocked     = PR_TRUE;
    mLockX      = aX;
    mLockY      = aY;
    mLockWidth  = aWidth;
    mLockHeight = aHeight;
    mLockFlags  = aFlags;

    if (mImage.isNull())
        mImage = mPixmap.convertToImage();

    *aBits = mImage.bits();
    *aStride = mImage.bytesPerLine();
    *aWidthBytes = mImage.bytesPerLine();

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQt::Unlock(void)
{
    if (!mLocked) {
        NS_ASSERTION(0,"attempting to unlock an DrawingSurface that isn't locked");
        return NS_ERROR_FAILURE;
    }
    if (mPixmap.isNull()) {
        NS_ASSERTION(0, "NULL pixmap in unlock attempt");
        return NS_ERROR_FAILURE;
    }
    if (!(mLockFlags & NS_LOCK_SURFACE_READ_ONLY)) {
        qDebug("nsDrawingSurfaceQt::Unlock: w/h=%d/%d", mPixmap.width(), mPixmap.height());
        mGC->drawPixmap(0, 0, mPixmap, mLockY, mLockY, mLockWidth, mLockHeight);
    }
    mLocked = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQt::GetDimensions(PRUint32 *aWidth,
                                                PRUint32 *aHeight)
{
    *aWidth = mWidth;
    *aHeight = mHeight;

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQt::IsOffscreen(PRBool *aOffScreen)
{
    *aOffScreen = mIsOffscreen;
    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQt::IsPixelAddressable(PRBool *aAddressable)
{
    *aAddressable = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQt::GetPixelFormat(nsPixelFormat *aFormat)
{
    *aFormat = mPixFormat;

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQt::Init(QPaintDevice *aPaintDevice,
                                       QPainter *aGC)
{
    PR_LOG(gQtLogModule, QT_BASIC, ("[%p] nsDrawingSurface::Init\n", this));
    NS_ASSERTION(aPaintDevice, "need paint dev.");
    QPaintDeviceMetrics qMetrics(aPaintDevice);
    mGC = aGC;

    mPaintDevice = aPaintDevice;

    mWidth = qMetrics.width();
    mHeight = qMetrics.height();
    mDepth = qMetrics.depth();

    mIsOffscreen = PR_FALSE;

    mImage.reset();

    return CommonInit();
}

NS_IMETHODIMP nsDrawingSurfaceQt::Init(QPainter *aGC,
                                       PRUint32 aWidth,
                                       PRUint32 aHeight,
                                       PRUint32 aFlags)
{
    PR_LOG(gQtLogModule, QT_BASIC, ("[%p] nsDrawingSurface::Init\n", this));
    if (nsnull == aGC || aWidth <= 0 || aHeight <= 0) {
        return NS_ERROR_FAILURE;
    }
    mGC     = aGC;
    mWidth  = aWidth;
    mHeight = aHeight;
    mFlags  = aFlags;

    mPixmap = QPixmap(mWidth, mHeight, mDepth);
    mPaintDevice = &mPixmap;
    NS_ASSERTION(mPaintDevice, "this better not fail");

    mIsOffscreen = PR_TRUE;

    mImage.reset();

    return CommonInit();
}

NS_IMETHODIMP nsDrawingSurfaceQt::CommonInit()
{
    PR_LOG(gQtLogModule, QT_BASIC, ("[%p] nsDrawingSurface::CommonInit\n", this));
    if (nsnull  == mGC || nsnull == mPaintDevice) {
        return NS_ERROR_FAILURE;
    }
    if (mGC->isActive()) {
        mGC->end();
    }
    mGC->begin(mPaintDevice);
    return NS_OK;
}

QPainter* nsDrawingSurfaceQt::GetGC()
{
    return mGC;
}

QPaintDevice* nsDrawingSurfaceQt::GetPaintDevice()
{
    PR_LOG(gQtLogModule, QT_BASIC, ("[%p] nsDrawingSurfaceQt::GetPaintDevice\n", this));
    NS_ASSERTION(mPaintDevice, "No paint device! Something will probably crash soon.");
    return mPaintDevice;
}









































#ifndef nsDrawingSurfaceQt_h___
#define nsDrawingSurfaceQt_h___

#include "nsIDrawingSurface.h"

#include <qpainter.h>
#include <qimage.h>

class QPixmap;

class nsDrawingSurfaceQt : public nsIDrawingSurface
{
public:
    nsDrawingSurfaceQt();
    virtual ~nsDrawingSurfaceQt();

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD Lock(PRInt32 aX,PRInt32 aY,
                    PRUint32 aWidth,PRUint32 aHeight,
                    void **aBits,PRInt32 *aStride,
                    PRInt32 *aWidthBytes,PRUint32 aFlags);
    NS_IMETHOD Unlock(void);
    NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
    NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
    NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
    NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

    







    NS_IMETHOD Init(QPaintDevice * aPaintDevice, QPainter *aGC);

    












    NS_IMETHOD Init(QPainter *aGC,PRUint32 aWidth,PRUint32 aHeight,
                    PRUint32 aFlags);

    
    PRInt32 GetDepth() { return mDepth; }
    
    
    virtual QPainter *GetGC(void);
    virtual QPaintDevice  *GetPaintDevice(void);

protected:
    NS_IMETHOD CommonInit();

private:
    
    QPaintDevice  *mPaintDevice;
    QPixmap       mPixmap;
    QPainter      *mGC;
    int           mDepth;
    nsPixelFormat mPixFormat;
    PRUint32      mWidth;
    PRUint32      mHeight;
    PRUint32      mFlags;
    PRBool        mIsOffscreen;

    
    QImage        mImage;
    PRInt32       mLockX;
    PRInt32       mLockY;
    PRUint32      mLockWidth;
    PRUint32      mLockHeight;
    PRUint32      mLockFlags;
    PRBool        mLocked;
};

#endif

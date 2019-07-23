





































#ifndef _NSTHEBESDRAWINGSURFACE_H_
#define _NSTHEBESDRAWINGSURFACE_H_

#include "nsCOMPtr.h"

#include "nsSize.h"
#include "nsRect.h"
#include "nsIDrawingSurface.h"
#include "nsIWidget.h"

#include "gfxASurface.h"

class nsThebesDeviceContext;

class nsThebesDrawingSurface : public nsIDrawingSurface
{
public:
    nsThebesDrawingSurface ();
    virtual ~nsThebesDrawingSurface ();

    
    
    nsresult Init (nsThebesDeviceContext *aDC, PRUint32 aWidth, PRUint32 aHeight, PRBool aFastAccess);

    
    nsresult Init (nsThebesDeviceContext *aDC, gfxASurface *aSurface);

    
    nsresult Init (nsThebesDeviceContext *aDC, nsIWidget *aWidget);

    
    nsresult Init (nsThebesDeviceContext *aDC, nsNativeWidget aWidget);

    
    nsresult Init (nsThebesDeviceContext* aDC, void* aNativePixmapBlack,
                   void* aNativePixmapWhite,
                   const nsIntSize& aSize);

    
    NS_DECL_ISUPPORTS

    

    NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                    void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                    PRUint32 aFlags);
    NS_IMETHOD Unlock(void);
    NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
    NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
    NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
    NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

    
    gfxASurface *GetThebesSurface(void) { return mSurface; }
    PRInt32 GetDepth() {  return 24; }

    nsresult PushFilter(const nsIntRect& aRect, PRBool aAreaIsOpaque, float aOpacity);
    void PopFilter();
private:
    nsRefPtr<gfxASurface> mSurface;
    nsThebesDeviceContext *mDC;

    PRUint32 mWidth, mHeight;
};

#endif 

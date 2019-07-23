





































#ifndef _NS_THEBESBLENDER_H_
#define _NS_THEBESBLENDER_H_

#include "nsIBlender.h"
#include "nsThebesDeviceContext.h"

class nsThebesBlender : public nsIBlender
{
public:
    nsThebesBlender();
    ~nsThebesBlender();

    
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD Init(nsIDeviceContext *aContext);

    NS_IMETHOD Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight,nsIDrawingSurface* aSrc,
                     nsIDrawingSurface* aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                     nsIDrawingSurface* aSecondSrc = nsnull, nscolor aSrcBackColor = NS_RGB(0, 0, 0),
                     nscolor aSecondSrcBackColor = NS_RGB(0, 0, 0));

    NS_IMETHOD Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight, nsIRenderingContext *aSrc,
                     nsIRenderingContext *aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                     nsIRenderingContext *aSecondSrc = nsnull, nscolor aSrcBackColor = NS_RGB(0, 0, 0),
                     nscolor aSecondSrcBackColor = NS_RGB(0, 0, 0));

    NS_IMETHOD GetAlphas(const nsRect& aRect, nsIDrawingSurface* aBlack,
                         nsIDrawingSurface* aWhite, PRUint8** aAlphas);
private:
    nsThebesDeviceContext *mThebesDC;
};

#endif 

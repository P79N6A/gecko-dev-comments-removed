





































#include "nsMemory.h"

#include "nsThebesBlender.h"
#include "nsThebesDrawingSurface.h"

NS_IMPL_ISUPPORTS1(nsThebesBlender, nsIBlender)

nsThebesBlender::nsThebesBlender()
    : mThebesDC(nsnull)
{
}

nsThebesBlender::~nsThebesBlender()
{
}

NS_IMETHODIMP
nsThebesBlender::Init(nsIDeviceContext *aContext)
{
    mThebesDC = NS_STATIC_CAST(nsThebesDeviceContext*, aContext);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesBlender::Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight,
                       nsIDrawingSurface* aSrc, nsIDrawingSurface* aDest,
                       PRInt32 aDX, PRInt32 aDY,
                       float aSrcOpacity,
                       nsIDrawingSurface* aSecondSrc,
                       nscolor aSrcBackColor, nscolor aSecondSrcBackColor)
{
    NS_WARNING("Should be using Push/PopFilter instead");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsThebesBlender::Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight,
                       nsIRenderingContext *aSrc, nsIRenderingContext *aDest,
                       PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                       nsIRenderingContext *aSecondSrc, nscolor aSrcBackColor,
                       nscolor aSecondSrcBackColor)
{
    NS_WARNING("Should be using Push/PopFilter instead");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsThebesBlender::GetAlphas(const nsRect& aRect, nsIDrawingSurface* aBlack,
                           nsIDrawingSurface* aWhite, PRUint8** aAlphas)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}






































#include "nsScreenMac.h"
#include <MacWindows.h>

nsScreenMac :: nsScreenMac ( GDHandle inScreen )
  : mScreen(inScreen)
{
  NS_ASSERTION ( inScreen, "Passing null device to nsScreenMac" );
  
  
  
  
}


nsScreenMac :: ~nsScreenMac()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenMac, nsIScreen)


NS_IMETHODIMP
nsScreenMac :: GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  *outLeft = (**mScreen).gdRect.left;
  *outTop = (**mScreen).gdRect.top;  
  *outWidth = (**mScreen).gdRect.right - (**mScreen).gdRect.left;
  *outHeight = (**mScreen).gdRect.bottom - (**mScreen).gdRect.top;

  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenMac :: GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  Rect adjustedRect;
  
  ::GetAvailableWindowPositioningBounds ( mScreen, &adjustedRect );

  *outLeft = adjustedRect.left;
  *outTop = adjustedRect.top;  
  *outWidth = adjustedRect.right - adjustedRect.left;
  *outHeight = adjustedRect.bottom - adjustedRect.top;

  return NS_OK;
  
} 



NS_IMETHODIMP 
nsScreenMac :: GetPixelDepth(PRInt32 *aPixelDepth)
{
  *aPixelDepth = (**(**mScreen).gdPMap).pixelSize;
  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenMac :: GetColorDepth(PRInt32 *aColorDepth)
{
  *aColorDepth = (**(**mScreen).gdPMap).pixelSize;
  return NS_OK;

} 









































#include "nsScreenXlib.h"
#include "xlibrgb.h"
#include <stdlib.h>

nsScreenXlib :: nsScreenXlib (  )
{
  
  
  
}


nsScreenXlib :: ~nsScreenXlib()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenXlib, nsIScreen)


NS_IMETHODIMP
nsScreenXlib :: GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  XlibRgbHandle *xrh;
  if (!(xrh = xxlib_find_handle(XXLIBRGB_DEFAULT_HANDLE)))
    abort();
    
  *outTop = 0;
  *outLeft = 0;
  *outWidth = XDisplayWidth(xxlib_rgb_get_display(xrh), 0);
  *outHeight = XDisplayHeight(xxlib_rgb_get_display(xrh), 0);

  return NS_OK;
  
} 


NS_IMETHODIMP
nsScreenXlib :: GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  XlibRgbHandle *xrh;
  if (!(xrh = xxlib_find_handle(XXLIBRGB_DEFAULT_HANDLE)))
    abort();
    
  *outTop = 0;
  *outLeft = 0;
  *outWidth  = XDisplayWidth(xxlib_rgb_get_display(xrh), 0);
  *outHeight = XDisplayHeight(xxlib_rgb_get_display(xrh), 0);

  return NS_OK;
  
} 


NS_IMETHODIMP 
nsScreenXlib :: GetPixelDepth(PRInt32 *aPixelDepth)
{
  XlibRgbHandle *xrh;
  if (!(xrh = xxlib_find_handle(XXLIBRGB_DEFAULT_HANDLE)))
    abort();
  
  *aPixelDepth = xxlib_rgb_get_depth(xrh);

  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenXlib :: GetColorDepth(PRInt32 *aColorDepth)
{
  return GetPixelDepth ( aColorDepth );

} 



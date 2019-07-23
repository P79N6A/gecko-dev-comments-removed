




































#include "nsScreenBeOS.h"

#include <Screen.h>

nsScreenBeOS :: nsScreenBeOS (  )
{
  
  
  
}


nsScreenBeOS :: ~nsScreenBeOS()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenBeOS, nsIScreen)


NS_IMETHODIMP
nsScreenBeOS :: GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
	BScreen screen;
	
  *outTop = 0;
  *outLeft = 0;
  *outWidth = PRInt32(screen.Frame().Width()+1);
  *outHeight = PRInt32(screen.Frame().Height()+1);

  return NS_OK;
  
} 


NS_IMETHODIMP
nsScreenBeOS :: GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
	BScreen screen;

  *outTop = 0;
  *outLeft = 0;
  *outWidth = PRInt32(screen.Frame().Width()+1);
  *outHeight = PRInt32(screen.Frame().Height()+1);

  return NS_OK;
  
} 


NS_IMETHODIMP 
nsScreenBeOS :: GetPixelDepth(PRInt32 *aPixelDepth)
{
	BScreen screen;

	color_space depth;
	PRInt32 pixelDepth;
	
	depth = screen.ColorSpace();
	switch(depth)
	{	
		case B_CMAP8:
			pixelDepth = 8;
			break;
		case B_RGB32:
			pixelDepth = 32;
			break;
		case B_RGB15:
			pixelDepth = 15;
			break;
		case B_RGB16:
			pixelDepth = 16;
			break;
		default:
			NS_NOTREACHED("FIXME:  Please add this screen depth to the code nsScreenBeOS.cpp");
			pixelDepth = 32;
			break;	
	}
  *aPixelDepth = pixelDepth;

  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenBeOS :: GetColorDepth(PRInt32 *aColorDepth)
{
  return GetPixelDepth ( aColorDepth );

} 



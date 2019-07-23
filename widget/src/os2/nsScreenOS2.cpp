




































#include "nsScreenOS2.h"

nsScreenOS2 :: nsScreenOS2 (  )
{
  
  
  
}


nsScreenOS2 :: ~nsScreenOS2()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenOS2, nsIScreen)


NS_IMETHODIMP
nsScreenOS2 :: GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  LONG alArray[2];

  HPS hps = ::WinGetScreenPS( HWND_DESKTOP);
  HDC hdc = ::GpiQueryDevice (hps);
  ::DevQueryCaps(hdc, CAPS_WIDTH, 2, alArray);
  ::WinReleasePS(hps);

  *outTop = 0;
  *outLeft = 0;
  *outWidth = alArray[0];
  *outHeight = alArray[1];


  return NS_OK;
  
} 


NS_IMETHODIMP
nsScreenOS2 :: GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  static APIRET rc = 0;
  static BOOL (APIENTRY * pfnQueryDesktopWorkArea)(HWND hwndDesktop, PWRECT pwrcWorkArea) = NULL;

  GetRect(outLeft, outTop, outWidth, outHeight);

  
  
  LONG lWorkAreaHeight = *outHeight;

  if ( !rc && !pfnQueryDesktopWorkArea )
  {
      HMODULE hmod = 0;
      rc = DosLoadModule( NULL, 0, "PMMERGE", &hmod );
      if ( !rc )
      {
          rc = DosQueryProcAddr( hmod, 5469, NULL, (PFN*) &pfnQueryDesktopWorkArea ); 
          DosFreeModule(hmod);
      }
  }
  if ( pfnQueryDesktopWorkArea && !rc )
  {
      RECTL rectl;
      pfnQueryDesktopWorkArea( HWND_DESKTOP, &rectl );
      lWorkAreaHeight = rectl.yTop - rectl.yBottom;
  }

  HWND hwndWarpCenter = WinWindowFromID( HWND_DESKTOP, 0x555 );
  if (hwndWarpCenter) {
    
    if (lWorkAreaHeight != *outHeight) {
      SWP swp;
      WinQueryWindowPos( hwndWarpCenter, &swp );
      if (swp.y != 0) {
         
         *outTop += swp.cy;
      }
      *outHeight -= swp.cy;
    }
  }

  return NS_OK;
  
} 


NS_IMETHODIMP 
nsScreenOS2 :: GetPixelDepth(PRInt32 *aPixelDepth)
{
  LONG lCap;

  HPS hps = ::WinGetScreenPS( HWND_DESKTOP);
  HDC hdc = ::GpiQueryDevice (hps);

  ::DevQueryCaps(hdc, CAPS_COLOR_BITCOUNT, 1, &lCap);

  *aPixelDepth = (PRInt32)lCap;

  
  ::WinReleasePS(hps);

  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenOS2 :: GetColorDepth(PRInt32 *aColorDepth)
{
  return GetPixelDepth ( aColorDepth );

} 



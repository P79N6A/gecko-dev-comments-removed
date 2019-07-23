









































#undef WINVER
#define WINVER 0x0500
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500

#include "nsScreenWin.h"




nsScreenWin :: nsScreenWin ( void* inScreen )
  : mScreen(inScreen)
{
#ifdef DEBUG
  HDC hDCScreen = ::GetDC(nsnull);
  NS_ASSERTION(hDCScreen,"GetDC Failure");
  NS_ASSERTION ( ::GetDeviceCaps(hDCScreen, TECHNOLOGY) == DT_RASDISPLAY, "Not a display screen");
  ::ReleaseDC(nsnull,hDCScreen);
#endif

  
  
  
}


nsScreenWin :: ~nsScreenWin()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenWin, nsIScreen)


NS_IMETHODIMP
nsScreenWin :: GetRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  BOOL success = FALSE;
#if _MSC_VER >= 1200
  if ( mScreen ) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    success = ::GetMonitorInfoW( (HMONITOR)mScreen, &info );
    if ( success ) {
      *outLeft = info.rcMonitor.left;
      *outTop = info.rcMonitor.top;
      *outWidth = info.rcMonitor.right - info.rcMonitor.left;
      *outHeight = info.rcMonitor.bottom - info.rcMonitor.top;
    }
  }
#endif
  if (!success) {
     HDC hDCScreen = ::GetDC(nsnull);
     NS_ASSERTION(hDCScreen,"GetDC Failure");
    
     *outTop = *outLeft = 0;
     *outWidth = ::GetDeviceCaps(hDCScreen, HORZRES);
     *outHeight = ::GetDeviceCaps(hDCScreen, VERTRES); 
     
     ::ReleaseDC(nsnull, hDCScreen);
  }
  return NS_OK;

} 


NS_IMETHODIMP
nsScreenWin :: GetAvailRect(PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight)
{
  BOOL success = FALSE;
#if _MSC_VER >= 1200
  if ( mScreen ) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    success = ::GetMonitorInfoW( (HMONITOR)mScreen, &info );
    if ( success ) {
      *outLeft = info.rcWork.left;
      *outTop = info.rcWork.top;
      *outWidth = info.rcWork.right - info.rcWork.left;
      *outHeight = info.rcWork.bottom - info.rcWork.top;
    }
  }
#endif
  if (!success) {
    RECT workArea;
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    *outLeft = workArea.left;
    *outTop = workArea.top;
    *outWidth = workArea.right - workArea.left;
    *outHeight = workArea.bottom - workArea.top;
  }

  return NS_OK;
  
} 



NS_IMETHODIMP 
nsScreenWin :: GetPixelDepth(PRInt32 *aPixelDepth)
{
  
  HDC hDCScreen = ::GetDC(nsnull);
  NS_ASSERTION(hDCScreen,"GetDC Failure");

  *aPixelDepth = ::GetDeviceCaps(hDCScreen, BITSPIXEL);

  ::ReleaseDC(nsnull, hDCScreen);
  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenWin :: GetColorDepth(PRInt32 *aColorDepth)
{
  return GetPixelDepth(aColorDepth);

} 



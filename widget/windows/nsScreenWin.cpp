




#include "nsScreenWin.h"
#include "nsCoord.h"


nsScreenWin :: nsScreenWin ( HMONITOR inScreen )
  : mScreen(inScreen)
{
#ifdef DEBUG
  HDC hDCScreen = ::GetDC(nullptr);
  NS_ASSERTION(hDCScreen,"GetDC Failure");
  NS_ASSERTION ( ::GetDeviceCaps(hDCScreen, TECHNOLOGY) == DT_RASDISPLAY, "Not a display screen");
  ::ReleaseDC(nullptr,hDCScreen);
#endif

  
  
  
}


nsScreenWin :: ~nsScreenWin()
{
  
}


NS_IMETHODIMP
nsScreenWin :: GetRect(int32_t *outLeft, int32_t *outTop, int32_t *outWidth, int32_t *outHeight)
{
  BOOL success = FALSE;
  if ( mScreen ) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    success = ::GetMonitorInfoW( mScreen, &info );
    if ( success ) {
      *outLeft = info.rcMonitor.left;
      *outTop = info.rcMonitor.top;
      *outWidth = info.rcMonitor.right - info.rcMonitor.left;
      *outHeight = info.rcMonitor.bottom - info.rcMonitor.top;
    }
  }
  if (!success) {
     HDC hDCScreen = ::GetDC(nullptr);
     NS_ASSERTION(hDCScreen,"GetDC Failure");
    
     *outTop = *outLeft = 0;
     *outWidth = ::GetDeviceCaps(hDCScreen, HORZRES);
     *outHeight = ::GetDeviceCaps(hDCScreen, VERTRES); 
     
     ::ReleaseDC(nullptr, hDCScreen);
  }
  return NS_OK;

} 


NS_IMETHODIMP
nsScreenWin :: GetAvailRect(int32_t *outLeft, int32_t *outTop, int32_t *outWidth, int32_t *outHeight)
{
  BOOL success = FALSE;

  if ( mScreen ) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    success = ::GetMonitorInfoW( mScreen, &info );
    if ( success ) {
      *outLeft = info.rcWork.left;
      *outTop = info.rcWork.top;
      *outWidth = info.rcWork.right - info.rcWork.left;
      *outHeight = info.rcWork.bottom - info.rcWork.top;
    }
  }
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
nsScreenWin::GetRectDisplayPix(int32_t *outLeft,  int32_t *outTop,
                               int32_t *outWidth, int32_t *outHeight)
{
  int32_t left, top, width, height;
  nsresult rv = GetRect(&left, &top, &width, &height);
  if (NS_FAILED(rv)) {
    return rv;
  }
  HDC dc = ::GetDC(nullptr);
  double scaleFactor = 96.0 / GetDeviceCaps(dc, LOGPIXELSY);
  ::ReleaseDC(nullptr, dc);
  *outLeft = NSToIntRound(left * scaleFactor);
  *outTop = NSToIntRound(top * scaleFactor);
  *outWidth = NSToIntRound(width * scaleFactor);
  *outHeight = NSToIntRound(height * scaleFactor);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenWin::GetAvailRectDisplayPix(int32_t *outLeft,  int32_t *outTop,
                                    int32_t *outWidth, int32_t *outHeight)
{
  int32_t left, top, width, height;
  nsresult rv = GetAvailRect(&left, &top, &width, &height);
  if (NS_FAILED(rv)) {
    return rv;
  }
  HDC dc = ::GetDC(nullptr);
  double scaleFactor = 96.0 / GetDeviceCaps(dc, LOGPIXELSY);
  ::ReleaseDC(nullptr, dc);
  *outLeft = NSToIntRound(left * scaleFactor);
  *outTop = NSToIntRound(top * scaleFactor);
  *outWidth = NSToIntRound(width * scaleFactor);
  *outHeight = NSToIntRound(height * scaleFactor);
  return NS_OK;
}


NS_IMETHODIMP 
nsScreenWin :: GetPixelDepth(int32_t *aPixelDepth)
{
  
  HDC hDCScreen = ::GetDC(nullptr);
  NS_ASSERTION(hDCScreen,"GetDC Failure");

  int32_t depth = ::GetDeviceCaps(hDCScreen, BITSPIXEL);
  if (depth == 32) {
    
    
    
    depth = 24;
  }
  *aPixelDepth = depth;

  ::ReleaseDC(nullptr, hDCScreen);
  return NS_OK;

} 


NS_IMETHODIMP 
nsScreenWin :: GetColorDepth(int32_t *aColorDepth)
{
  return GetPixelDepth(aColorDepth);

} 



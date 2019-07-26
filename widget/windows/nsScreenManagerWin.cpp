




#include "nsScreenManagerWin.h"
#include "nsScreenWin.h"
#include "gfxWindowsPlatform.h"


BOOL CALLBACK CountMonitors ( HMONITOR, HDC, LPRECT, LPARAM ioCount ) ;

nsScreenManagerWin :: nsScreenManagerWin ( )
  : mNumberOfScreens(0)
{
  
  
  
}


nsScreenManagerWin :: ~nsScreenManagerWin()
{
}



NS_IMPL_ISUPPORTS1(nsScreenManagerWin, nsIScreenManager)










nsIScreen* 
nsScreenManagerWin :: CreateNewScreenObject ( HMONITOR inScreen )
{
  nsIScreen* retScreen = nullptr;
  
  
  
  for ( unsigned i = 0; i < mScreenList.Length(); ++i ) {
    ScreenListItem& curr = mScreenList[i];
    if ( inScreen == curr.mMon ) {
      NS_IF_ADDREF(retScreen = curr.mScreen.get());
      return retScreen;
    }
  } 
 
  retScreen = new nsScreenWin(inScreen);
  mScreenList.AppendElement ( ScreenListItem ( inScreen, retScreen ) );

  NS_IF_ADDREF(retScreen);
  return retScreen;
}










NS_IMETHODIMP
nsScreenManagerWin :: ScreenForRect ( int32_t inLeft, int32_t inTop, int32_t inWidth, int32_t inHeight,
                                        nsIScreen **outScreen )
{
  if ( !(inWidth || inHeight) ) {
    NS_WARNING ( "trying to find screen for sizeless window, using primary monitor" );
    *outScreen = CreateNewScreenObject ( nullptr );    
    return NS_OK;
  }

  
  FLOAT dpiScale = gfxWindowsPlatform::GetPlatform()->GetDPIScale();
  RECT globalWindowBounds = {
    NSToIntRound(dpiScale * inLeft),
    NSToIntRound(dpiScale * inTop),
    NSToIntRound(dpiScale * (inLeft + inWidth)),
    NSToIntRound(dpiScale * (inTop + inHeight))
  };

  HMONITOR genScreen = ::MonitorFromRect( &globalWindowBounds, MONITOR_DEFAULTTOPRIMARY );

  *outScreen = CreateNewScreenObject ( genScreen );    
  
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerWin :: GetPrimaryScreen(nsIScreen** aPrimaryScreen) 
{
  *aPrimaryScreen = CreateNewScreenObject ( nullptr );    
  return NS_OK;
  
} 









BOOL CALLBACK
CountMonitors ( HMONITOR, HDC, LPRECT, LPARAM ioParam )
{
  uint32_t* countPtr = reinterpret_cast<uint32_t*>(ioParam);
  ++(*countPtr);

  return TRUE; 

} 







NS_IMETHODIMP
nsScreenManagerWin :: GetNumberOfScreens(uint32_t *aNumberOfScreens)
{
  if ( mNumberOfScreens )
    *aNumberOfScreens = mNumberOfScreens;
  else {
    uint32_t count = 0;
    BOOL result = ::EnumDisplayMonitors(nullptr, nullptr, (MONITORENUMPROC)CountMonitors, (LPARAM)&count);
    if (!result)
      return NS_ERROR_FAILURE;
    *aNumberOfScreens = mNumberOfScreens = count;
  }

  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerWin :: ScreenForNativeWidget(void *aWidget, nsIScreen **outScreen)
{
  HMONITOR mon = MonitorFromWindow ((HWND) aWidget, MONITOR_DEFAULTTOPRIMARY);
  *outScreen = CreateNewScreenObject (mon);
  return NS_OK;
}

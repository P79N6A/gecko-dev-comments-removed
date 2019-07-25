




#include "nsScreenManagerWin.h"
#include "nsScreenWin.h"


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
  nsIScreen* retScreen = nsnull;
  
  
  
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
nsScreenManagerWin :: ScreenForRect ( PRInt32 inLeft, PRInt32 inTop, PRInt32 inWidth, PRInt32 inHeight,
                                        nsIScreen **outScreen )
{
  if ( !(inWidth || inHeight) ) {
    NS_WARNING ( "trying to find screen for sizeless window, using primary monitor" );
    *outScreen = CreateNewScreenObject ( nsnull );    
    return NS_OK;
  }

  RECT globalWindowBounds = { inLeft, inTop, inLeft + inWidth, inTop + inHeight };

  HMONITOR genScreen = ::MonitorFromRect( &globalWindowBounds, MONITOR_DEFAULTTOPRIMARY );

  *outScreen = CreateNewScreenObject ( genScreen );    
  
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerWin :: GetPrimaryScreen(nsIScreen** aPrimaryScreen) 
{
  *aPrimaryScreen = CreateNewScreenObject ( nsnull );    
  return NS_OK;
  
} 









BOOL CALLBACK
CountMonitors ( HMONITOR, HDC, LPRECT, LPARAM ioParam )
{
  PRUint32* countPtr = reinterpret_cast<PRUint32*>(ioParam);
  ++(*countPtr);

  return TRUE; 

} 







NS_IMETHODIMP
nsScreenManagerWin :: GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
  if ( mNumberOfScreens )
    *aNumberOfScreens = mNumberOfScreens;
  else {
    PRUint32 count = 0;
    BOOL result = ::EnumDisplayMonitors(nsnull, nsnull, (MONITORENUMPROC)CountMonitors, (LPARAM)&count);
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











































#undef WINVER
#define WINVER 0x0500
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500

#include "nsScreenManagerWin.h"
#include "nsScreenWin.h"


BOOL CALLBACK CountMonitors ( HMONITOR, HDC, LPRECT, LPARAM ioCount ) ;


class ScreenListItem
{
public:
  ScreenListItem ( HMONITOR inMon, nsIScreen* inScreen )
    : mMon(inMon), mScreen(inScreen) { } ;
  
  HMONITOR mMon;
  nsCOMPtr<nsIScreen> mScreen;
};


nsScreenManagerWin :: nsScreenManagerWin ( )
  : mNumberOfScreens(0)
{
  
  
  
}


nsScreenManagerWin :: ~nsScreenManagerWin()
{
  
  for ( int i = 0; i < mScreenList.Count(); ++i ) {
    ScreenListItem* item = NS_REINTERPRET_CAST(ScreenListItem*, mScreenList[i]);
    delete item;
  }
}



NS_IMPL_ISUPPORTS1(nsScreenManagerWin, nsIScreenManager)










nsIScreen* 
nsScreenManagerWin :: CreateNewScreenObject ( void* inScreen )
{
  nsIScreen* retScreen = nsnull;
  
  
  
  for ( int i = 0; i < mScreenList.Count(); ++i ) {
    ScreenListItem* curr = NS_REINTERPRET_CAST(ScreenListItem*, mScreenList[i]);
    if ( inScreen == curr->mMon ) {
      NS_IF_ADDREF(retScreen = curr->mScreen.get());
      return retScreen;
    }
  } 
 
  retScreen = new nsScreenWin(inScreen);
  ScreenListItem* listItem = new ScreenListItem ( (HMONITOR)inScreen, retScreen );
  mScreenList.AppendElement ( listItem );

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

  void* genScreen = ::MonitorFromRect( &globalWindowBounds, MONITOR_DEFAULTTOPRIMARY );

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
  PRUint32* countPtr = NS_REINTERPRET_CAST(PRUint32*, ioParam);
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

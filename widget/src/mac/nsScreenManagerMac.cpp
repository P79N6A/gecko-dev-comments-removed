




































#include "nsScreenManagerMac.h"
#include "nsScreenMac.h"
#include "nsCOMPtr.h"


class ScreenListItem
{
public:
  ScreenListItem ( GDHandle inGD, nsIScreen* inScreen )
    : mGD(inGD), mScreen(inScreen) { } ;
  
  GDHandle mGD;
  nsCOMPtr<nsIScreen> mScreen;
};


nsScreenManagerMac :: nsScreenManagerMac ( )
{
  
  
  
}


nsScreenManagerMac :: ~nsScreenManagerMac()
{
  
  for ( int i = 0; i < mScreenList.Count(); ++i ) {
    ScreenListItem* item = NS_REINTERPRET_CAST(ScreenListItem*, mScreenList[i]);
    delete item;
  }
}



NS_IMPL_ISUPPORTS1(nsScreenManagerMac, nsIScreenManager)







nsIScreen* 
nsScreenManagerMac :: CreateNewScreenObject ( GDHandle inDevice )
{
  nsIScreen* retScreen = nsnull;
  
  
  
  for ( int i = 0; i < mScreenList.Count(); ++i ) {
    ScreenListItem* curr = NS_REINTERPRET_CAST(ScreenListItem*, mScreenList[i]);
    if ( inDevice == curr->mGD ) {
      NS_IF_ADDREF(retScreen = curr->mScreen.get());
      return retScreen;
    }
  } 
  
  
  retScreen = new nsScreenMac ( inDevice );
  ScreenListItem* listItem = new ScreenListItem ( inDevice, retScreen );
  mScreenList.AppendElement ( listItem );
  
  NS_IF_ADDREF(retScreen);
  return retScreen;
}










NS_IMETHODIMP
nsScreenManagerMac :: ScreenForRect ( PRInt32 inLeft, PRInt32 inTop, PRInt32 inWidth, PRInt32 inHeight,
                                        nsIScreen **outScreen )
{
  if ( !(inWidth || inHeight) ) {
    NS_WARNING ( "trying to find screen for sizeless window, using primary monitor" );
    *outScreen = CreateNewScreenObject ( ::GetMainDevice() );    
    return NS_OK;
  }

  Rect globalWindowBounds = { inTop, inLeft, inTop + inHeight, inLeft + inWidth };

  GDHandle currDevice = ::GetDeviceList();
  GDHandle deviceWindowIsOn = ::GetMainDevice();
  PRInt32 greatestArea = 0;
  while ( currDevice ) {
    if ( ::TestDeviceAttribute(currDevice, screenDevice) && ::TestDeviceAttribute(currDevice, screenActive) ) {
      
      Rect intersection;
      Rect devRect = (**currDevice).gdRect;
      ::SectRect ( &globalWindowBounds, &devRect, &intersection );
      PRInt32 intersectArea = (intersection.right - intersection.left) * 
                                  (intersection.bottom - intersection.top);
      if ( intersectArea > greatestArea ) {
        greatestArea = intersectArea;
        deviceWindowIsOn = currDevice;
     }      
    } 
    currDevice = ::GetNextDevice(currDevice);
  } 

  *outScreen = CreateNewScreenObject ( deviceWindowIsOn );    
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerMac :: GetPrimaryScreen(nsIScreen * *aPrimaryScreen) 
{
  *aPrimaryScreen = CreateNewScreenObject ( ::GetMainDevice() );    
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerMac :: GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
  *aNumberOfScreens = 0;
  GDHandle currDevice = ::GetDeviceList();
  while ( currDevice ) {
    if ( ::TestDeviceAttribute(currDevice, screenDevice) && ::TestDeviceAttribute(currDevice, screenActive) )
      ++(*aNumberOfScreens);
    currDevice = ::GetNextDevice(currDevice);
  }
  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerMac :: ScreenForNativeWidget(void *nativeWidget, nsIScreen **aScreen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

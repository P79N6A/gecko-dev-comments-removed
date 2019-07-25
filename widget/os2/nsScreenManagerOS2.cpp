




#include "nsScreenManagerOS2.h"
#include "nsScreenOS2.h"


nsScreenManagerOS2 :: nsScreenManagerOS2 ( )
{
  
  
  
}


nsScreenManagerOS2 :: ~nsScreenManagerOS2()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenManagerOS2, nsIScreenManager)










nsIScreen* 
nsScreenManagerOS2 :: CreateNewScreenObject (  )
{
  nsIScreen* retval = nullptr;
  if ( !mCachedMainScreen )
    mCachedMainScreen = new nsScreenOS2 ( );
  NS_IF_ADDREF(retval = mCachedMainScreen.get());
  
  return retval;
}










NS_IMETHODIMP
nsScreenManagerOS2 :: ScreenForRect ( int32_t , int32_t , int32_t ,
                                       int32_t , nsIScreen **outScreen )
{
  GetPrimaryScreen ( outScreen );
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerOS2 :: GetPrimaryScreen(nsIScreen * *aPrimaryScreen) 
{
  *aPrimaryScreen = CreateNewScreenObject();    
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerOS2 :: GetNumberOfScreens(uint32_t *aNumberOfScreens)
{
  *aNumberOfScreens = 1;
  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerOS2 :: ScreenForNativeWidget(void *nativeWidget, nsIScreen **aScreen)
{
  *aScreen = CreateNewScreenObject();    
  return NS_OK;
}

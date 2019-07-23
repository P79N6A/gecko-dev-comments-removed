




































#include "nsScreenManagerBeOS.h"
#include "nsScreenBeOS.h"


nsScreenManagerBeOS :: nsScreenManagerBeOS ( )
{
  
  
  
}


nsScreenManagerBeOS :: ~nsScreenManagerBeOS()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenManagerBeOS, nsIScreenManager)










nsIScreen* 
nsScreenManagerBeOS :: CreateNewScreenObject (  )
{
  nsIScreen* retval = nsnull;
  if ( !mCachedMainScreen )
    mCachedMainScreen = new nsScreenBeOS ( );
  NS_IF_ADDREF(retval = mCachedMainScreen.get());
  
  return retval;
}










NS_IMETHODIMP
nsScreenManagerBeOS :: ScreenForRect ( PRInt32 , PRInt32 , PRInt32 ,
                                       PRInt32 , nsIScreen **outScreen )
{
  GetPrimaryScreen ( outScreen );
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerBeOS :: GetPrimaryScreen(nsIScreen * *aPrimaryScreen) 
{
  *aPrimaryScreen = CreateNewScreenObject();    
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerBeOS :: GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
  *aNumberOfScreens = 1;
  return NS_OK;
  
} 

NS_IMETHODIMP
nsScreenManagerBeOS :: ScreenForNativeWidget(void *nativeWidget, nsIScreen **aScreen)                
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

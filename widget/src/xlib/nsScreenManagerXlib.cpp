






































#include "nsScreenManagerXlib.h"
#include "nsScreenXlib.h"


nsScreenManagerXlib :: nsScreenManagerXlib ( )
{
  
  
  
}


nsScreenManagerXlib :: ~nsScreenManagerXlib()
{
  
}



NS_IMPL_ISUPPORTS1(nsScreenManagerXlib, nsIScreenManager)










nsIScreen* 
nsScreenManagerXlib :: CreateNewScreenObject (  )
{
  nsIScreen* retval = nsnull;
  if ( !mCachedMainScreen )
    mCachedMainScreen = new nsScreenXlib ( );
  NS_IF_ADDREF(retval = mCachedMainScreen.get());
  
  return retval;
}










NS_IMETHODIMP
nsScreenManagerXlib :: ScreenForRect ( PRInt32 , PRInt32 , PRInt32 ,
                                       PRInt32 , nsIScreen **outScreen )
{
  GetPrimaryScreen ( outScreen );
  return NS_OK;
    
} 








NS_IMETHODIMP 
nsScreenManagerXlib :: GetPrimaryScreen(nsIScreen * *aPrimaryScreen) 
{
  *aPrimaryScreen = CreateNewScreenObject();    
  return NS_OK;
  
} 







NS_IMETHODIMP
nsScreenManagerXlib :: GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
  *aNumberOfScreens = 1;
  return NS_OK;
  
} 


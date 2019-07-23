




































#include "nsScreenManagerPh.h"
#include "nsScreenPh.h"

#include "nsPhGfxLog.h"

nsScreenManagerPh :: nsScreenManagerPh( ) {
  
  
  
	}


nsScreenManagerPh :: ~nsScreenManagerPh( ) {
	}



NS_IMPL_ISUPPORTS1(nsScreenManagerPh, nsIScreenManager)










nsIScreen* nsScreenManagerPh :: CreateNewScreenObject( ) {
  nsIScreen* retval = nsnull;
  if( !mCachedMainScreen ) mCachedMainScreen = new nsScreenPh( );
  NS_IF_ADDREF(retval = mCachedMainScreen.get());
  return retval;
	}









NS_IMETHODIMP nsScreenManagerPh :: ScreenForRect ( PRInt32 , PRInt32 , PRInt32 , PRInt32 , nsIScreen **outScreen ) {
  GetPrimaryScreen( outScreen );
  return NS_OK;
	}







NS_IMETHODIMP nsScreenManagerPh :: GetPrimaryScreen( nsIScreen * *aPrimaryScreen ) {
  *aPrimaryScreen = CreateNewScreenObject();    
  return NS_OK;
	}






NS_IMETHODIMP nsScreenManagerPh :: GetNumberOfScreens( PRUint32 *aNumberOfScreens ) {
  *aNumberOfScreens = 1;
  return NS_OK;
	}


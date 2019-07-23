






































#ifndef nsScreenManagerXlib_h___
#define nsScreenManagerXlib_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"



class nsScreenManagerXlib : public nsIScreenManager
{
public:
  nsScreenManagerXlib ( );
  virtual ~nsScreenManagerXlib();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsIScreen* CreateNewScreenObject ( ) ;

    
  nsCOMPtr<nsIScreen> mCachedMainScreen;

};

#endif  

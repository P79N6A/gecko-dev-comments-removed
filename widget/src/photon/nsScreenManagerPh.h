




































#ifndef nsScreenManagerPh_h___
#define nsScreenManagerPh_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"



class nsScreenManagerPh : public nsIScreenManager
{
public:
  nsScreenManagerPh ( );
  virtual ~nsScreenManagerPh();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsIScreen* CreateNewScreenObject ( ) ;

    
  nsCOMPtr<nsIScreen> mCachedMainScreen;

};

#endif  

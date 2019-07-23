




































#ifndef nsScreenManagerBeOS_h___
#define nsScreenManagerBeOS_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"



class nsScreenManagerBeOS : public nsIScreenManager
{
public:
  nsScreenManagerBeOS ( );
  virtual ~nsScreenManagerBeOS();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsIScreen* CreateNewScreenObject ( ) ;

    
  nsCOMPtr<nsIScreen> mCachedMainScreen;

};

#endif  

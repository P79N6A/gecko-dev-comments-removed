




































#ifndef nsScreenManagerOS2_h___
#define nsScreenManagerOS2_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"



class nsScreenManagerOS2 : public nsIScreenManager
{
public:
  nsScreenManagerOS2 ( );
  virtual ~nsScreenManagerOS2();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsIScreen* CreateNewScreenObject ( ) ;

    
  nsCOMPtr<nsIScreen> mCachedMainScreen;

};

#endif  

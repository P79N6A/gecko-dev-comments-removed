




































#ifndef nsScreenManagerMac_h___
#define nsScreenManagerMac_h___

#include "nsIScreenManager.h"
#include <Quickdraw.h>

#include "nsVoidArray.h"

class nsIScreen;




class nsScreenManagerMac : public nsIScreenManager
{
public:
  nsScreenManagerMac ( );
  virtual ~nsScreenManagerMac();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsIScreen* CreateNewScreenObject ( GDHandle inDevice ) ;
  
  nsAutoVoidArray mScreenList;

};


#endif  

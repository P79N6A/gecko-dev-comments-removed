




































#ifndef nsScreenManagerWin_h___
#define nsScreenManagerWin_h___

#include "nsIScreenManager.h"

#include <windows.h>
#include "nsCOMPtr.h"
#include "nsVoidArray.h"

class nsIScreen;




class nsScreenManagerWin : public nsIScreenManager
{
public:
  nsScreenManagerWin ( );
  ~nsScreenManagerWin();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsIScreen* CreateNewScreenObject ( void* inScreen ) ;

  PRUint32 mNumberOfScreens;

    
  nsAutoVoidArray mScreenList;

};

#endif  

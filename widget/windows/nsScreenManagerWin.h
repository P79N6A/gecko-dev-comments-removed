




































#ifndef nsScreenManagerWin_h___
#define nsScreenManagerWin_h___

#include "nsIScreenManager.h"

#include <windows.h>
#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsIScreen;



class ScreenListItem
{
public:
  ScreenListItem ( HMONITOR inMon, nsIScreen* inScreen )
    : mMon(inMon), mScreen(inScreen) { } ;
  
  HMONITOR mMon;
  nsCOMPtr<nsIScreen> mScreen;
};

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

    
  nsAutoTArray<ScreenListItem, 8> mScreenList;

};

#endif  

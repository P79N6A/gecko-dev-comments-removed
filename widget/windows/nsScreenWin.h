




































#ifndef nsScreenWin_h___
#define nsScreenWin_h___

#include <windows.h>
#include "nsIScreen.h"



class nsScreenWin : public nsIScreen
{
public:
  nsScreenWin ( void* inScreen );
  ~nsScreenWin();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:

  void* mScreen;                    
};

#endif  

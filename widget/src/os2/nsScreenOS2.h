




































#ifndef nsScreenOS2_h___
#define nsScreenOS2_h___

#include "nsIScreen.h"

#define INCL_WIN
#define INCL_DOS
#include <os2.h>



class nsScreenOS2 : public nsIScreen
{
public:
  nsScreenOS2 ( );
  virtual ~nsScreenOS2();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:

};

#endif

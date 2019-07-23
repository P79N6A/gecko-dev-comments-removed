




































#ifndef nsScreenOS2_h___
#define nsScreenOS2_h___

#include "nsIScreen.h"



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

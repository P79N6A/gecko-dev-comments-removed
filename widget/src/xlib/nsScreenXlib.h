






































#ifndef nsScreenXlib_h___
#define nsScreenXlib_h___

#include "nsIScreen.h"



class nsScreenXlib : public nsIScreen
{
public:
  nsScreenXlib ( );
  virtual ~nsScreenXlib();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:

};

#endif

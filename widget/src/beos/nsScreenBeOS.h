




































#ifndef nsScreenBeOS_h___
#define nsScreenBeOS_h___

#include "nsIScreen.h"



class nsScreenBeOS : public nsIScreen
{
public:
  nsScreenBeOS ( );
  virtual ~nsScreenBeOS();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:

};

#endif

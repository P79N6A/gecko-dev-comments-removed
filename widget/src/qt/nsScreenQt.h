







































#ifndef nsScreenQt_h___
#define nsScreenQt_h___

#include "nsIScreen.h"



class nsScreenQt : public nsIScreen
{
public:
  nsScreenQt (int aScreen);
  virtual ~nsScreenQt();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:
  int screen;
};

#endif  






































#ifndef nsScreenPh_h___
#define nsScreenPh_h___

#include "nsIScreen.h"



class nsScreenPh : public nsIScreen
{
public:
  nsScreenPh ( );
  virtual ~nsScreenPh();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:
  PRInt32 mWidth;
  PRInt32 mHeight;
  PRInt32 mPixelDepth;
};

#endif  

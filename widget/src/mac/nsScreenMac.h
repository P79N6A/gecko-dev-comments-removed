




































#ifndef nsScreenMac_h___
#define nsScreenMac_h___

#include "nsIScreen.h"

#include <Quickdraw.h>



class nsScreenMac : public nsIScreen
{
public:
  nsScreenMac ( GDHandle inScreen );
  ~nsScreenMac();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:
  
    
    
  PRBool IsPrimaryScreen ( ) const { return (mScreen == ::GetMainDevice()); }
  
  GDHandle mScreen;   
};

#endif  

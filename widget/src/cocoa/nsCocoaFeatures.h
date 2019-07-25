





































#ifndef nsCocoaFeatures_h_
#define nsCocoaFeatures_h_

#include "prtypes.h"

class nsCocoaFeatures {
public:
  static PRInt32 OSXVersion();

  static PRBool OnSnowLeopardOrLater();  
  static PRBool OnLionOrLater();  
private:
  static PRInt32 mOSXVersion;
};
#endif 







































#ifndef nsCocoaFeatures_h_
#define nsCocoaFeatures_h_

#include "prtypes.h"

class nsCocoaFeatures {
public:
  static PRInt32 OSXVersion();

  static bool OnSnowLeopardOrLater();  
  static bool OnLionOrLater();  
private:
  static PRInt32 mOSXVersion;
};
#endif 

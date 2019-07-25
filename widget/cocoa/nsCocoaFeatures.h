




#ifndef nsCocoaFeatures_h_
#define nsCocoaFeatures_h_

#include "prtypes.h"

class nsCocoaFeatures {
public:
  static int32_t OSXVersion();
  static bool OnSnowLeopardOrLater();
  static bool OnLionOrLater();
  static bool SupportCoreAnimationPlugins();

private:
  static int32_t mOSXVersion;
};
#endif 

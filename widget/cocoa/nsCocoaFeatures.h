




#ifndef nsCocoaFeatures_h_
#define nsCocoaFeatures_h_

#include "mozilla/StandardInteger.h"

class nsCocoaFeatures {
public:
  static int32_t OSXVersion();
  static bool OnLionOrLater();
  static bool OnMountainLionOrLater();
  static bool SupportCoreAnimationPlugins();

private:
  static int32_t mOSXVersion;
};
#endif 

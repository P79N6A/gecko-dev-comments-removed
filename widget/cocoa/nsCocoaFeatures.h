




#ifndef nsCocoaFeatures_h_
#define nsCocoaFeatures_h_

#include <stdint.h>

class nsCocoaFeatures {
public:
  static int32_t OSXVersion();
  static bool OnLionOrLater();
  static bool OnMountainLionOrLater();
  static bool SupportCoreAnimationPlugins();
  static bool OnMavericksOrLater();

private:
  static int32_t mOSXVersion;
};
#endif 

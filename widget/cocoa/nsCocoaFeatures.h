




#ifndef nsCocoaFeatures_h_
#define nsCocoaFeatures_h_

#include <stdint.h>

class nsCocoaFeatures {
public:
  static int32_t OSXVersion();
  static int32_t OSXVersionMajor();
  static int32_t OSXVersionMinor();
  static int32_t OSXVersionBugFix();
  static bool OnLionOrLater();
  static bool OnMountainLionOrLater();
  static bool OnMavericksOrLater();
  static bool OnYosemiteOrLater();
  static bool SupportCoreAnimationPlugins();
  static bool AccelerateByDefault();

  static bool IsAtLeastVersion(int32_t aMajor, int32_t aMinor, int32_t aBugFix=0);

private:
  static void InitializeVersionNumbers();

  static int32_t mOSXVersion;
};
#endif 











#ifndef SRC_VIDEO_ENGINE_TEST_AUTO_TEST_AUTOMATED_VIE_LEGACY_FIXTURE_H_
#define SRC_VIDEO_ENGINE_TEST_AUTO_TEST_AUTOMATED_VIE_LEGACY_FIXTURE_H_

#include "video_engine/test/auto_test/automated/two_windows_fixture.h"


class LegacyFixture : public TwoWindowsFixture {
 public:
  
  static void SetUpTestCase();

  
  static void TearDownTestCase();

 protected:
  static ViEAutoTest* tests_;
};

#endif  

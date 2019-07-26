









#include "video_engine/test/auto_test/automated/legacy_fixture.h"

#include "video_engine/test/auto_test/interface/vie_autotest.h"

void LegacyFixture::SetUpTestCase() {
  TwoWindowsFixture::SetUpTestCase();

  
  tests_ = new ViEAutoTest(window_1_, window_2_);
}

void LegacyFixture::TearDownTestCase() {
  delete tests_;

  TwoWindowsFixture::TearDownTestCase();
}

ViEAutoTest* LegacyFixture::tests_ = NULL;





#include "base/gfx/native_theme.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(NativeThemeTest, Init) {
  ASSERT_TRUE(gfx::NativeTheme::instance() != NULL);
}

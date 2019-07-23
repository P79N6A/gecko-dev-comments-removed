



#include "base/path_service.h"

#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/file_path.h"
#if defined(OS_WIN)
#include "base/win_util.h"
#endif
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "testing/platform_test.h"

namespace {



bool ReturnsValidPath(int dir_type) {
  FilePath path;
  bool result = PathService::Get(dir_type, &path);
  return result && !path.value().empty() && file_util::PathExists(path);
}

#if defined(OS_WIN)

bool ReturnsInvalidPath(int dir_type) {
  std::wstring path;
  bool result = PathService::Get(base::DIR_LOCAL_APP_DATA_LOW, &path);
  return !result && path.empty();
}
#endif

}  



typedef PlatformTest PathServiceTest;





TEST_F(PathServiceTest, Get) {
  for (int key = base::DIR_CURRENT; key < base::PATH_END; ++key) {
    EXPECT_PRED1(ReturnsValidPath, key);
  }
#ifdef OS_WIN
  for (int key = base::PATH_WIN_START + 1; key < base::PATH_WIN_END; ++key) {
    if (key == base::DIR_LOCAL_APP_DATA_LOW &&
        win_util::GetWinVersion() < win_util::WINVERSION_VISTA) {
      
      
      EXPECT_TRUE(ReturnsInvalidPath(key)) << key;
    } else {
      EXPECT_TRUE(ReturnsValidPath(key)) << key;
    }
  }
#endif
}

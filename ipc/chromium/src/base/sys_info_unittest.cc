



#include "base/file_util.h"
#include "base/sys_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

typedef PlatformTest SysInfoTest;

TEST_F(SysInfoTest, NumProcs) {
  
  EXPECT_GE(base::SysInfo::NumberOfProcessors(), 1);
}

TEST_F(SysInfoTest, AmountOfMem) {
  
  EXPECT_GT(base::SysInfo::AmountOfPhysicalMemory(), 0);
  EXPECT_GT(base::SysInfo::AmountOfPhysicalMemoryMB(), 0);
}

TEST_F(SysInfoTest, AmountOfFreeDiskSpace) {
  
  std::wstring tmp_path;
  ASSERT_TRUE(file_util::GetTempDir(&tmp_path));
  EXPECT_GT(base::SysInfo::AmountOfFreeDiskSpace(tmp_path), 0) << tmp_path;
}

TEST_F(SysInfoTest, GetEnvVar) {
  
  EXPECT_NE(base::SysInfo::GetEnvVar(L"PATH"), L"");
}

TEST_F(SysInfoTest, HasEnvVar) {
  
  EXPECT_TRUE(base::SysInfo::HasEnvVar(L"PATH"));
}



#if defined(OS_WIN) || defined(OS_MACOSX)
TEST_F(SysInfoTest, OperatingSystemVersionNumbers) {
  int32 os_major_version = -1;
  int32 os_minor_version = -1;
  int32 os_bugfix_version = -1;
  base::SysInfo::OperatingSystemVersionNumbers(&os_major_version,
                                               &os_minor_version,
                                               &os_bugfix_version);
  EXPECT_GT(os_major_version, -1);
  EXPECT_GT(os_minor_version, -1);
  EXPECT_GT(os_bugfix_version, -1);
}
#endif  









































#include "gtest/gtest.h"

#if GTEST_OS_WINDOWS_MOBILE
# include <windows.h>
#elif GTEST_OS_WINDOWS
# include <direct.h>
#endif  






#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

namespace testing {
namespace internal {
namespace {


FilePath GetAbsolutePathOf(const FilePath& relative_path) {
  return FilePath::ConcatPaths(FilePath::GetCurrentDir(), relative_path);
}



TEST(XmlOutputTest, GetOutputFormatDefault) {
  GTEST_FLAG(output) = "";
  EXPECT_STREQ("", UnitTestOptions::GetOutputFormat().c_str());
}

TEST(XmlOutputTest, GetOutputFormat) {
  GTEST_FLAG(output) = "xml:filename";
  EXPECT_STREQ("xml", UnitTestOptions::GetOutputFormat().c_str());
}

TEST(XmlOutputTest, GetOutputFileDefault) {
  GTEST_FLAG(output) = "";
  EXPECT_STREQ(GetAbsolutePathOf(FilePath("test_detail.xml")).c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
}

TEST(XmlOutputTest, GetOutputFileSingleFile) {
  GTEST_FLAG(output) = "xml:filename.abc";
  EXPECT_STREQ(GetAbsolutePathOf(FilePath("filename.abc")).c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
}

TEST(XmlOutputTest, GetOutputFileFromDirectoryPath) {
  GTEST_FLAG(output) = "xml:path" GTEST_PATH_SEP_;
  const std::string expected_output_file =
      GetAbsolutePathOf(
          FilePath(std::string("path") + GTEST_PATH_SEP_ +
                   GetCurrentExecutableName().c_str() + ".xml")).c_str();
  const String& output_file = UnitTestOptions::GetAbsolutePathToOutputFile();
#if GTEST_OS_WINDOWS
  EXPECT_STRCASEEQ(expected_output_file.c_str(), output_file.c_str());
#else
  EXPECT_EQ(expected_output_file, output_file.c_str());
#endif
}

TEST(OutputFileHelpersTest, GetCurrentExecutableName) {
  const std::string exe_str = GetCurrentExecutableName().c_str();
#if GTEST_OS_WINDOWS
  const bool success =
      _strcmpi("gtest-options_test", exe_str.c_str()) == 0 ||
      _strcmpi("gtest-options-ex_test", exe_str.c_str()) == 0 ||
      _strcmpi("gtest_all_test", exe_str.c_str()) == 0 ||
      _strcmpi("gtest_dll_test", exe_str.c_str()) == 0;
#else
  
  
  const bool success =
      exe_str == "gtest-options_test" ||
      exe_str == "gtest_all_test" ||
      exe_str == "lt-gtest_all_test" ||
      exe_str == "gtest_dll_test";
#endif  
  if (!success)
    FAIL() << "GetCurrentExecutableName() returns " << exe_str;
}

class XmlOutputChangeDirTest : public Test {
 protected:
  virtual void SetUp() {
    original_working_dir_ = FilePath::GetCurrentDir();
    posix::ChDir("..");
    
    EXPECT_STRNE(original_working_dir_.c_str(),
                 FilePath::GetCurrentDir().c_str());
  }

  virtual void TearDown() {
    posix::ChDir(original_working_dir_.c_str());
  }

  FilePath original_working_dir_;
};

TEST_F(XmlOutputChangeDirTest, PreserveOriginalWorkingDirWithDefault) {
  GTEST_FLAG(output) = "";
  EXPECT_STREQ(FilePath::ConcatPaths(original_working_dir_,
                                     FilePath("test_detail.xml")).c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
}

TEST_F(XmlOutputChangeDirTest, PreserveOriginalWorkingDirWithDefaultXML) {
  GTEST_FLAG(output) = "xml";
  EXPECT_STREQ(FilePath::ConcatPaths(original_working_dir_,
                                     FilePath("test_detail.xml")).c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
}

TEST_F(XmlOutputChangeDirTest, PreserveOriginalWorkingDirWithRelativeFile) {
  GTEST_FLAG(output) = "xml:filename.abc";
  EXPECT_STREQ(FilePath::ConcatPaths(original_working_dir_,
                                     FilePath("filename.abc")).c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
}

TEST_F(XmlOutputChangeDirTest, PreserveOriginalWorkingDirWithRelativePath) {
  GTEST_FLAG(output) = "xml:path" GTEST_PATH_SEP_;
  const std::string expected_output_file =
      FilePath::ConcatPaths(
          original_working_dir_,
          FilePath(std::string("path") + GTEST_PATH_SEP_ +
                   GetCurrentExecutableName().c_str() + ".xml")).c_str();
  const String& output_file = UnitTestOptions::GetAbsolutePathToOutputFile();
#if GTEST_OS_WINDOWS
  EXPECT_STRCASEEQ(expected_output_file.c_str(), output_file.c_str());
#else
  EXPECT_EQ(expected_output_file, output_file.c_str());
#endif
}

TEST_F(XmlOutputChangeDirTest, PreserveOriginalWorkingDirWithAbsoluteFile) {
#if GTEST_OS_WINDOWS
  GTEST_FLAG(output) = "xml:c:\\tmp\\filename.abc";
  EXPECT_STREQ(FilePath("c:\\tmp\\filename.abc").c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
#else
  GTEST_FLAG(output) ="xml:/tmp/filename.abc";
  EXPECT_STREQ(FilePath("/tmp/filename.abc").c_str(),
               UnitTestOptions::GetAbsolutePathToOutputFile().c_str());
#endif
}

TEST_F(XmlOutputChangeDirTest, PreserveOriginalWorkingDirWithAbsolutePath) {
#if GTEST_OS_WINDOWS
  const std::string path = "c:\\tmp\\";
#else
  const std::string path = "/tmp/";
#endif

  GTEST_FLAG(output) = "xml:" + path;
  const std::string expected_output_file =
      path + GetCurrentExecutableName().c_str() + ".xml";
  const String& output_file = UnitTestOptions::GetAbsolutePathToOutputFile();

#if GTEST_OS_WINDOWS
  EXPECT_STRCASEEQ(expected_output_file.c_str(), output_file.c_str());
#else
  EXPECT_EQ(expected_output_file, output_file.c_str());
#endif
}

}  
}  
}  











#include "webrtc/test/testsupport/fileutils.h"

#include <stdio.h>

#include <list>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/gtest_disable.h"

#ifdef WIN32
#define chdir _chdir
static const char* kPathDelimiter = "\\";
#else
static const char* kPathDelimiter = "/";
#endif

static const std::string kResourcesDir = "resources";
static const std::string kTestName = "fileutils_unittest";
static const std::string kExtension = "tmp";

namespace webrtc {




class FileUtilsTest : public testing::Test {
 protected:
  FileUtilsTest() {
  }
  virtual ~FileUtilsTest() {}
  
  static void SetUpTestCase() {
    original_working_dir_ = webrtc::test::WorkingDir();
  }
  void SetUp() {
    ASSERT_EQ(chdir(original_working_dir_.c_str()), 0);
  }
  void TearDown() {
    ASSERT_EQ(chdir(original_working_dir_.c_str()), 0);
  }
 private:
  static std::string original_working_dir_;
};

std::string FileUtilsTest::original_working_dir_ = "";





TEST_F(FileUtilsTest, ProjectRootPath) {
  std::string project_root = webrtc::test::ProjectRootPath();
  
  ASSERT_GT(project_root.length(), 0u);
}


TEST_F(FileUtilsTest, DISABLED_ON_ANDROID(OutputPathFromUnchangedWorkingDir)) {
  std::string path = webrtc::test::OutputPath();
  std::string expected_end = "out";
  expected_end = kPathDelimiter + expected_end + kPathDelimiter;
  ASSERT_EQ(path.length() - expected_end.length(), path.find(expected_end));
}



TEST_F(FileUtilsTest, DISABLED_ON_ANDROID(OutputPathFromRootWorkingDir)) {
  ASSERT_EQ(0, chdir(kPathDelimiter));
  ASSERT_EQ("./", webrtc::test::OutputPath());
}

TEST_F(FileUtilsTest, TempFilename) {
  std::string temp_filename = webrtc::test::TempFilename(
      webrtc::test::OutputPath(), "TempFilenameTest");
  ASSERT_TRUE(webrtc::test::FileExists(temp_filename))
      << "Couldn't find file: " << temp_filename;
  remove(temp_filename.c_str());
}


TEST_F(FileUtilsTest, CreateDir) {
  std::string directory = "fileutils-unittest-empty-dir";
  
  remove(directory.c_str());
  ASSERT_TRUE(webrtc::test::CreateDir(directory));
  remove(directory.c_str());
}

TEST_F(FileUtilsTest, WorkingDirReturnsValue) {
  
  
  std::string working_dir = webrtc::test::WorkingDir();
  ASSERT_GT(working_dir.length(), 0u);
}





TEST_F(FileUtilsTest, ResourcePathReturnsValue) {
  std::string resource = webrtc::test::ResourcePath(kTestName, kExtension);
  ASSERT_GT(resource.find(kTestName), 0u);
  ASSERT_GT(resource.find(kExtension), 0u);
}

TEST_F(FileUtilsTest, ResourcePathFromRootWorkingDir) {
  ASSERT_EQ(0, chdir(kPathDelimiter));
  std::string resource = webrtc::test::ResourcePath(kTestName, kExtension);
  ASSERT_NE(resource.find("resources"), std::string::npos);
  ASSERT_GT(resource.find(kTestName), 0u);
  ASSERT_GT(resource.find(kExtension), 0u);
}

TEST_F(FileUtilsTest, GetFileSizeExistingFile) {
  
  std::string temp_filename = webrtc::test::TempFilename(
      webrtc::test::OutputPath(), "fileutils_unittest");
  FILE* file = fopen(temp_filename.c_str(), "wb");
  ASSERT_TRUE(file != NULL) << "Failed to open file: " << temp_filename;
  ASSERT_GT(fprintf(file, "%s",  "Dummy data"), 0) <<
      "Failed to write to file: " << temp_filename;
  fclose(file);
  ASSERT_GT(webrtc::test::GetFileSize(std::string(temp_filename.c_str())), 0u);
  remove(temp_filename.c_str());
}

TEST_F(FileUtilsTest, GetFileSizeNonExistingFile) {
  ASSERT_EQ(0u, webrtc::test::GetFileSize("non-existing-file.tmp"));
}

}  











#include "testsupport/fileutils.h"

#include <cstdio>
#include <list>
#include <string>

#include "gtest/gtest.h"

#ifdef WIN32
#define chdir _chdir
static const char* kPathDelimiter = "\\";
#else
static const char* kPathDelimiter = "/";
#endif

static const std::string kDummyDir = "file_utils_unittest_dummy_dir";
static const std::string kResourcesDir = "resources";
static const std::string kTestName = "fileutils_unittest";
static const std::string kExtension = "tmp";

typedef std::list<std::string> FileList;

namespace webrtc {




class FileUtilsTest : public testing::Test {
 protected:
  FileUtilsTest() {
  }
  virtual ~FileUtilsTest() {}
  
  static void SetUpTestCase() {
    original_working_dir_ = webrtc::test::WorkingDir();
    std::string resources_path = original_working_dir_ + kPathDelimiter +
        kResourcesDir + kPathDelimiter;
    webrtc::test::CreateDirectory(resources_path);

    files_.push_back(resources_path + kTestName + "." + kExtension);
    files_.push_back(resources_path + kTestName + "_32." + kExtension);
    files_.push_back(resources_path + kTestName + "_64." + kExtension);
    files_.push_back(resources_path + kTestName + "_linux." + kExtension);
    files_.push_back(resources_path + kTestName + "_mac." + kExtension);
    files_.push_back(resources_path + kTestName + "_win." + kExtension);
    files_.push_back(resources_path + kTestName + "_linux_32." + kExtension);
    files_.push_back(resources_path + kTestName + "_mac_32." + kExtension);
    files_.push_back(resources_path + kTestName + "_win_32." + kExtension);
    files_.push_back(resources_path + kTestName + "_linux_64." + kExtension);
    files_.push_back(resources_path + kTestName + "_mac_64." + kExtension);
    files_.push_back(resources_path + kTestName + "_win_64." + kExtension);

    
    for (FileList::iterator file_it = files_.begin();
        file_it != files_.end(); ++file_it) {
      FILE* file = fopen(file_it->c_str(), "wb");
      ASSERT_TRUE(file != NULL) << "Failed to write file: " << file_it->c_str();
      ASSERT_GT(fprintf(file, "%s",  "Dummy data"), 0);
      fclose(file);
    }
    
    empty_dummy_dir_ = original_working_dir_ + kPathDelimiter + kDummyDir;
    webrtc::test::CreateDirectory(empty_dummy_dir_);
  }
  static void TearDownTestCase() {
    
    for (FileList::iterator file_it = files_.begin();
            file_it != files_.end(); ++file_it) {
      remove(file_it->c_str());
    }
    std::remove(empty_dummy_dir_.c_str());
  }
  void SetUp() {
    ASSERT_EQ(chdir(original_working_dir_.c_str()), 0);
  }
  void TearDown() {
    ASSERT_EQ(chdir(original_working_dir_.c_str()), 0);
  }
 protected:
  static FileList files_;
  static std::string empty_dummy_dir_;
 private:
  static std::string original_working_dir_;
};

FileList FileUtilsTest::files_;
std::string FileUtilsTest::original_working_dir_ = "";
std::string FileUtilsTest::empty_dummy_dir_ = "";






TEST_F(FileUtilsTest, ProjectRootPathFromUnchangedWorkingDir) {
  std::string path = webrtc::test::ProjectRootPath();
  std::string expected_end = "trunk";
  expected_end = kPathDelimiter + expected_end + kPathDelimiter;
  ASSERT_EQ(path.length() - expected_end.length(), path.find(expected_end));
}


TEST_F(FileUtilsTest, OutputPathFromUnchangedWorkingDir) {
  std::string path = webrtc::test::OutputPath();
  std::string expected_end = "out";
  expected_end = kPathDelimiter + expected_end + kPathDelimiter;
  ASSERT_EQ(path.length() - expected_end.length(), path.find(expected_end));
}




TEST_F(FileUtilsTest, ProjectRootPathFromDeeperWorkingDir) {
  std::string path = webrtc::test::ProjectRootPath();
  std::string original_working_dir = path;  
  
  ASSERT_EQ(0, chdir(empty_dummy_dir_.c_str()));
  ASSERT_EQ(original_working_dir, webrtc::test::ProjectRootPath());
}


TEST_F(FileUtilsTest, OutputPathFromDeeperWorkingDir) {
  std::string path = webrtc::test::OutputPath();
  std::string original_working_dir = path;
  ASSERT_EQ(0, chdir(empty_dummy_dir_.c_str()));
  ASSERT_EQ(original_working_dir, webrtc::test::OutputPath());
}




TEST_F(FileUtilsTest, ProjectRootPathFromRootWorkingDir) {
  
  
  ASSERT_EQ(0, chdir(kPathDelimiter));
  ASSERT_EQ(webrtc::test::kCannotFindProjectRootDir,
            webrtc::test::ProjectRootPath());
}


TEST_F(FileUtilsTest, OutputPathFromRootWorkingDir) {
  ASSERT_EQ(0, chdir(kPathDelimiter));
  ASSERT_EQ("./", webrtc::test::OutputPath());
}


TEST_F(FileUtilsTest, CreateDirectory) {
  std::string directory = "fileutils-unittest-empty-dir";
  
  std::remove(directory.c_str());
  ASSERT_TRUE(webrtc::test::CreateDirectory(directory));
  std::remove(directory.c_str());
}

TEST_F(FileUtilsTest, WorkingDirReturnsValue) {
  
  
  std::string working_dir = webrtc::test::WorkingDir();
  ASSERT_GT(working_dir.length(), 0u);
}





TEST_F(FileUtilsTest, ResourcePathReturnsValue) {
  std::string resource = webrtc::test::ResourcePath(kTestName, kExtension);
  ASSERT_GT(resource.find(kTestName), 0u);
  ASSERT_GT(resource.find(kExtension), 0u);
  ASSERT_EQ(0, chdir(kPathDelimiter));
  ASSERT_EQ("./", webrtc::test::OutputPath());
}

TEST_F(FileUtilsTest, GetFileSizeExistingFile) {
  ASSERT_GT(webrtc::test::GetFileSize(files_.front()), 0u);
}

TEST_F(FileUtilsTest, GetFileSizeNonExistingFile) {
  ASSERT_EQ(0u, webrtc::test::GetFileSize("non-existing-file.tmp"));
}

}  

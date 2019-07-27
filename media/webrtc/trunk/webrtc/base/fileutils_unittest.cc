









#include "webrtc/base/fileutils.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"

namespace rtc {


TEST(FilesystemTest, GetTemporaryFolder) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetTemporaryFolder(path, true, NULL));
}


TEST(FilesystemTest, TestOpenFile) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetTemporaryFolder(path, true, NULL));
  path.SetPathname(Filesystem::TempFilename(path, "ut"));

  FileStream* fs;
  char buf[256];
  size_t bytes;

  fs = Filesystem::OpenFile(path, "wb");
  ASSERT_TRUE(fs != NULL);
  EXPECT_EQ(SR_SUCCESS, fs->Write("test", 4, &bytes, NULL));
  EXPECT_EQ(4U, bytes);
  delete fs;

  EXPECT_TRUE(Filesystem::IsFile(path));

  fs = Filesystem::OpenFile(path, "rb");
  ASSERT_TRUE(fs != NULL);
  EXPECT_EQ(SR_SUCCESS, fs->Read(buf, sizeof(buf), &bytes, NULL));
  EXPECT_EQ(4U, bytes);
  delete fs;

  EXPECT_TRUE(Filesystem::DeleteFile(path));
  EXPECT_FALSE(Filesystem::IsFile(path));
}


TEST(FilesystemTest, TestOpenBadFile) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetTemporaryFolder(path, true, NULL));
  path.SetFilename("not an actual file");

  EXPECT_FALSE(Filesystem::IsFile(path));

  FileStream* fs = Filesystem::OpenFile(path, "rb");
  EXPECT_FALSE(fs != NULL);
}



TEST(FilesystemTest, TestCreatePrivateFile) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetTemporaryFolder(path, true, NULL));
  path.SetFilename("private_file_test");

  
  EXPECT_TRUE(Filesystem::CreatePrivateFile(path));
  
  EXPECT_FALSE(Filesystem::CreatePrivateFile(path));

  
  scoped_ptr<FileStream> fs(Filesystem::OpenFile(path, "wb"));
  EXPECT_TRUE(fs.get() != NULL);
  
  fs.reset();

  
  EXPECT_TRUE(Filesystem::DeleteFile(path));
}


TEST(FilesystemTest, TestGetDiskFreeSpace) {
  
  
  Pathname path;
  ASSERT_TRUE(Filesystem::GetAppDataFolder(&path, true));

  int64 free1 = 0;
  EXPECT_TRUE(Filesystem::IsFolder(path));
  EXPECT_FALSE(Filesystem::IsFile(path));
  EXPECT_TRUE(Filesystem::GetDiskFreeSpace(path, &free1));
  EXPECT_GT(free1, 0);

  int64 free2 = 0;
  path.AppendFolder("this_folder_doesnt_exist");
  EXPECT_FALSE(Filesystem::IsFolder(path));
  EXPECT_TRUE(Filesystem::IsAbsent(path));
  EXPECT_TRUE(Filesystem::GetDiskFreeSpace(path, &free2));
  
  
  EXPECT_LT(static_cast<int64>(free1 * .9), free2);
  EXPECT_LT(free2, static_cast<int64>(free1 * 1.1));

  int64 free3 = 0;
  path.clear();
  EXPECT_TRUE(path.empty());
  EXPECT_TRUE(Filesystem::GetDiskFreeSpace(path, &free3));
  
  
  
  EXPECT_GT(free3, 0);
}


TEST(FilesystemTest, TestGetCurrentDirectory) {
  EXPECT_FALSE(Filesystem::GetCurrentDirectory().empty());
}


TEST(FilesystemTest, TestGetAppPathname) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetAppPathname(&path));
  EXPECT_FALSE(path.empty());
}

}  

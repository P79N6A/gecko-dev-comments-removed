



#include "base/file_util.h"
#include "base/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ScopedTempDir, FullPath) {
  FilePath test_path;
  file_util::CreateNewTempDirectory(FILE_PATH_LITERAL("scoped_temp_dir"),
                                    &test_path);

  
  EXPECT_TRUE(file_util::DirectoryExists(test_path));
  {
    ScopedTempDir dir;
    EXPECT_TRUE(dir.Set(test_path));
    EXPECT_TRUE(dir.IsValid());
  }
  EXPECT_FALSE(file_util::DirectoryExists(test_path));

  {
    ScopedTempDir dir;
    dir.Set(test_path);
    
    EXPECT_TRUE(file_util::DirectoryExists(test_path));
    
    FilePath path = dir.Take();
    EXPECT_EQ(path.value(), test_path.value());
    EXPECT_FALSE(dir.IsValid());
  }
  EXPECT_TRUE(file_util::DirectoryExists(test_path));

  
  {
    ScopedTempDir dir;
    dir.Set(test_path);
  }
  EXPECT_FALSE(file_util::DirectoryExists(test_path));
}

TEST(ScopedTempDir, TempDir) {
  
  
  FilePath test_path;
  {
    ScopedTempDir dir;
    EXPECT_TRUE(dir.CreateUniqueTempDir());
    test_path = dir.path();
    EXPECT_TRUE(file_util::DirectoryExists(test_path));
    FilePath tmp_dir;
    EXPECT_TRUE(file_util::GetTempDir(&tmp_dir));
    EXPECT_TRUE(test_path.value().find(tmp_dir.value()) != std::string::npos);
  }
  EXPECT_FALSE(file_util::DirectoryExists(test_path));
}

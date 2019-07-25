







































#include "gtest/internal/gtest-filepath.h"
#include "gtest/gtest.h"






#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

#if GTEST_OS_WINDOWS_MOBILE
# include <windows.h>  
#elif GTEST_OS_WINDOWS
# include <direct.h>  
#endif  

namespace testing {
namespace internal {
namespace {

#if GTEST_OS_WINDOWS_MOBILE




int remove(const char* path) {
  LPCWSTR wpath = String::AnsiToUtf16(path);
  int ret = DeleteFile(wpath) ? 0 : -1;
  delete [] wpath;
  return ret;
}

int _rmdir(const char* path) {
  FilePath filepath(path);
  LPCWSTR wpath = String::AnsiToUtf16(
      filepath.RemoveTrailingPathSeparator().c_str());
  int ret = RemoveDirectory(wpath) ? 0 : -1;
  delete [] wpath;
  return ret;
}

#else

TEST(GetCurrentDirTest, ReturnsCurrentDir) {
  const FilePath original_dir = FilePath::GetCurrentDir();
  EXPECT_FALSE(original_dir.IsEmpty());

  posix::ChDir(GTEST_PATH_SEP_);
  const FilePath cwd = FilePath::GetCurrentDir();
  posix::ChDir(original_dir.c_str());

# if GTEST_OS_WINDOWS

  
  const char* const cwd_without_drive = strchr(cwd.c_str(), ':');
  ASSERT_TRUE(cwd_without_drive != NULL);
  EXPECT_STREQ(GTEST_PATH_SEP_, cwd_without_drive + 1);

# else

  EXPECT_STREQ(GTEST_PATH_SEP_, cwd.c_str());

# endif
}

#endif  

TEST(IsEmptyTest, ReturnsTrueForEmptyPath) {
  EXPECT_TRUE(FilePath("").IsEmpty());
  EXPECT_TRUE(FilePath(NULL).IsEmpty());
}

TEST(IsEmptyTest, ReturnsFalseForNonEmptyPath) {
  EXPECT_FALSE(FilePath("a").IsEmpty());
  EXPECT_FALSE(FilePath(".").IsEmpty());
  EXPECT_FALSE(FilePath("a/b").IsEmpty());
  EXPECT_FALSE(FilePath("a\\b\\").IsEmpty());
}


TEST(RemoveDirectoryNameTest, WhenEmptyName) {
  EXPECT_STREQ("", FilePath("").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, ButNoDirectory) {
  EXPECT_STREQ("afile",
      FilePath("afile").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, RootFileShouldGiveFileName) {
  EXPECT_STREQ("afile",
      FilePath(GTEST_PATH_SEP_ "afile").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, WhereThereIsNoFileName) {
  EXPECT_STREQ("",
      FilePath("adir" GTEST_PATH_SEP_).RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, ShouldGiveFileName) {
  EXPECT_STREQ("afile",
      FilePath("adir" GTEST_PATH_SEP_ "afile").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, ShouldAlsoGiveFileName) {
  EXPECT_STREQ("afile",
      FilePath("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_ "afile")
      .RemoveDirectoryName().c_str());
}

#if GTEST_HAS_ALT_PATH_SEP_





TEST(RemoveDirectoryNameTest, RootFileShouldGiveFileNameForAlternateSeparator) {
  EXPECT_STREQ("afile",
               FilePath("/afile").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, WhereThereIsNoFileNameForAlternateSeparator) {
  EXPECT_STREQ("",
               FilePath("adir/").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, ShouldGiveFileNameForAlternateSeparator) {
  EXPECT_STREQ("afile",
               FilePath("adir/afile").RemoveDirectoryName().c_str());
}


TEST(RemoveDirectoryNameTest, ShouldAlsoGiveFileNameForAlternateSeparator) {
  EXPECT_STREQ("afile",
               FilePath("adir/subdir/afile").RemoveDirectoryName().c_str());
}

#endif


TEST(RemoveFileNameTest, EmptyName) {
#if GTEST_OS_WINDOWS_MOBILE
  
  EXPECT_STREQ(GTEST_PATH_SEP_,
      FilePath("").RemoveFileName().c_str());
#else
  EXPECT_STREQ("." GTEST_PATH_SEP_,
      FilePath("").RemoveFileName().c_str());
#endif
}


TEST(RemoveFileNameTest, ButNoFile) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
      FilePath("adir" GTEST_PATH_SEP_).RemoveFileName().c_str());
}


TEST(RemoveFileNameTest, GivesDirName) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
      FilePath("adir" GTEST_PATH_SEP_ "afile")
      .RemoveFileName().c_str());
}


TEST(RemoveFileNameTest, GivesDirAndSubDirName) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_,
      FilePath("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_ "afile")
      .RemoveFileName().c_str());
}


TEST(RemoveFileNameTest, GivesRootDir) {
  EXPECT_STREQ(GTEST_PATH_SEP_,
      FilePath(GTEST_PATH_SEP_ "afile").RemoveFileName().c_str());
}

#if GTEST_HAS_ALT_PATH_SEP_





TEST(RemoveFileNameTest, ButNoFileForAlternateSeparator) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
               FilePath("adir/").RemoveFileName().c_str());
}


TEST(RemoveFileNameTest, GivesDirNameForAlternateSeparator) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_,
               FilePath("adir/afile").RemoveFileName().c_str());
}


TEST(RemoveFileNameTest, GivesDirAndSubDirNameForAlternateSeparator) {
  EXPECT_STREQ("adir" GTEST_PATH_SEP_ "subdir" GTEST_PATH_SEP_,
               FilePath("adir/subdir/afile").RemoveFileName().c_str());
}


TEST(RemoveFileNameTest, GivesRootDirForAlternateSeparator) {
  EXPECT_STREQ(GTEST_PATH_SEP_,
               FilePath("/afile").RemoveFileName().c_str());
}

#endif

TEST(MakeFileNameTest, GenerateWhenNumberIsZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo"), FilePath("bar"),
      0, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateFileNameNumberGtZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo"), FilePath("bar"),
      12, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar_12.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateFileNameWithSlashNumberIsZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo" GTEST_PATH_SEP_),
      FilePath("bar"), 0, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateFileNameWithSlashNumberGtZero) {
  FilePath actual = FilePath::MakeFileName(FilePath("foo" GTEST_PATH_SEP_),
      FilePath("bar"), 12, "xml");
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar_12.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateWhenNumberIsZeroAndDirIsEmpty) {
  FilePath actual = FilePath::MakeFileName(FilePath(""), FilePath("bar"),
      0, "xml");
  EXPECT_STREQ("bar.xml", actual.c_str());
}

TEST(MakeFileNameTest, GenerateWhenNumberIsNotZeroAndDirIsEmpty) {
  FilePath actual = FilePath::MakeFileName(FilePath(""), FilePath("bar"),
      14, "xml");
  EXPECT_STREQ("bar_14.xml", actual.c_str());
}

TEST(ConcatPathsTest, WorksWhenDirDoesNotEndWithPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo"),
                                          FilePath("bar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(ConcatPathsTest, WorksWhenPath1EndsWithPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo" GTEST_PATH_SEP_),
                                          FilePath("bar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar.xml", actual.c_str());
}

TEST(ConcatPathsTest, Path1BeingEmpty) {
  FilePath actual = FilePath::ConcatPaths(FilePath(""),
                                          FilePath("bar.xml"));
  EXPECT_STREQ("bar.xml", actual.c_str());
}

TEST(ConcatPathsTest, Path2BeingEmpty) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo"),
                                          FilePath(""));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_, actual.c_str());
}

TEST(ConcatPathsTest, BothPathBeingEmpty) {
  FilePath actual = FilePath::ConcatPaths(FilePath(""),
                                          FilePath(""));
  EXPECT_STREQ("", actual.c_str());
}

TEST(ConcatPathsTest, Path1ContainsPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo" GTEST_PATH_SEP_ "bar"),
                                          FilePath("foobar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_ "foobar.xml",
               actual.c_str());
}

TEST(ConcatPathsTest, Path2ContainsPathSep) {
  FilePath actual = FilePath::ConcatPaths(
      FilePath("foo" GTEST_PATH_SEP_),
      FilePath("bar" GTEST_PATH_SEP_ "bar.xml"));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_ "bar.xml",
               actual.c_str());
}

TEST(ConcatPathsTest, Path2EndsWithPathSep) {
  FilePath actual = FilePath::ConcatPaths(FilePath("foo"),
                                          FilePath("bar" GTEST_PATH_SEP_));
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_, actual.c_str());
}


TEST(RemoveTrailingPathSeparatorTest, EmptyString) {
  EXPECT_STREQ("",
      FilePath("").RemoveTrailingPathSeparator().c_str());
}


TEST(RemoveTrailingPathSeparatorTest, FileNoSlashString) {
  EXPECT_STREQ("foo",
      FilePath("foo").RemoveTrailingPathSeparator().c_str());
}


TEST(RemoveTrailingPathSeparatorTest, ShouldRemoveTrailingSeparator) {
  EXPECT_STREQ(
      "foo",
      FilePath("foo" GTEST_PATH_SEP_).RemoveTrailingPathSeparator().c_str());
#if GTEST_HAS_ALT_PATH_SEP_
  EXPECT_STREQ("foo",
               FilePath("foo/").RemoveTrailingPathSeparator().c_str());
#endif
}


TEST(RemoveTrailingPathSeparatorTest, ShouldRemoveLastSeparator) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ "bar" GTEST_PATH_SEP_)
               .RemoveTrailingPathSeparator().c_str());
}


TEST(RemoveTrailingPathSeparatorTest, ShouldReturnUnmodified) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ "bar")
               .RemoveTrailingPathSeparator().c_str());
}

TEST(DirectoryTest, RootDirectoryExists) {
#if GTEST_OS_WINDOWS  
  char current_drive[_MAX_PATH];  
  current_drive[0] = static_cast<char>(_getdrive() + 'A' - 1);
  current_drive[1] = ':';
  current_drive[2] = '\\';
  current_drive[3] = '\0';
  EXPECT_TRUE(FilePath(current_drive).DirectoryExists());
#else
  EXPECT_TRUE(FilePath("/").DirectoryExists());
#endif  
}

#if GTEST_OS_WINDOWS
TEST(DirectoryTest, RootOfWrongDriveDoesNotExists) {
  const int saved_drive_ = _getdrive();
  
  for (char drive = 'Z'; drive >= 'A'; drive--)
    if (_chdrive(drive - 'A' + 1) == -1) {
      char non_drive[_MAX_PATH];  
      non_drive[0] = drive;
      non_drive[1] = ':';
      non_drive[2] = '\\';
      non_drive[3] = '\0';
      EXPECT_FALSE(FilePath(non_drive).DirectoryExists());
      break;
    }
  _chdrive(saved_drive_);
}
#endif  

#if !GTEST_OS_WINDOWS_MOBILE

TEST(DirectoryTest, EmptyPathDirectoryDoesNotExist) {
  EXPECT_FALSE(FilePath("").DirectoryExists());
}
#endif  

TEST(DirectoryTest, CurrentDirectoryExists) {
#if GTEST_OS_WINDOWS  
# ifndef _WIN32_CE  

  EXPECT_TRUE(FilePath(".").DirectoryExists());
  EXPECT_TRUE(FilePath(".\\").DirectoryExists());

# endif  
#else
  EXPECT_TRUE(FilePath(".").DirectoryExists());
  EXPECT_TRUE(FilePath("./").DirectoryExists());
#endif  
}

TEST(NormalizeTest, NullStringsEqualEmptyDirectory) {
  EXPECT_STREQ("", FilePath(NULL).c_str());
  EXPECT_STREQ("", FilePath(String(NULL)).c_str());
}


TEST(NormalizeTest, MultipleConsecutiveSepaparatorsInMidstring) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_ "bar",
               FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_
                        GTEST_PATH_SEP_ "bar").c_str());
}


TEST(NormalizeTest, MultipleConsecutiveSepaparatorsAtStringStart) {
  EXPECT_STREQ(GTEST_PATH_SEP_ "bar",
    FilePath(GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ(GTEST_PATH_SEP_ "bar",
    FilePath(GTEST_PATH_SEP_ GTEST_PATH_SEP_ "bar").c_str());
  EXPECT_STREQ(GTEST_PATH_SEP_ "bar",
    FilePath(GTEST_PATH_SEP_ GTEST_PATH_SEP_ GTEST_PATH_SEP_ "bar").c_str());
}


TEST(NormalizeTest, MultipleConsecutiveSepaparatorsAtStringEnd) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
    FilePath("foo" GTEST_PATH_SEP_).c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
    FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_).c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
    FilePath("foo" GTEST_PATH_SEP_ GTEST_PATH_SEP_ GTEST_PATH_SEP_).c_str());
}

#if GTEST_HAS_ALT_PATH_SEP_




TEST(NormalizeTest, MixAlternateSeparatorAtStringEnd) {
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
               FilePath("foo/").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
               FilePath("foo" GTEST_PATH_SEP_ "/").c_str());
  EXPECT_STREQ("foo" GTEST_PATH_SEP_,
               FilePath("foo//" GTEST_PATH_SEP_).c_str());
}

#endif

TEST(AssignmentOperatorTest, DefaultAssignedToNonDefault) {
  FilePath default_path;
  FilePath non_default_path("path");
  non_default_path = default_path;
  EXPECT_STREQ("", non_default_path.c_str());
  EXPECT_STREQ("", default_path.c_str());  
}

TEST(AssignmentOperatorTest, NonDefaultAssignedToDefault) {
  FilePath non_default_path("path");
  FilePath default_path;
  default_path = non_default_path;
  EXPECT_STREQ("path", default_path.c_str());
  EXPECT_STREQ("path", non_default_path.c_str());  
}

TEST(AssignmentOperatorTest, ConstAssignedToNonConst) {
  const FilePath const_default_path("const_path");
  FilePath non_default_path("path");
  non_default_path = const_default_path;
  EXPECT_STREQ("const_path", non_default_path.c_str());
}

class DirectoryCreationTest : public Test {
 protected:
  virtual void SetUp() {
    testdata_path_.Set(FilePath(String::Format("%s%s%s",
        TempDir().c_str(), GetCurrentExecutableName().c_str(),
        "_directory_creation" GTEST_PATH_SEP_ "test" GTEST_PATH_SEP_)));
    testdata_file_.Set(testdata_path_.RemoveTrailingPathSeparator());

    unique_file0_.Set(FilePath::MakeFileName(testdata_path_, FilePath("unique"),
        0, "txt"));
    unique_file1_.Set(FilePath::MakeFileName(testdata_path_, FilePath("unique"),
        1, "txt"));

    remove(testdata_file_.c_str());
    remove(unique_file0_.c_str());
    remove(unique_file1_.c_str());
    posix::RmDir(testdata_path_.c_str());
  }

  virtual void TearDown() {
    remove(testdata_file_.c_str());
    remove(unique_file0_.c_str());
    remove(unique_file1_.c_str());
    posix::RmDir(testdata_path_.c_str());
  }

  String TempDir() const {
#if GTEST_OS_WINDOWS_MOBILE
    return String("\\temp\\");
#elif GTEST_OS_WINDOWS
    const char* temp_dir = posix::GetEnv("TEMP");
    if (temp_dir == NULL || temp_dir[0] == '\0')
      return String("\\temp\\");
    else if (String(temp_dir).EndsWith("\\"))
      return String(temp_dir);
    else
      return String::Format("%s\\", temp_dir);
#else
    return String("/tmp/");
#endif  
  }

  void CreateTextFile(const char* filename) {
    FILE* f = posix::FOpen(filename, "w");
    fprintf(f, "text\n");
    fclose(f);
  }

  
  
  
  FilePath testdata_path_;  
  FilePath testdata_file_;  
  FilePath unique_file0_;  
  FilePath unique_file1_;  
};

TEST_F(DirectoryCreationTest, CreateDirectoriesRecursively) {
  EXPECT_FALSE(testdata_path_.DirectoryExists()) << testdata_path_.c_str();
  EXPECT_TRUE(testdata_path_.CreateDirectoriesRecursively());
  EXPECT_TRUE(testdata_path_.DirectoryExists());
}

TEST_F(DirectoryCreationTest, CreateDirectoriesForAlreadyExistingPath) {
  EXPECT_FALSE(testdata_path_.DirectoryExists()) << testdata_path_.c_str();
  EXPECT_TRUE(testdata_path_.CreateDirectoriesRecursively());
  
  EXPECT_TRUE(testdata_path_.CreateDirectoriesRecursively());
}

TEST_F(DirectoryCreationTest, CreateDirectoriesAndUniqueFilename) {
  FilePath file_path(FilePath::GenerateUniqueFileName(testdata_path_,
      FilePath("unique"), "txt"));
  EXPECT_STREQ(unique_file0_.c_str(), file_path.c_str());
  EXPECT_FALSE(file_path.FileOrDirectoryExists());  

  testdata_path_.CreateDirectoriesRecursively();
  EXPECT_FALSE(file_path.FileOrDirectoryExists());  
  CreateTextFile(file_path.c_str());
  EXPECT_TRUE(file_path.FileOrDirectoryExists());

  FilePath file_path2(FilePath::GenerateUniqueFileName(testdata_path_,
      FilePath("unique"), "txt"));
  EXPECT_STREQ(unique_file1_.c_str(), file_path2.c_str());
  EXPECT_FALSE(file_path2.FileOrDirectoryExists());  
  CreateTextFile(file_path2.c_str());
  EXPECT_TRUE(file_path2.FileOrDirectoryExists());
}

TEST_F(DirectoryCreationTest, CreateDirectoriesFail) {
  
  CreateTextFile(testdata_file_.c_str());
  EXPECT_TRUE(testdata_file_.FileOrDirectoryExists());
  EXPECT_FALSE(testdata_file_.DirectoryExists());
  EXPECT_FALSE(testdata_file_.CreateDirectoriesRecursively());
}

TEST(NoDirectoryCreationTest, CreateNoDirectoriesForDefaultXmlFile) {
  const FilePath test_detail_xml("test_detail.xml");
  EXPECT_FALSE(test_detail_xml.CreateDirectoriesRecursively());
}

TEST(FilePathTest, DefaultConstructor) {
  FilePath fp;
  EXPECT_STREQ("", fp.c_str());
}

TEST(FilePathTest, CharAndCopyConstructors) {
  const FilePath fp("spicy");
  EXPECT_STREQ("spicy", fp.c_str());

  const FilePath fp_copy(fp);
  EXPECT_STREQ("spicy", fp_copy.c_str());
}

TEST(FilePathTest, StringConstructor) {
  const FilePath fp(String("cider"));
  EXPECT_STREQ("cider", fp.c_str());
}

TEST(FilePathTest, Set) {
  const FilePath apple("apple");
  FilePath mac("mac");
  mac.Set(apple);  
  EXPECT_STREQ("apple", mac.c_str());
  EXPECT_STREQ("apple", apple.c_str());
}

TEST(FilePathTest, ToString) {
  const FilePath file("drink");
  String str(file.ToString());
  EXPECT_STREQ("drink", str.c_str());
}

TEST(FilePathTest, RemoveExtension) {
  EXPECT_STREQ("app", FilePath("app.exe").RemoveExtension("exe").c_str());
  EXPECT_STREQ("APP", FilePath("APP.EXE").RemoveExtension("exe").c_str());
}

TEST(FilePathTest, RemoveExtensionWhenThereIsNoExtension) {
  EXPECT_STREQ("app", FilePath("app").RemoveExtension("exe").c_str());
}

TEST(FilePathTest, IsDirectory) {
  EXPECT_FALSE(FilePath("cola").IsDirectory());
  EXPECT_TRUE(FilePath("koala" GTEST_PATH_SEP_).IsDirectory());
#if GTEST_HAS_ALT_PATH_SEP_
  EXPECT_TRUE(FilePath("koala/").IsDirectory());
#endif
}

TEST(FilePathTest, IsAbsolutePath) {
  EXPECT_FALSE(FilePath("is" GTEST_PATH_SEP_ "relative").IsAbsolutePath());
  EXPECT_FALSE(FilePath("").IsAbsolutePath());
#if GTEST_OS_WINDOWS
  EXPECT_TRUE(FilePath("c:\\" GTEST_PATH_SEP_ "is_not"
                       GTEST_PATH_SEP_ "relative").IsAbsolutePath());
  EXPECT_FALSE(FilePath("c:foo" GTEST_PATH_SEP_ "bar").IsAbsolutePath());
  EXPECT_TRUE(FilePath("c:/" GTEST_PATH_SEP_ "is_not"
                       GTEST_PATH_SEP_ "relative").IsAbsolutePath());
#else
  EXPECT_TRUE(FilePath(GTEST_PATH_SEP_ "is_not" GTEST_PATH_SEP_ "relative")
              .IsAbsolutePath());
#endif  
}

TEST(FilePathTest, IsRootDirectory) {
#if GTEST_OS_WINDOWS
  EXPECT_TRUE(FilePath("a:\\").IsRootDirectory());
  EXPECT_TRUE(FilePath("Z:/").IsRootDirectory());
  EXPECT_TRUE(FilePath("e://").IsRootDirectory());
  EXPECT_FALSE(FilePath("").IsRootDirectory());
  EXPECT_FALSE(FilePath("b:").IsRootDirectory());
  EXPECT_FALSE(FilePath("b:a").IsRootDirectory());
  EXPECT_FALSE(FilePath("8:/").IsRootDirectory());
  EXPECT_FALSE(FilePath("c|/").IsRootDirectory());
#else
  EXPECT_TRUE(FilePath("/").IsRootDirectory());
  EXPECT_TRUE(FilePath("//").IsRootDirectory());
  EXPECT_FALSE(FilePath("").IsRootDirectory());
  EXPECT_FALSE(FilePath("\\").IsRootDirectory());
  EXPECT_FALSE(FilePath("/x").IsRootDirectory());
#endif
}

}  
}  
}  

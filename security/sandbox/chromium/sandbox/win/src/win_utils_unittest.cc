



#include <windows.h>

#include "base/win/scoped_handle.h"
#include "sandbox/win/src/win_utils.h"
#include "sandbox/win/tests/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(WinUtils, IsReparsePoint) {
  using sandbox::IsReparsePoint;

  
  wchar_t temp_directory[MAX_PATH];
  wchar_t my_folder[MAX_PATH];
  ASSERT_NE(::GetTempPath(MAX_PATH, temp_directory), 0u);
  ASSERT_NE(::GetTempFileName(temp_directory, L"test", 0, my_folder), 0u);

  
  ASSERT_TRUE(::DeleteFile(my_folder));
  ASSERT_TRUE(::CreateDirectory(my_folder, NULL));

  bool result = true;
  EXPECT_EQ(ERROR_SUCCESS, IsReparsePoint(my_folder, &result));
  EXPECT_FALSE(result);

  
  base::string16 not_found = base::string16(my_folder) + L"\\foo\\bar";
  

  base::string16 new_file = base::string16(my_folder) + L"\\foo";
  EXPECT_EQ(ERROR_SUCCESS, IsReparsePoint(new_file, &result));
  EXPECT_FALSE(result);

  
  HANDLE dir = ::CreateFile(my_folder, FILE_ALL_ACCESS,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  EXPECT_NE(INVALID_HANDLE_VALUE, dir);

  base::string16 temp_dir_nt = base::string16(L"\\??\\") + temp_directory;
  EXPECT_TRUE(SetReparsePoint(dir, temp_dir_nt.c_str()));

  EXPECT_EQ(ERROR_SUCCESS, IsReparsePoint(new_file, &result));
  EXPECT_TRUE(result);

  EXPECT_TRUE(DeleteReparsePoint(dir));
  EXPECT_TRUE(::CloseHandle(dir));
  EXPECT_TRUE(::RemoveDirectory(my_folder));
}

TEST(WinUtils, SameObject) {
  using sandbox::SameObject;

  
  wchar_t temp_directory[MAX_PATH];
  wchar_t my_folder[MAX_PATH];
  ASSERT_NE(::GetTempPath(MAX_PATH, temp_directory), 0u);
  ASSERT_NE(::GetTempFileName(temp_directory, L"test", 0, my_folder), 0u);

  
  ASSERT_TRUE(::DeleteFile(my_folder));
  ASSERT_TRUE(::CreateDirectory(my_folder, NULL));

  base::string16 folder(my_folder);
  base::string16 file_name = folder + L"\\foo.txt";
  const ULONG kSharing = FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE;
  base::win::ScopedHandle file(CreateFile(
      file_name.c_str(), GENERIC_WRITE, kSharing, NULL, CREATE_ALWAYS,
      FILE_FLAG_DELETE_ON_CLOSE, NULL));

  EXPECT_TRUE(file.IsValid());
  base::string16 file_name_nt1 = base::string16(L"\\??\\") + file_name;
  base::string16 file_name_nt2 =
      base::string16(L"\\??\\") + folder + L"\\FOO.txT";
  EXPECT_TRUE(SameObject(file.Get(), file_name_nt1.c_str()));
  EXPECT_TRUE(SameObject(file.Get(), file_name_nt2.c_str()));

  file.Close();
  EXPECT_TRUE(::RemoveDirectory(my_folder));
}













#include "webrtc/base/gunit.h"
#include "webrtc/base/common.h"
#include "webrtc/base/win32regkey.h"

namespace rtc {

#ifndef EXPECT_SUCCEEDED
#define EXPECT_SUCCEEDED(x)  EXPECT_TRUE(SUCCEEDED(x))
#endif

#ifndef EXPECT_FAILED
#define EXPECT_FAILED(x)  EXPECT_TRUE(FAILED(x))
#endif

#define kBaseKey           L"Software\\Google\\__TEST"
#define kSubkeyName        L"subkey_test"

const wchar_t kRkey1[] = kBaseKey;
const wchar_t kRkey1SubkeyName[] = kSubkeyName;
const wchar_t kRkey1Subkey[] = kBaseKey L"\\" kSubkeyName;
const wchar_t kFullRkey1[] = L"HKCU\\" kBaseKey;
const wchar_t kFullRkey1Subkey[] = L"HKCU\\" kBaseKey L"\\" kSubkeyName;

const wchar_t kValNameInt[] = L"Int32 Value";
const DWORD kIntVal = 20;
const DWORD kIntVal2 = 30;

const wchar_t kValNameInt64[] = L"Int64 Value";
const DWORD64 kIntVal64 = 119600064000000000uI64;

const wchar_t kValNameFloat[] = L"Float Value";
const float kFloatVal = 12.3456789f;

const wchar_t kValNameDouble[] = L"Double Value";
const double kDoubleVal = 98.7654321;

const wchar_t kValNameStr[] = L"Str Value";
const wchar_t kStrVal[] = L"Some string data 1";
const wchar_t kStrVal2[] = L"Some string data 2";

const wchar_t kValNameBinary[] = L"Binary Value";
const char kBinaryVal[] = "Some binary data abcdefghi 1";
const char kBinaryVal2[] = "Some binary data abcdefghi 2";

const wchar_t kValNameMultiStr[] = L"MultiStr Value";
const wchar_t kMultiSZ[] = L"abc\0def\0P12345\0";
const wchar_t kEmptyMultiSZ[] = L"";
const wchar_t kInvalidMultiSZ[] = {L'6', L'7', L'8'};


void RegKeyHelperFunctionsTest() {
  
  std::wstring temp_key = L"";
  EXPECT_TRUE(RegKey::GetRootKeyInfo(&temp_key) == NULL);
  EXPECT_STREQ(temp_key.c_str(), L"");

  temp_key = L"a";
  EXPECT_TRUE(RegKey::GetRootKeyInfo(&temp_key) == NULL);
  EXPECT_STREQ(temp_key.c_str(), L"");

  
  temp_key = L"HKLM\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_LOCAL_MACHINE);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKEY_LOCAL_MACHINE\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_LOCAL_MACHINE);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKCU\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_CURRENT_USER);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKEY_CURRENT_USER\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_CURRENT_USER);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKU\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_USERS);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKEY_USERS\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_USERS);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKCR\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_CLASSES_ROOT);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"HKEY_CLASSES_ROOT\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_CLASSES_ROOT);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  
  temp_key = L"hkcr\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_CLASSES_ROOT);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"hkey_CLASSES_ROOT\\a";
  EXPECT_EQ(RegKey::GetRootKeyInfo(&temp_key), HKEY_CLASSES_ROOT);
  EXPECT_STREQ(temp_key.c_str(), L"a");

  
  
  

  
  temp_key = L"";
  EXPECT_STREQ(RegKey::GetParentKeyInfo(&temp_key).c_str(), L"");
  EXPECT_STREQ(temp_key.c_str(), L"");

  temp_key = L"a";
  EXPECT_STREQ(RegKey::GetParentKeyInfo(&temp_key).c_str(), L"");
  EXPECT_STREQ(temp_key.c_str(), L"a");

  temp_key = L"a\\b";
  EXPECT_STREQ(RegKey::GetParentKeyInfo(&temp_key).c_str(), L"a");
  EXPECT_STREQ(temp_key.c_str(), L"b");

  temp_key = L"\\b";
  EXPECT_STREQ(RegKey::GetParentKeyInfo(&temp_key).c_str(), L"");
  EXPECT_STREQ(temp_key.c_str(), L"b");

  
  temp_key = L"HKEY_CLASSES_ROOT\\moon";
  EXPECT_STREQ(RegKey::GetParentKeyInfo(&temp_key).c_str(),
               L"HKEY_CLASSES_ROOT");
  EXPECT_STREQ(temp_key.c_str(), L"moon");

  temp_key = L"HKEY_CLASSES_ROOT\\moon\\doggy";
  EXPECT_STREQ(RegKey::GetParentKeyInfo(&temp_key).c_str(),
               L"HKEY_CLASSES_ROOT\\moon");
  EXPECT_STREQ(temp_key.c_str(), L"doggy");

  
  
  

  std::vector<std::wstring> result;
  EXPECT_SUCCEEDED(RegKey::MultiSZBytesToStringArray(
      reinterpret_cast<const uint8*>(kMultiSZ), sizeof(kMultiSZ), &result));
  EXPECT_EQ(result.size(), 3);
  EXPECT_STREQ(result[0].c_str(), L"abc");
  EXPECT_STREQ(result[1].c_str(), L"def");
  EXPECT_STREQ(result[2].c_str(), L"P12345");

  EXPECT_SUCCEEDED(RegKey::MultiSZBytesToStringArray(
      reinterpret_cast<const uint8*>(kEmptyMultiSZ),
      sizeof(kEmptyMultiSZ), &result));
  EXPECT_EQ(result.size(), 0);
  EXPECT_FALSE(SUCCEEDED(RegKey::MultiSZBytesToStringArray(
      reinterpret_cast<const uint8*>(kInvalidMultiSZ),
      sizeof(kInvalidMultiSZ), &result)));
}

TEST(RegKeyTest, RegKeyHelperFunctionsTest) {
  RegKeyHelperFunctionsTest();
}

TEST(RegKeyTest, RegKeyNonStaticFunctionsTest) {
  DWORD int_val = 0;
  DWORD64 int64_val = 0;
  wchar_t* str_val = NULL;
  uint8* binary_val = NULL;
  DWORD uint8_count = 0;

  
  
  RegKey::DeleteKey(kFullRkey1);

  
  RegKey r_key;
  EXPECT_TRUE(r_key.key() == NULL);

  
  EXPECT_SUCCEEDED(r_key.Create(HKEY_CURRENT_USER, kRkey1));

  
  EXPECT_SUCCEEDED(r_key.Create(HKEY_CURRENT_USER, kRkey1));

  
  EXPECT_SUCCEEDED(r_key.Open(HKEY_CURRENT_USER, kRkey1));

  
  EXPECT_EQ(r_key.GetValue(kValNameInt, &int_val),
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameInt, kIntVal));

  
  EXPECT_TRUE(r_key.HasValue(kValNameInt));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameInt, &int_val));
  EXPECT_EQ(int_val, kIntVal);

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameInt, kIntVal2));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameInt, &int_val));
  EXPECT_EQ(int_val, kIntVal2);

  
  EXPECT_SUCCEEDED(r_key.DeleteValue(kValNameInt));

  
  EXPECT_FALSE(r_key.HasValue(kValNameInt));

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameInt64, kIntVal64));

  
  EXPECT_TRUE(r_key.HasValue(kValNameInt64));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameInt64, &int64_val));
  EXPECT_EQ(int64_val, kIntVal64);

  
  EXPECT_SUCCEEDED(r_key.DeleteValue(kValNameInt64));

  
  EXPECT_FALSE(r_key.HasValue(kValNameInt64));

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameStr, kStrVal));

  
  EXPECT_TRUE(r_key.HasValue(kValNameStr));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameStr, &str_val));
  EXPECT_TRUE(lstrcmp(str_val, kStrVal) == 0);
  delete[] str_val;

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameStr, kStrVal2));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameStr, &str_val));
  EXPECT_TRUE(lstrcmp(str_val, kStrVal2) == 0);
  delete[] str_val;

  
  EXPECT_SUCCEEDED(r_key.DeleteValue(kValNameStr));

  
  EXPECT_FALSE(r_key.HasValue(kValNameInt));

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameBinary,
      reinterpret_cast<const uint8*>(kBinaryVal), sizeof(kBinaryVal) - 1));

  
  EXPECT_TRUE(r_key.HasValue(kValNameBinary));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameBinary, &binary_val, &uint8_count));
  EXPECT_TRUE(memcmp(binary_val, kBinaryVal, sizeof(kBinaryVal) - 1) == 0);
  delete[] binary_val;

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameBinary,
      reinterpret_cast<const uint8*>(kBinaryVal2), sizeof(kBinaryVal) - 1));

  
  EXPECT_SUCCEEDED(r_key.GetValue(kValNameBinary, &binary_val, &uint8_count));
  EXPECT_TRUE(memcmp(binary_val, kBinaryVal2, sizeof(kBinaryVal2) - 1) == 0);
  delete[] binary_val;

  
  EXPECT_SUCCEEDED(r_key.DeleteValue(kValNameBinary));

  
  EXPECT_FALSE(r_key.HasValue(kValNameBinary));

  

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameInt, kIntVal));

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameInt64, kIntVal64));

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameStr, kStrVal));

  
  EXPECT_SUCCEEDED(r_key.SetValue(kValNameBinary,
      reinterpret_cast<const uint8*>(kBinaryVal), sizeof(kBinaryVal) - 1));

  
  uint32 value_count = r_key.GetValueCount();
  EXPECT_EQ(value_count, 4);

  
  std::wstring value_name;
  DWORD type = 0;

  EXPECT_SUCCEEDED(r_key.GetValueNameAt(0, &value_name, &type));
  EXPECT_STREQ(value_name.c_str(), kValNameInt);
  EXPECT_EQ(type, REG_DWORD);

  EXPECT_SUCCEEDED(r_key.GetValueNameAt(1, &value_name, &type));
  EXPECT_STREQ(value_name.c_str(), kValNameInt64);
  EXPECT_EQ(type, REG_QWORD);

  EXPECT_SUCCEEDED(r_key.GetValueNameAt(2, &value_name, &type));
  EXPECT_STREQ(value_name.c_str(), kValNameStr);
  EXPECT_EQ(type, REG_SZ);

  EXPECT_SUCCEEDED(r_key.GetValueNameAt(3, &value_name, &type));
  EXPECT_STREQ(value_name.c_str(), kValNameBinary);
  EXPECT_EQ(type, REG_BINARY);

  
  EXPECT_FAILED(r_key.GetValueNameAt(4, &value_name, &type));

  uint32 subkey_count = r_key.GetSubkeyCount();
  EXPECT_EQ(subkey_count, 0);

  
  RegKey temp_key;
  EXPECT_SUCCEEDED(temp_key.Create(HKEY_CURRENT_USER, kRkey1Subkey));

  
  EXPECT_TRUE(r_key.HasSubkey(kRkey1SubkeyName));

  
  EXPECT_EQ(r_key.GetSubkeyCount(), 1);

  std::wstring subkey_name;
  EXPECT_SUCCEEDED(r_key.GetSubkeyNameAt(0, &subkey_name));
  EXPECT_STREQ(subkey_name.c_str(), kRkey1SubkeyName);

  
  EXPECT_SUCCEEDED(r_key.DeleteSubKey(kRkey1));

  
  EXPECT_SUCCEEDED(r_key.Close());

  
  EXPECT_SUCCEEDED(RegKey::DeleteKey(kFullRkey1));
}

TEST(RegKeyTest, RegKeyStaticFunctionsTest) {
  DWORD int_val = 0;
  DWORD64 int64_val = 0;
  float float_val = 0;
  double double_val = 0;
  wchar_t* str_val = NULL;
  std::wstring wstr_val;
  uint8* binary_val = NULL;
  DWORD uint8_count = 0;

  
  
  RegKey::DeleteKey(kFullRkey1);

  
  EXPECT_EQ(RegKey::GetValue(kFullRkey1, kValNameInt, &int_val),
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameInt, kIntVal));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameInt));

  
  EXPECT_EQ(RegKey::GetValue(kFullRkey1, L"bogus", &int_val),
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameInt, &int_val));
  EXPECT_EQ(int_val, kIntVal);

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameInt));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameInt));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameInt64, kIntVal64));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameInt64));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameInt64, &int64_val));
  EXPECT_EQ(int64_val, kIntVal64);

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameInt64));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameInt64));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameFloat, kFloatVal));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameFloat));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameFloat, &float_val));
  EXPECT_EQ(float_val, kFloatVal);

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameFloat));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameFloat));
  EXPECT_FAILED(RegKey::GetValue(kFullRkey1, kValNameFloat, &float_val));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameDouble, kDoubleVal));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameDouble));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameDouble, &double_val));
  EXPECT_EQ(double_val, kDoubleVal);

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameDouble));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameDouble));
  EXPECT_FAILED(RegKey::GetValue(kFullRkey1, kValNameDouble, &double_val));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameStr, kStrVal));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameStr));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameStr, &str_val));
  EXPECT_TRUE(lstrcmp(str_val, kStrVal) == 0);
  delete[] str_val;

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameStr, &wstr_val));
  EXPECT_STREQ(wstr_val.c_str(), kStrVal);

  
  EXPECT_EQ(RegKey::GetValue(kFullRkey1, L"bogus", &str_val),
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameStr));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameStr));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameBinary,
      reinterpret_cast<const uint8*>(kBinaryVal), sizeof(kBinaryVal)-1));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameBinary));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameBinary,
      &binary_val, &uint8_count));
  EXPECT_TRUE(memcmp(binary_val, kBinaryVal, sizeof(kBinaryVal)-1) == 0);
  delete[] binary_val;

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameBinary));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameBinary));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameBinary,
      reinterpret_cast<const uint8*>(kBinaryVal), 0));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameBinary));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameBinary,
      &binary_val, &uint8_count));
  EXPECT_EQ(uint8_count, 0);
  EXPECT_TRUE(binary_val == NULL);
  delete[] binary_val;

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameBinary));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameBinary));

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1, kValNameBinary, NULL, 100));

  
  EXPECT_TRUE(RegKey::HasValue(kFullRkey1, kValNameBinary));

  
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameBinary,
                                    &binary_val, &uint8_count));
  EXPECT_EQ(uint8_count, 0);
  EXPECT_TRUE(binary_val == NULL);
  delete[] binary_val;

  
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1, kValNameBinary));

  
  EXPECT_FALSE(RegKey::HasValue(kFullRkey1, kValNameBinary));

  
  std::vector<std::wstring> result;
  EXPECT_SUCCEEDED(RegKey::SetValueMultiSZ(kFullRkey1, kValNameMultiStr,
      reinterpret_cast<const uint8*>(kMultiSZ), sizeof(kMultiSZ)));
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameMultiStr, &result));
  EXPECT_EQ(result.size(), 3);
  EXPECT_STREQ(result[0].c_str(), L"abc");
  EXPECT_STREQ(result[1].c_str(), L"def");
  EXPECT_STREQ(result[2].c_str(), L"P12345");
  EXPECT_SUCCEEDED(RegKey::SetValueMultiSZ(kFullRkey1, kValNameMultiStr,
      reinterpret_cast<const uint8*>(kEmptyMultiSZ), sizeof(kEmptyMultiSZ)));
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameMultiStr, &result));
  EXPECT_EQ(result.size(), 0);
  
  EXPECT_SUCCEEDED(RegKey::SetValueMultiSZ(kFullRkey1, kValNameMultiStr,
      reinterpret_cast<const uint8*>(kInvalidMultiSZ), sizeof(kInvalidMultiSZ)));
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1, kValNameMultiStr, &result));
  EXPECT_EQ(result.size(), 1);
  EXPECT_STREQ(result[0].c_str(), L"678");

  
  
#ifdef IS_PRIVATE_BUILD
  
  wchar_t temp_path[MAX_PATH] = {0};
  EXPECT_LT(::GetTempPath(ARRAY_SIZE(temp_path), temp_path),
            static_cast<DWORD>(ARRAY_SIZE(temp_path)));
  wchar_t temp_file[MAX_PATH] = {0};
  EXPECT_NE(::GetTempFileName(temp_path, L"rkut_",
                              ::GetTickCount(), temp_file), 0);

  
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1Subkey, kValNameInt, kIntVal));
  EXPECT_SUCCEEDED(RegKey::SetValue(kFullRkey1Subkey, kValNameInt64, kIntVal64));
  EXPECT_SUCCEEDED(RegKey::Save(kFullRkey1Subkey, temp_file));
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1Subkey, kValNameInt));
  EXPECT_SUCCEEDED(RegKey::DeleteValue(kFullRkey1Subkey, kValNameInt64));

  
  EXPECT_SUCCEEDED(RegKey::Restore(kFullRkey1Subkey, temp_file));
  int_val = 0;
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1Subkey, kValNameInt, &int_val));
  EXPECT_EQ(int_val, kIntVal);
  int64_val = 0;
  EXPECT_SUCCEEDED(RegKey::GetValue(kFullRkey1Subkey,
                                    kValNameInt64,
                                    &int64_val));
  EXPECT_EQ(int64_val, kIntVal64);

  
  EXPECT_EQ(TRUE, ::DeleteFile(temp_file));
#endif

  
  EXPECT_SUCCEEDED(RegKey::DeleteKey(kFullRkey1));
}

}  

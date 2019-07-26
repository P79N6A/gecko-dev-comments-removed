



#include <shlobj.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "sandbox/win/src/registry_policy.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/win_utils.h"
#include "sandbox/win/tests/common/controller.h"

namespace {

static const DWORD kAllowedRegFlags = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                                      KEY_NOTIFY | KEY_READ | GENERIC_READ |
                                      GENERIC_EXECUTE | READ_CONTROL;

#define BINDNTDLL(name) \
  name ## Function name = reinterpret_cast<name ## Function>( \
    ::GetProcAddress(::GetModuleHandle(L"ntdll.dll"), #name))

bool IsKeyOpenForRead(HKEY handle) {
  BINDNTDLL(NtQueryObject);

  OBJECT_BASIC_INFORMATION info = {0};
  NTSTATUS status = NtQueryObject(handle, ObjectBasicInformation, &info,
                                  sizeof(info), NULL);

  if (!NT_SUCCESS(status))
    return false;

  if ((info.GrantedAccess & (~kAllowedRegFlags)) != 0)
    return false;
  return true;
}

}

namespace sandbox {

SBOX_TESTS_COMMAND int Reg_OpenKey(int argc, wchar_t **argv) {
  if (argc != 4)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;

  REGSAM desired_access = 0;
  ULONG options = 0;
  if (wcscmp(argv[1], L"read") == 0) {
    desired_access = KEY_READ;
  } else if (wcscmp(argv[1], L"write") == 0) {
    desired_access = KEY_ALL_ACCESS;
  } else if (wcscmp(argv[1], L"link") == 0) {
    options = REG_OPTION_CREATE_LINK;
    desired_access = KEY_ALL_ACCESS;
  } else {
    desired_access = MAXIMUM_ALLOWED;
  }

  HKEY root = GetReservedKeyFromName(argv[2]);
  HKEY key;
  LRESULT result = 0;

  if (wcscmp(argv[0], L"create") == 0)
    result = ::RegCreateKeyEx(root, argv[3], 0, NULL, options, desired_access,
                              NULL, &key, NULL);
  else
    result = ::RegOpenKeyEx(root, argv[3], 0, desired_access, &key);

  if (ERROR_SUCCESS == result) {
    if (MAXIMUM_ALLOWED == desired_access) {
      if (!IsKeyOpenForRead(key)) {
        ::RegCloseKey(key);
        return SBOX_TEST_FAILED;
      }
    }
    ::RegCloseKey(key);
    return SBOX_TEST_SUCCEEDED;
  } else if (ERROR_ACCESS_DENIED == result) {
    return SBOX_TEST_DENIED;
  }

  return SBOX_TEST_FAILED;
}

TEST(RegistryPolicyTest, TestKeyAnyAccess) {
  TestRunner runner;
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_ANY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Microsoft"));

  
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
      L"Reg_OpenKey create read HKEY_LOCAL_MACHINE software\\microsoft"));

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
      L"Reg_OpenKey open read HKEY_LOCAL_MACHINE software\\microsoft"));

  if (::IsUserAnAdmin()) {
    
    EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
        L"Reg_OpenKey create write HKEY_LOCAL_MACHINE software\\microsoft"));

    EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
        L"Reg_OpenKey open write HKEY_LOCAL_MACHINE software\\microsoft"));
  }

  
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"Reg_OpenKey create read "
      L"HKEY_LOCAL_MACHINE software\\microsoft\\Windows"));

  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"Reg_OpenKey open read "
      L"HKEY_LOCAL_MACHINE software\\microsoft\\windows"));

  
  
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"Reg_OpenKey create write "
      L"HKEY_LOCAL_MACHINE software\\Microsoft\\google_unit_tests"));

  RegDeleteKey(HKEY_LOCAL_MACHINE, L"software\\Microsoft\\google_unit_tests");

  
  
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(
      L"Reg_OpenKey create read HKEY_LOCAL_MACHINE software\\microsoft\\"));

  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(
      L"Reg_OpenKey open read HKEY_LOCAL_MACHINE software\\microsoft\\"));
}

TEST(RegistryPolicyTest, TestKeyNoAccess) {
  TestRunner runner;

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE"));

  
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(
      L"Reg_OpenKey create read HKEY_LOCAL_MACHINE software"));

  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(
      L"Reg_OpenKey open read HKEY_LOCAL_MACHINE software"));
}

TEST(RegistryPolicyTest, TestKeyReadOnlyAccess) {
  TestRunner runner;

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Policies"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Policies\\*"));

  
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"Reg_OpenKey create read "
      L"HKEY_LOCAL_MACHINE software\\Policies\\microsoft"));

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"Reg_OpenKey open read "
      L"HKEY_LOCAL_MACHINE software\\Policies\\microsoft"));

  
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(L"Reg_OpenKey create write "
      L"HKEY_LOCAL_MACHINE software\\Policies\\google_unit_tests"));

  RegDeleteKey(HKEY_LOCAL_MACHINE, L"software\\Policies\\google_unit_tests");
}

TEST(RegistryPolicyTest, TestKeyAllAccessSubDir) {
  TestRunner runner;

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_ANY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Policies"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_ANY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Policies\\*"));

  if (::IsUserAnAdmin()) {
    
    EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"Reg_OpenKey create write "
        L"HKEY_LOCAL_MACHINE software\\Policies\\google_unit_tests"));

    RegDeleteKey(HKEY_LOCAL_MACHINE, L"software\\Policies\\google_unit_tests");
  }
}

TEST(RegistryPolicyTest, TestKeyCreateLink) {
  TestRunner runner;

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_LOCAL_MACHINE"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_ANY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Policies"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_ANY,
                             L"HKEY_LOCAL_MACHINE\\Software\\Policies\\*"));

  
  
  
  
  
  
  
  
  
  EXPECT_NE(SBOX_TEST_SUCCEEDED, runner.RunTest(L"Reg_OpenKey create link "
      L"HKEY_LOCAL_MACHINE software\\Policies\\google_unit_tests"));

  
  
  HKEY key = NULL;
  LRESULT result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                  L"software\\Policies\\google_unit_tests",
                                  REG_OPTION_OPEN_LINK, MAXIMUM_ALLOWED,
                                  &key);

  if (!result) {
    HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
    NtDeleteKeyFunction NtDeleteKey =
        reinterpret_cast<NtDeleteKeyFunction>(GetProcAddress(ntdll,
                                                             "NtDeleteKey"));
    NtDeleteKey(key);
  }
}

TEST(RegistryPolicyTest, TestKeyReadOnlyHKCU) {
  TestRunner runner;
  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_CURRENT_USER"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_CURRENT_USER\\Software"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_USERS\\.default"));

  EXPECT_TRUE(runner.AddRule(TargetPolicy::SUBSYS_REGISTRY,
                             TargetPolicy::REG_ALLOW_READONLY,
                             L"HKEY_USERS\\.default\\software"));

  
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
      L"Reg_OpenKey create read HKEY_CURRENT_USER software"));

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
      L"Reg_OpenKey open read HKEY_CURRENT_USER software"));

  
  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(
      L"Reg_OpenKey create write HKEY_CURRENT_USER software"));

  EXPECT_EQ(SBOX_TEST_DENIED, runner.RunTest(
      L"Reg_OpenKey open write HKEY_CURRENT_USER software"));

  
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
      L"Reg_OpenKey create maximum_allowed HKEY_CURRENT_USER software"));

  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(
      L"Reg_OpenKey open maximum_allowed HKEY_CURRENT_USER software"));
}

}  

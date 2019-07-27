



#include "base/win/windows_version.h"
#include "sandbox/win/src/app_container.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {


TEST(AppContainerTest, CreateAppContainer) {
  if (base::win::OSInfo::GetInstance()->version() < base::win::VERSION_WIN8)
    return;

  const wchar_t kName[] = L"Test";
  const wchar_t kValidSid[] = L"S-1-15-2-12345-234-567-890-123-456-789";

  EXPECT_TRUE(LookupAppContainer(kValidSid).empty());
  EXPECT_EQ(SBOX_ERROR_GENERIC, DeleteAppContainer(kValidSid));

  EXPECT_EQ(SBOX_ALL_OK, CreateAppContainer(kValidSid, kName));
  EXPECT_EQ(SBOX_ERROR_GENERIC, CreateAppContainer(kValidSid, kName));
  EXPECT_EQ(kName, LookupAppContainer(kValidSid));
  EXPECT_EQ(SBOX_ALL_OK, DeleteAppContainer(kValidSid));

  EXPECT_TRUE(LookupAppContainer(kValidSid).empty());
  EXPECT_EQ(SBOX_ERROR_GENERIC, DeleteAppContainer(kValidSid));

  EXPECT_EQ(SBOX_ERROR_INVALID_APP_CONTAINER,
            CreateAppContainer(L"Foo", kName));
}


TEST(AppContainerTest, SecurityCapabilities) {
  if (base::win::OSInfo::GetInstance()->version() < base::win::VERSION_WIN8)
    return;

  scoped_ptr<AppContainerAttributes> attributes(new AppContainerAttributes);
  std::vector<base::string16> capabilities;
  EXPECT_EQ(SBOX_ERROR_INVALID_APP_CONTAINER,
            attributes->SetAppContainer(L"S-1-foo", capabilities));

  EXPECT_EQ(SBOX_ALL_OK,
            attributes->SetAppContainer(L"S-1-15-2-12345-234", capabilities));
  EXPECT_TRUE(attributes->HasAppContainer());

  attributes.reset(new AppContainerAttributes);
  capabilities.push_back(L"S-1-15-3-12345678-87654321");
  capabilities.push_back(L"S-1-15-3-1");
  capabilities.push_back(L"S-1-15-3-2");
  capabilities.push_back(L"S-1-15-3-3");
  EXPECT_EQ(SBOX_ALL_OK,
            attributes->SetAppContainer(L"S-1-15-2-1-2", capabilities));
  EXPECT_TRUE(attributes->HasAppContainer());
}

}  





#include <windows.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "base/registry.h"
#include "base/string_util.h"
#include "base/win_util.h"



TEST(BaseWinUtilTest, TestIsUACEnabled) {
  if (win_util::GetWinVersion() >= win_util::WINVERSION_VISTA) {
    win_util::UserAccountControlIsEnabled();
  } else {
    EXPECT_TRUE(win_util::UserAccountControlIsEnabled());
  }
}

TEST(BaseWinUtilTest, TestGetUserSidString) {
  std::wstring user_sid;
  EXPECT_TRUE(win_util::GetUserSidString(&user_sid));
  EXPECT_TRUE(!user_sid.empty());
}

TEST(BaseWinUtilTest, TestGetNonClientMetrics) {
  NONCLIENTMETRICS metrics = {0};
  win_util::GetNonClientMetrics(&metrics);
  EXPECT_TRUE(metrics.cbSize > 0);
  EXPECT_TRUE(metrics.iScrollWidth > 0);
  EXPECT_TRUE(metrics.iScrollHeight > 0);
}

namespace {



class ThreadLocaleSaver {
 public:
  ThreadLocaleSaver() : original_locale_id_(GetThreadLocale()) {}
  ~ThreadLocaleSaver() { SetThreadLocale(original_locale_id_); }

 private:
  LCID original_locale_id_;

  DISALLOW_COPY_AND_ASSIGN(ThreadLocaleSaver);
};

}  

TEST(BaseWinUtilTest, FormatMessage) {
  
  
  ThreadLocaleSaver thread_locale_saver;
  WORD language_id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
  LCID locale_id = MAKELCID(language_id, SORT_DEFAULT);
  ASSERT_TRUE(SetThreadLocale(locale_id));

  const int kAccessDeniedErrorCode = 5;
  SetLastError(kAccessDeniedErrorCode);
  ASSERT_EQ(GetLastError(), kAccessDeniedErrorCode);
  std::wstring value;
  TrimWhitespace(win_util::FormatLastWin32Error(), TRIM_ALL, &value);
  EXPECT_EQ(std::wstring(L"Access is denied."), value);

  
  wchar_t * string_buffer = NULL;
  unsigned string_length =
      ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      kAccessDeniedErrorCode, 0,
                      reinterpret_cast<wchar_t *>(&string_buffer), 0, NULL);

  
  ASSERT_TRUE(string_length);
  ASSERT_TRUE(string_buffer);

  
  EXPECT_EQ(win_util::FormatLastWin32Error(), std::wstring(string_buffer));
  EXPECT_EQ(win_util::FormatMessage(kAccessDeniedErrorCode),
            std::wstring(string_buffer));

  
  LocalFree(string_buffer);
}

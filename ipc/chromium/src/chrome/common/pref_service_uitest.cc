



#include <string>

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/json_value_serializer.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/automation/browser_proxy.h"
#include "chrome/test/automation/window_proxy.h"
#include "chrome/test/ui/ui_test.h"

class PreferenceServiceTest : public UITest {
public:
  void SetUp() {
    PathService::Get(base::DIR_TEMP, &tmp_profile_);
    tmp_profile_ = tmp_profile_.AppendASCII("tmp_profile");

    
    file_util::Delete(tmp_profile_, true);
    file_util::CreateDirectory(tmp_profile_);

    FilePath reference_pref_file =
        test_data_directory_
            .AppendASCII("profiles")
            .AppendASCII("window_placement")
            .Append(chrome::kLocalStateFilename);

    tmp_pref_file_ = tmp_profile_.Append(chrome::kLocalStateFilename);

    ASSERT_TRUE(file_util::PathExists(reference_pref_file));

    
    ASSERT_TRUE(file_util::CopyFile(reference_pref_file, tmp_pref_file_));

#if defined(OS_WIN)
    
    
    ASSERT_TRUE(::SetFileAttributesW(tmp_pref_file_.value().c_str(),
        FILE_ATTRIBUTE_NORMAL));
#endif

    launch_arguments_.AppendSwitchWithValue(switches::kUserDataDir,
                                            tmp_profile_.ToWStringHack());
  }

  bool LaunchAppWithProfile() {
    if (!file_util::PathExists(tmp_pref_file_))
      return false;
    UITest::SetUp();
    return true;
  }

  void TearDown() {
    UITest::TearDown();

    EXPECT_TRUE(DieFileDie(tmp_profile_, true));
  }

public:
  FilePath tmp_pref_file_;
  FilePath tmp_profile_;
};

#if defined(OS_WIN)







TEST_F(PreferenceServiceTest, PreservedWindowPlacementIsLoaded) {
  
  ASSERT_TRUE(LaunchAppWithProfile());

  ASSERT_TRUE(file_util::PathExists(tmp_pref_file_));

  JSONFileValueSerializer deserializer(tmp_pref_file_);
  scoped_ptr<Value> root(deserializer.Deserialize(NULL));

  ASSERT_TRUE(root.get());
  ASSERT_TRUE(root->IsType(Value::TYPE_DICTIONARY));

  DictionaryValue* root_dict = static_cast<DictionaryValue*>(root.get());

  
  scoped_ptr<BrowserProxy> browser(automation()->GetBrowserWindow(0));
  ASSERT_TRUE(browser.get());
  scoped_ptr<WindowProxy> window(browser->GetWindow());

  HWND hWnd;
  ASSERT_TRUE(window->GetHWND(&hWnd));

  WINDOWPLACEMENT window_placement;
  ASSERT_TRUE(GetWindowPlacement(hWnd, &window_placement));

  
  int bottom = 0;
  std::wstring kBrowserWindowPlacement(prefs::kBrowserWindowPlacement);
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + L".bottom",
      &bottom));
  EXPECT_EQ(bottom, window_placement.rcNormalPosition.bottom);

  int top = 0;
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + L".top",
      &top));
  EXPECT_EQ(top, window_placement.rcNormalPosition.top);

  int left = 0;
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + L".left",
      &left));
  EXPECT_EQ(left, window_placement.rcNormalPosition.left);

  int right = 0;
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + L".right",
      &right));
  EXPECT_EQ(right, window_placement.rcNormalPosition.right);

  
  bool is_window_maximized = (window_placement.showCmd == SW_MAXIMIZE);

  bool is_maximized = false;
  EXPECT_TRUE(root_dict->GetBoolean(kBrowserWindowPlacement + L".maximized",
      &is_maximized));
  EXPECT_EQ(is_maximized, is_window_maximized);
}
#endif

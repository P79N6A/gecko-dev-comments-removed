



#include "base/file_path.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/pref_member.h"
#include "chrome/common/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

static const wchar_t kBoolPref[] = L"bool";
static const wchar_t kIntPref[] = L"int";
static const wchar_t kRealPref[] = L"real";
static const wchar_t kStringPref[] = L"string";

void RegisterTestPrefs(PrefService* prefs) {
  prefs->RegisterBooleanPref(kBoolPref, false);
  prefs->RegisterIntegerPref(kIntPref, 0);
  prefs->RegisterRealPref(kRealPref, 0.0);
  prefs->RegisterStringPref(kStringPref, L"default");
}

class PrefMemberTestClass : public NotificationObserver {
 public:
  PrefMemberTestClass(PrefService* prefs) : observe_cnt_(0), prefs_(prefs) {
    str_.Init(kStringPref, prefs, this);
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    DCHECK(NotificationType::PREF_CHANGED == type);
    PrefService* prefs_in = Source<PrefService>(source).ptr();
    EXPECT_EQ(prefs_in, prefs_);
    std::wstring* pref_name_in = Details<std::wstring>(details).ptr();
    EXPECT_EQ(*pref_name_in, kStringPref);
    EXPECT_EQ(str_.GetValue(), prefs_->GetString(kStringPref));
    ++observe_cnt_;
  }

  StringPrefMember str_;
  int observe_cnt_;

 private:
  PrefService* prefs_;
};

}  

TEST(PrefMemberTest, BasicGetAndSet) {
  PrefService prefs(FilePath(), NULL);
  RegisterTestPrefs(&prefs);

  
  BooleanPrefMember boolean;
  boolean.Init(kBoolPref, &prefs, NULL);

  
  EXPECT_FALSE(prefs.GetBoolean(kBoolPref));
  EXPECT_FALSE(boolean.GetValue());
  EXPECT_FALSE(*boolean);

  
  boolean.SetValue(true);
  EXPECT_TRUE(boolean.GetValue());
  EXPECT_TRUE(prefs.GetBoolean(kBoolPref));
  EXPECT_TRUE(*boolean);

  
  prefs.SetBoolean(kBoolPref, false);
  EXPECT_FALSE(prefs.GetBoolean(kBoolPref));
  EXPECT_FALSE(boolean.GetValue());
  EXPECT_FALSE(*boolean);

  
  IntegerPrefMember integer;
  integer.Init(kIntPref, &prefs, NULL);

  
  EXPECT_EQ(0, prefs.GetInteger(kIntPref));
  EXPECT_EQ(0, integer.GetValue());
  EXPECT_EQ(0, *integer);

  
  integer.SetValue(5);
  EXPECT_EQ(5, integer.GetValue());
  EXPECT_EQ(5, prefs.GetInteger(kIntPref));
  EXPECT_EQ(5, *integer);

  
  prefs.SetInteger(kIntPref, 2);
  EXPECT_EQ(2, prefs.GetInteger(kIntPref));
  EXPECT_EQ(2, integer.GetValue());
  EXPECT_EQ(2, *integer);

  
  RealPrefMember real;
  real.Init(kRealPref, &prefs, NULL);

  
  EXPECT_EQ(0.0, prefs.GetReal(kRealPref));
  EXPECT_EQ(0.0, real.GetValue());
  EXPECT_EQ(0.0, *real);

  
  real.SetValue(1.0);
  EXPECT_EQ(1.0, real.GetValue());
  EXPECT_EQ(1.0, prefs.GetReal(kRealPref));
  EXPECT_EQ(1.0, *real);

  
  prefs.SetReal(kRealPref, 3.0);
  EXPECT_EQ(3.0, prefs.GetReal(kRealPref));
  EXPECT_EQ(3.0, real.GetValue());
  EXPECT_EQ(3.0, *real);

  
  StringPrefMember string;
  string.Init(kStringPref, &prefs, NULL);

  
  EXPECT_EQ(L"default", prefs.GetString(kStringPref));
  EXPECT_EQ(L"default", string.GetValue());
  EXPECT_EQ(L"default", *string);

  
  string.SetValue(L"foo");
  EXPECT_EQ(L"foo", string.GetValue());
  EXPECT_EQ(L"foo", prefs.GetString(kStringPref));
  EXPECT_EQ(L"foo", *string);

  
  prefs.SetString(kStringPref, L"bar");
  EXPECT_EQ(L"bar", prefs.GetString(kStringPref));
  EXPECT_EQ(L"bar", string.GetValue());
  EXPECT_EQ(L"bar", *string);
}

TEST(PrefMemberTest, TwoPrefs) {
  
  PrefService prefs(FilePath(), NULL);
  RegisterTestPrefs(&prefs);

  RealPrefMember pref1;
  pref1.Init(kRealPref, &prefs, NULL);
  RealPrefMember pref2;
  pref2.Init(kRealPref, &prefs, NULL);

  pref1.SetValue(2.3);
  EXPECT_EQ(2.3, *pref2);

  pref2.SetValue(3.5);
  EXPECT_EQ(3.5, *pref1);

  prefs.SetReal(kRealPref, 4.2);
  EXPECT_EQ(4.2, *pref1);
  EXPECT_EQ(4.2, *pref2);
}

TEST(PrefMemberTest, Observer) {
  PrefService prefs(FilePath(), NULL);
  RegisterTestPrefs(&prefs);

  PrefMemberTestClass test_obj(&prefs);
  EXPECT_EQ(L"default", *test_obj.str_);

  
  test_obj.str_.SetValue(L"hello");
  EXPECT_EQ(0, test_obj.observe_cnt_);
  EXPECT_EQ(L"hello", prefs.GetString(kStringPref));

  
  prefs.SetString(kStringPref, L"world");
  EXPECT_EQ(1, test_obj.observe_cnt_);
  EXPECT_EQ(L"world", *(test_obj.str_));

  
  prefs.SetString(kStringPref, L"world");
  EXPECT_EQ(1, test_obj.observe_cnt_);
  EXPECT_EQ(L"world", *(test_obj.str_));

  prefs.SetString(kStringPref, L"hello");
  EXPECT_EQ(2, test_obj.observe_cnt_);
  EXPECT_EQ(L"hello", prefs.GetString(kStringPref));
}

TEST(PrefMemberTest, NoInit) {
  
  IntegerPrefMember pref;
}

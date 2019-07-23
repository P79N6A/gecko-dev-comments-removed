






















#ifndef CHROME_COMMON_PREF_MEMBER_H_
#define CHROME_COMMON_PREF_MEMBER_H_

#include <string>

#include "base/basictypes.h"
#include "chrome/common/notification_observer.h"

class PrefService;

namespace subtle {

class PrefMemberBase : public NotificationObserver {
 protected:
  PrefMemberBase();
  virtual ~PrefMemberBase();

  
  void Init(const wchar_t* pref_name, PrefService* prefs,
            NotificationObserver* observer);

  
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  void VerifyValuePrefName();

  
  virtual void UpdateValueFromPref() = 0;

  const std::wstring& pref_name() const { return pref_name_; }
  PrefService* prefs() { return prefs_; }

 
 private:
  std::wstring pref_name_;
  NotificationObserver* observer_;
  PrefService* prefs_;

 protected:
  bool is_synced_;
  bool setting_value_;
};

}  


template <typename ValueType>
class PrefMember : public subtle::PrefMemberBase {
 public:
  
  
  PrefMember() { }
  virtual ~PrefMember() { }

  
  
  void Init(const wchar_t* pref_name, PrefService* prefs,
            NotificationObserver* observer) {
    subtle::PrefMemberBase::Init(pref_name, prefs, observer);
  }

  
  ValueType GetValue() {
    VerifyValuePrefName();
    
    
    if (!is_synced_) {
      UpdateValueFromPref();
      is_synced_ = true;
    }
    return value_;
  }

  
  ValueType operator*() {
    return GetValue();
  }

  
  void SetValue(const ValueType& value) {
    VerifyValuePrefName();
    setting_value_ = true;
    UpdatePref(value);
    setting_value_ = false;
  }

 protected:
  
  virtual void UpdatePref(const ValueType& value) = 0;

  
  
  ValueType value_;
};




class BooleanPrefMember : public PrefMember<bool> {
 public:
  BooleanPrefMember() : PrefMember<bool>() { }
  virtual ~BooleanPrefMember() { }

 protected:
  virtual void UpdateValueFromPref();
  virtual void UpdatePref(const bool& value);

 private:
  DISALLOW_COPY_AND_ASSIGN(BooleanPrefMember);
};

class IntegerPrefMember : public PrefMember<int> {
 public:
  IntegerPrefMember() : PrefMember<int>() { }
  virtual ~IntegerPrefMember() { }

 protected:
  virtual void UpdateValueFromPref();
  virtual void UpdatePref(const int& value);

 private:
  DISALLOW_COPY_AND_ASSIGN(IntegerPrefMember);
};

class RealPrefMember : public PrefMember<double> {
 public:
  RealPrefMember() : PrefMember<double>() { }
  virtual ~RealPrefMember() { }

 protected:
  virtual void UpdateValueFromPref();
  virtual void UpdatePref(const double& value);

 private:
  DISALLOW_COPY_AND_ASSIGN(RealPrefMember);
};

class StringPrefMember : public PrefMember<std::wstring> {
 public:
  StringPrefMember() : PrefMember<std::wstring>() { }
  virtual ~StringPrefMember() { }

 protected:
  virtual void UpdateValueFromPref();
  virtual void UpdatePref(const std::wstring& value);

 private:
  DISALLOW_COPY_AND_ASSIGN(StringPrefMember);
};

#endif  

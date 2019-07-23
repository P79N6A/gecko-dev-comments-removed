













#ifndef CHROME_COMMON_PREF_SERVICE_H_
#define CHROME_COMMON_PREF_SERVICE_H_

#include <set>

#include "base/file_path.h"
#include "base/hash_tables.h"
#include "base/non_thread_safe.h"
#include "base/observer_list.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "chrome/common/important_file_writer.h"

class NotificationObserver;
class Preference;

namespace base {
class Thread;
}

class PrefService : public NonThreadSafe {
 public:

  
  class Preference {
   public:

    
    
    
    
    
    Preference(DictionaryValue* root_pref,
               const wchar_t* name,
               Value* default_value);
    ~Preference() {}

    Value::ValueType type() const { return type_; }

    
    
    const std::wstring name() const { return name_; }

    
    
    const Value* GetValue() const;

    
    bool IsDefaultValue() const;

   private:
    friend class PrefService;

    Value::ValueType type_;
    std::wstring name_;
    scoped_ptr<Value> default_value_;

    
    DictionaryValue* root_pref_;

    DISALLOW_COPY_AND_ASSIGN(Preference);
  };

  
  
  
  
  PrefService(const FilePath& pref_filename,
              const base::Thread* backend_thread);
  ~PrefService();

  
  
  
  bool ReloadPersistentPrefs();

  
  
  
  
  
  bool SavePersistentPrefs();

  
  bool ScheduleSavePersistentPrefs();

  DictionaryValue* transient() { return transient_.get(); }

  
  void RegisterBooleanPref(const wchar_t* path,
                           bool default_value);
  void RegisterIntegerPref(const wchar_t* path,
                           int default_value);
  void RegisterRealPref(const wchar_t* path,
                        double default_value);
  void RegisterStringPref(const wchar_t* path,
                          const std::wstring& default_value);
  void RegisterFilePathPref(const wchar_t* path,
                            const FilePath& default_value);
  void RegisterListPref(const wchar_t* path);
  void RegisterDictionaryPref(const wchar_t* path);

  
  void RegisterLocalizedBooleanPref(const wchar_t* path,
                                    int locale_default_message_id);
  void RegisterLocalizedIntegerPref(const wchar_t* path,
                                    int locale_default_message_id);
  void RegisterLocalizedRealPref(const wchar_t* path,
                                 int locale_default_message_id);
  void RegisterLocalizedStringPref(const wchar_t* path,
                                   int locale_default_message_id);

  
  bool IsPrefRegistered(const wchar_t* path);

  
  
  
  bool GetBoolean(const wchar_t* path) const;
  int GetInteger(const wchar_t* path) const;
  double GetReal(const wchar_t* path) const;
  std::wstring GetString(const wchar_t* path) const;
  FilePath GetFilePath(const wchar_t* path) const;

  
  
  const DictionaryValue* GetDictionary(const wchar_t* path) const;
  const ListValue* GetList(const wchar_t* path) const;

  
  
  void AddPrefObserver(const wchar_t* path, NotificationObserver* obs);
  void RemovePrefObserver(const wchar_t* path, NotificationObserver* obs);

  
  void ClearPref(const wchar_t* path);

  
  void SetBoolean(const wchar_t* path, bool value);
  void SetInteger(const wchar_t* path, int value);
  void SetReal(const wchar_t* path, double value);
  void SetString(const wchar_t* path, const std::wstring& value);
  void SetFilePath(const wchar_t* path, const FilePath& value);

  
  
  
  void SetInt64(const wchar_t* path, int64 value);
  int64 GetInt64(const wchar_t* path) const;
  void RegisterInt64Pref(const wchar_t* path, int64 default_value);

  
  
  
  
  
  
  DictionaryValue* GetMutableDictionary(const wchar_t* path);
  ListValue* GetMutableList(const wchar_t* path);

  
  
  
  
  bool HasPrefPath(const wchar_t* path) const;

  class PreferencePathComparator {
   public:
    bool operator() (Preference* lhs, Preference* rhs) const {
      return lhs->name() < rhs->name();
    }
  };
  typedef std::set<Preference*, PreferencePathComparator> PreferenceSet;
  const PreferenceSet& preference_set() const { return prefs_; }

  
  
  const Preference* FindPreference(const wchar_t* pref_name) const;

 private:
  
  
  void RegisterPreference(Preference* pref);

  
  
  Value* GetPrefCopy(const wchar_t* pref_name);

  
  void FireObservers(const wchar_t* pref_name);

  
  
  void FireObserversIfChanged(const wchar_t* pref_name,
                              const Value* old_value);

  
  
  bool SerializePrefData(std::string* output) const;

  scoped_ptr<DictionaryValue> persistent_;
  scoped_ptr<DictionaryValue> transient_;

  
  ImportantFileWriter writer_;

  
  PreferenceSet prefs_;

  
  
  typedef ObserverList<NotificationObserver> NotificationObserverList;
  typedef base::hash_map<std::wstring, NotificationObserverList*>
      PrefObserverMap;
  PrefObserverMap pref_observers_;

  DISALLOW_COPY_AND_ASSIGN(PrefService);
};

#endif  

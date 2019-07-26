































#ifndef SimpleStringDictionary_H__
#define SimpleStringDictionary_H__

#import <string>
#import <vector>

namespace google_breakpad {




























class KeyValueEntry {
 public:
  KeyValueEntry() {
    Clear();
  }
  
  KeyValueEntry(const char *key, const char *value) {
    SetKeyValue(key, value);
  }

  void        SetKeyValue(const char *key, const char *value) {
    if (!key) {
      key = "";
    }
    if (!value) {
      value = "";
    }
    
    strlcpy(key_, key, sizeof(key_));
    strlcpy(value_, value, sizeof(value_));
  }  

  void        SetValue(const char *value) {
    if (!value) {
      value = "";
    }
    strlcpy(value_, value, sizeof(value_));
  };
  
  
  void        Clear() {
    memset(key_, 0, sizeof(key_));
    memset(value_, 0, sizeof(value_));
  }

  bool        IsActive() const { return key_[0] != '\0'; }
  const char *GetKey() const { return key_; }
  const char *GetValue() const { return value_; }

  
  
  
  enum {MAX_STRING_STORAGE_SIZE = 256};
  
 private:
  char key_[MAX_STRING_STORAGE_SIZE];
  char value_[MAX_STRING_STORAGE_SIZE];
};










class SimpleStringDictionary {
 public:
  SimpleStringDictionary() {};  
  
  
  
  int GetCount() const;

  
  
  
  const char *GetValueForKey(const char *key) const;
    
  
  
  
  void SetKeyValue(const char *key, const char *value);
  
  
  
  void RemoveKey(const char *key);

  
  
  
  enum {MAX_NUM_ENTRIES = 64};

 private:
  friend class SimpleStringDictionaryIterator;

  const KeyValueEntry *GetEntry(int i) const;

  KeyValueEntry             entries_[MAX_NUM_ENTRIES];
};


class SimpleStringDictionaryIterator {
 public:
  SimpleStringDictionaryIterator(const SimpleStringDictionary &dict)
    : dict_(dict), i_(0) {
    }

  
  void Start() {
    i_ = 0;
  }
  
  
  
  
  const KeyValueEntry* Next() {
    for (; i_ < SimpleStringDictionary::MAX_NUM_ENTRIES; ++i_) {
      const KeyValueEntry *entry = dict_.GetEntry(i_);
      if (entry->IsActive()) {
        i_++;   
        return entry;
      }
    }

    return NULL;  
  }
  
 private:
  const SimpleStringDictionary&   dict_;
  int                             i_;
};

}  

#endif  

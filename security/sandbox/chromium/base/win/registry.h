



#ifndef BASE_WIN_REGISTRY_H_
#define BASE_WIN_REGISTRY_H_

#include <windows.h>
#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/stl_util.h"

namespace base {
namespace win {








class BASE_EXPORT RegKey {
 public:
  RegKey();
  explicit RegKey(HKEY key);
  RegKey(HKEY rootkey, const wchar_t* subkey, REGSAM access);
  ~RegKey();

  LONG Create(HKEY rootkey, const wchar_t* subkey, REGSAM access);

  LONG CreateWithDisposition(HKEY rootkey, const wchar_t* subkey,
                             DWORD* disposition, REGSAM access);

  
  LONG CreateKey(const wchar_t* name, REGSAM access);

  
  LONG Open(HKEY rootkey, const wchar_t* subkey, REGSAM access);

  
  LONG OpenKey(const wchar_t* relative_key_name, REGSAM access);

  
  void Close();

  
  void Set(HKEY key);

  
  HKEY Take();

  
  
  bool HasValue(const wchar_t* value_name) const;

  
  
  DWORD GetValueCount() const;

  
  LONG GetValueNameAt(int index, std::wstring* name) const;

  
  bool Valid() const { return key_ != NULL; }

  
  
  LONG DeleteKey(const wchar_t* name);

  
  LONG DeleteValue(const wchar_t* name);

  

  
  
  LONG ReadValueDW(const wchar_t* name, DWORD* out_value) const;

  
  
  LONG ReadInt64(const wchar_t* name, int64* out_value) const;

  
  
  LONG ReadValue(const wchar_t* name, std::wstring* out_value) const;

  
  
  
  LONG ReadValues(const wchar_t* name, std::vector<std::wstring>* values);

  
  
  LONG ReadValue(const wchar_t* name,
                 void* data,
                 DWORD* dsize,
                 DWORD* dtype) const;

  

  
  LONG WriteValue(const wchar_t* name, DWORD in_value);

  
  LONG WriteValue(const wchar_t* name, const wchar_t* in_value);

  
  LONG WriteValue(const wchar_t* name,
                  const void* data,
                  DWORD dsize,
                  DWORD dtype);

  
  
  LONG StartWatching();

  
  
  
  bool HasChanged();

  
  
  LONG StopWatching();

  inline bool IsWatching() const { return watch_event_ != 0; }
  HANDLE watch_event() const { return watch_event_; }
  HKEY Handle() const { return key_; }

 private:
  HKEY key_;  
  HANDLE watch_event_;

  DISALLOW_COPY_AND_ASSIGN(RegKey);
};


class BASE_EXPORT RegistryValueIterator {
 public:
  RegistryValueIterator(HKEY root_key, const wchar_t* folder_key);

  ~RegistryValueIterator();

  DWORD ValueCount() const;

  
  bool Valid() const;

  
  void operator++();

  const wchar_t* Name() const { return name_.c_str(); }
  const wchar_t* Value() const { return vector_as_array(&value_); }
  
  DWORD ValueSize() const { return value_size_; }
  DWORD Type() const { return type_; }

  int Index() const { return index_; }

 private:
  
  bool Read();

  
  HKEY key_;

  
  int index_;

  
  std::wstring name_;
  std::vector<wchar_t> value_;
  DWORD value_size_;
  DWORD type_;

  DISALLOW_COPY_AND_ASSIGN(RegistryValueIterator);
};

class BASE_EXPORT RegistryKeyIterator {
 public:
  RegistryKeyIterator(HKEY root_key, const wchar_t* folder_key);

  ~RegistryKeyIterator();

  DWORD SubkeyCount() const;

  
  bool Valid() const;

  
  void operator++();

  const wchar_t* Name() const { return name_; }

  int Index() const { return index_; }

 private:
  
  bool Read();

  
  HKEY key_;

  
  int index_;

  wchar_t name_[MAX_PATH];

  DISALLOW_COPY_AND_ASSIGN(RegistryKeyIterator);
};

}  
}  

#endif  

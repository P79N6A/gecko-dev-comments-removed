




#ifndef BASE_REGISTRY_H__
#define BASE_REGISTRY_H__

#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <string>





#define tchar TCHAR
#define CTP const tchar*
#define tstr std::basic_string<tchar>






class RegKey {
 public:
  RegKey(HKEY rootkey = NULL, CTP subkey = NULL, REGSAM access = KEY_READ);
    

  ~RegKey() { this->Close(); }

  bool Create(HKEY rootkey, CTP subkey, REGSAM access = KEY_READ);

  bool CreateWithDisposition(HKEY rootkey, CTP subkey, DWORD* disposition,
                             REGSAM access = KEY_READ);

  bool Open(HKEY rootkey, CTP subkey, REGSAM access = KEY_READ);

  
  bool CreateKey(CTP name, REGSAM access);

  
  bool OpenKey(CTP name, REGSAM access);

  
  void Close();

  DWORD ValueCount();  

  bool ReadName(int index, tstr* name);  

  
  bool Valid() const { return NULL != key_; }

  
  bool DeleteKey(CTP name);

  
  bool DeleteValue(CTP name);

  bool ValueExists(CTP name);
  bool ReadValue(CTP name, void * data, DWORD * dsize, DWORD * dtype = NULL);
  bool ReadValue(CTP name, tstr * value);
  bool ReadValueDW(CTP name, DWORD * value);  

  bool WriteValue(CTP name, const void * data, DWORD dsize,
                  DWORD dtype = REG_BINARY);
  bool WriteValue(CTP name, CTP value);
  bool WriteValue(CTP name, DWORD value);

  
  
  
  
  bool StartWatching();

  
  
  
  
  bool HasChanged();

  
  
  
  bool StopWatching();

  inline bool IsWatching() const { return watch_event_ != 0; }
  HANDLE watch_event() const { return watch_event_; }
  HKEY Handle() const { return key_; }

 private:
  HKEY  key_;    
  HANDLE watch_event_;
};








inline bool AddToRegistry(HKEY root_key, CTP key, CTP value_name,
                          void const * data, DWORD dsize,
                          DWORD dtype = REG_BINARY) {
  return RegKey(root_key, key, KEY_WRITE).WriteValue(value_name, data, dsize,
                                                     dtype);
}


inline bool AddToRegistry(HKEY root_key, CTP key, CTP value_name, CTP value) {
  return AddToRegistry(root_key, key, value_name, value,
                       sizeof(*value) * (lstrlen(value) + 1), REG_SZ);
}



inline bool ReadFromRegistry(HKEY root_key, CTP key, CTP value_name,
                             void* data, DWORD* dsize, DWORD* dtype = NULL) {
  return RegKey(root_key, key).ReadValue(value_name, data, dsize, dtype);
}



inline bool DeleteFromRegistry(HKEY root_key, CTP subkey, CTP value_name) {
  if (value_name)
    return ERROR_SUCCESS == ::SHDeleteValue(root_key, subkey, value_name);
  else
    return ERROR_SUCCESS == ::SHDeleteKey(root_key, subkey);
}




inline bool DeleteKeyFromRegistry(HKEY root_key, CTP key_path, CTP key_name) {
  RegKey key;
  return key.Open(root_key, key_path, KEY_WRITE)
      && key.DeleteKey(key_name);
}






class RegistryValueIterator {
 public:
  
  RegistryValueIterator(HKEY root_key, LPCTSTR folder_key);

  ~RegistryValueIterator();

  DWORD ValueCount() const;  

  bool Valid() const;  

  void operator++();  

  
  
  CTP Name() const { return name_; }
  CTP Value() const { return value_; }
  DWORD ValueSize() const { return value_size_; }
  DWORD Type() const { return type_; }

  int Index() const { return index_; }

 private:
  bool Read();   

  HKEY  key_;    
  int   index_;  

  
  TCHAR name_[MAX_PATH];
  TCHAR value_[MAX_PATH];
  DWORD value_size_;
  DWORD type_;
};


class RegistryKeyIterator {
 public:
  
  RegistryKeyIterator(HKEY root_key, LPCTSTR folder_key);

  ~RegistryKeyIterator();

  DWORD SubkeyCount() const;  

  bool Valid() const;  

  void operator++();  

  
  CTP Name() const { return name_; }

  int Index() const { return index_; }

 private:
  bool Read();   

  HKEY  key_;    
  int   index_;  

  
  TCHAR name_[MAX_PATH];
};



bool RegisterCOMServer(const tchar* guid, const tchar* name,
                       const tchar* modulepath);
bool RegisterCOMServer(const tchar* guid, const tchar* name, HINSTANCE module);
bool UnregisterCOMServer(const tchar* guid);


#undef tchar
#undef CTP
#undef tstr

#endif  

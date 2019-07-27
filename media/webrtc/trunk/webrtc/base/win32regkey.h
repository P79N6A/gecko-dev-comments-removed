


















#ifndef WEBRTC_BASE_WIN32REGKEY_H_
#define WEBRTC_BASE_WIN32REGKEY_H_

#include <string>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/win32.h"

namespace rtc {


const int kMaxKeyNameChars = 255 + 1;
const int kMaxValueNameChars = 16383 + 1;

class RegKey {
 public:
  
  RegKey();

  
  ~RegKey();

  
  HRESULT Create(HKEY parent_key, const wchar_t* key_name);

  HRESULT Create(HKEY parent_key,
                 const wchar_t* key_name,
                 wchar_t* reg_class,
                 DWORD options,
                 REGSAM sam_desired,
                 LPSECURITY_ATTRIBUTES lp_sec_attr,
                 LPDWORD lp_disposition);

  
  HRESULT Open(HKEY parent_key, const wchar_t* key_name);

  HRESULT Open(HKEY parent_key, const wchar_t* key_name, REGSAM sam_desired);

  
  HRESULT Close();

  
  bool HasValue(const wchar_t* value_name) const;

  
  uint32 GetValueCount();

  
  
  
  
  
  
  
  
  HRESULT GetValueNameAt(int index, std::wstring* value_name, DWORD* type);

  
  bool HasSubkey(const wchar_t* key_name) const;

  
  uint32 GetSubkeyCount();

  
  
  
  
  
  
  HRESULT GetSubkeyNameAt(int index, std::wstring* key_name);

  

  
  HRESULT SetValue(const wchar_t* value_name, DWORD value) const;

  
  HRESULT SetValue(const wchar_t* value_name, DWORD64 value) const;

  
  HRESULT SetValue(const wchar_t* value_name, const wchar_t* value) const;

  
  HRESULT SetValue(const wchar_t* value_name,
                   const uint8* value,
                   DWORD byte_count) const;

  
  HRESULT SetValue(const wchar_t* value_name,
                   const uint8* value,
                   DWORD byte_count,
                   DWORD type) const;

  

  
  HRESULT GetValue(const wchar_t* value_name, DWORD* value) const;

  
  HRESULT GetValue(const wchar_t* value_name, DWORD64* value) const;

  
  HRESULT GetValue(const wchar_t* value_name, wchar_t** value) const;

  
  HRESULT GetValue(const wchar_t* value_name, std::wstring* value) const;

  
  HRESULT GetValue(const wchar_t* value_name,
                   std::vector<std::wstring>* value) const;

  
  HRESULT GetValue(const wchar_t* value_name,
                   uint8** value,
                   DWORD* byte_count) const;

  
  HRESULT GetValue(const wchar_t* value_name,
                   uint8** value,
                   DWORD* byte_count,
                   DWORD* type) const;

  

  
  static HRESULT FlushKey(const wchar_t* full_key_name);

  
  static bool HasKey(const wchar_t* full_key_name);

  
  static bool HasValue(const wchar_t* full_key_name, const wchar_t* value_name);

  

  
  static HRESULT SetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          DWORD value);

  
  static HRESULT SetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          DWORD64 value);

  
  static HRESULT SetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          float value);

  
  static HRESULT SetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          double value);

  
  static HRESULT SetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          const wchar_t* value);

  
  static HRESULT SetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          const uint8* value,
                          DWORD byte_count);

  
  static HRESULT SetValueMultiSZ(const wchar_t* full_key_name,
                                 const TCHAR* value_name,
                                 const uint8* value,
                                 DWORD byte_count);

  

  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          DWORD* value);

  
  
  
  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          DWORD64* value);

  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          float* value);

  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          double* value);

  
  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          wchar_t** value);
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          std::wstring* value);

  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          std::vector<std::wstring>* value);

  
  static HRESULT GetValue(const wchar_t* full_key_name,
                          const wchar_t* value_name,
                          uint8** value,
                          DWORD* byte_count);

  
  static HRESULT GetValueType(const wchar_t* full_key_name,
                              const wchar_t* value_name,
                              DWORD* value_type);

  
  HRESULT DeleteSubKey(const wchar_t* key_name);

  
  HRESULT RecurseDeleteSubKey(const wchar_t* key_name);

  
  
  
  
  static HRESULT DeleteKey(const wchar_t* full_key_name);

  
  
  
  
  static HRESULT DeleteKey(const wchar_t* full_key_name, bool recursive);

  
  HRESULT DeleteValue(const wchar_t* value_name);

  
  
  
  static HRESULT DeleteValue(const wchar_t* full_key_name,
                             const wchar_t* value_name);

  
  HKEY key() { return h_key_; }

  
  
  
  
  static HKEY GetRootKeyInfo(std::wstring* full_key_name);

  
  
  static bool SafeKeyNameForDeletion(const wchar_t* key_name);

  
  static HRESULT Save(const wchar_t* full_key_name, const wchar_t* file_name);

  
  
  static HRESULT Restore(const wchar_t* full_key_name,
                         const wchar_t* file_name);

  
  static bool IsKeyEmpty(const wchar_t* full_key_name);

 private:

  
  
  HRESULT GetValueHelper(const wchar_t* value_name,
                         DWORD* type, uint8** value,
                         DWORD* byte_count) const;

  
  
  
  static std::wstring GetParentKeyInfo(std::wstring* key_name);

  
  static HRESULT SetValueStaticHelper(const wchar_t* full_key_name,
                                      const wchar_t* value_name,
                                      DWORD type,
                                      LPVOID value,
                                      DWORD byte_count = 0);

  
  static HRESULT GetValueStaticHelper(const wchar_t* full_key_name,
                                      const wchar_t* value_name,
                                      DWORD type,
                                      LPVOID value,
                                      DWORD* byte_count = NULL);

  
  static HRESULT MultiSZBytesToStringArray(const uint8* buffer,
                                           DWORD byte_count,
                                           std::vector<std::wstring>* value);

  
  HKEY h_key_;

  
  friend void RegKeyHelperFunctionsTest();

  DISALLOW_EVIL_CONSTRUCTORS(RegKey);
};

}  

#endif  

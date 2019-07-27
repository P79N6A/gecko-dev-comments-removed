


















#include "webrtc/base/win32regkey.h"

#include <shlwapi.h>

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/scoped_ptr.h"

namespace rtc {

RegKey::RegKey() {
  h_key_ = NULL;
}

RegKey::~RegKey() {
  Close();
}

HRESULT RegKey::Create(HKEY parent_key, const wchar_t* key_name) {
  return Create(parent_key,
                key_name,
                REG_NONE,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                NULL);
}

HRESULT RegKey::Open(HKEY parent_key, const wchar_t* key_name) {
  return Open(parent_key, key_name, KEY_ALL_ACCESS);
}

bool RegKey::HasValue(const TCHAR* value_name) const {
  return (ERROR_SUCCESS == ::RegQueryValueEx(h_key_, value_name, NULL,
                                             NULL, NULL, NULL));
}

HRESULT RegKey::SetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         DWORD value) {
  ASSERT(full_key_name != NULL);

  return SetValueStaticHelper(full_key_name, value_name, REG_DWORD, &value);
}

HRESULT RegKey::SetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         DWORD64 value) {
  ASSERT(full_key_name != NULL);

  return SetValueStaticHelper(full_key_name, value_name, REG_QWORD, &value);
}

HRESULT RegKey::SetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         float value) {
  ASSERT(full_key_name != NULL);

  return SetValueStaticHelper(full_key_name, value_name,
                              REG_BINARY, &value, sizeof(value));
}

HRESULT RegKey::SetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         double value) {
  ASSERT(full_key_name != NULL);

  return SetValueStaticHelper(full_key_name, value_name,
                              REG_BINARY, &value, sizeof(value));
}

HRESULT RegKey::SetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         const TCHAR* value) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);

  return SetValueStaticHelper(full_key_name, value_name,
                              REG_SZ, const_cast<wchar_t*>(value));
}

HRESULT RegKey::SetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         const uint8* value,
                         DWORD byte_count) {
  ASSERT(full_key_name != NULL);

  return SetValueStaticHelper(full_key_name, value_name, REG_BINARY,
                              const_cast<uint8*>(value), byte_count);
}

HRESULT RegKey::SetValueMultiSZ(const wchar_t* full_key_name,
                                const wchar_t* value_name,
                                const uint8* value,
                                DWORD byte_count) {
  ASSERT(full_key_name != NULL);

  return SetValueStaticHelper(full_key_name, value_name, REG_MULTI_SZ,
                              const_cast<uint8*>(value), byte_count);
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         DWORD* value) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);

  return GetValueStaticHelper(full_key_name, value_name, REG_DWORD, value);
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         DWORD64* value) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);

  return GetValueStaticHelper(full_key_name, value_name, REG_QWORD, value);
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         float* value) {
  ASSERT(value != NULL);
  ASSERT(full_key_name != NULL);

  DWORD byte_count = 0;
  scoped_ptr<byte[]> buffer;
  HRESULT hr = GetValueStaticHelper(full_key_name, value_name,
                                    REG_BINARY, buffer.accept(), &byte_count);
  if (SUCCEEDED(hr)) {
    ASSERT(byte_count == sizeof(*value));
    if (byte_count == sizeof(*value)) {
      *value = *reinterpret_cast<float*>(buffer.get());
    }
  }
  return hr;
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         double* value) {
  ASSERT(value != NULL);
  ASSERT(full_key_name != NULL);

  DWORD byte_count = 0;
  scoped_ptr<byte[]> buffer;
  HRESULT hr = GetValueStaticHelper(full_key_name, value_name,
                                    REG_BINARY, buffer.accept(), &byte_count);
  if (SUCCEEDED(hr)) {
    ASSERT(byte_count == sizeof(*value));
    if (byte_count == sizeof(*value)) {
      *value = *reinterpret_cast<double*>(buffer.get());
    }
  }
  return hr;
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         wchar_t** value) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);

  return GetValueStaticHelper(full_key_name, value_name, REG_SZ, value);
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         std::wstring* value) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);

  scoped_ptr<wchar_t[]> buffer;
  HRESULT hr = RegKey::GetValue(full_key_name, value_name, buffer.accept());
  if (SUCCEEDED(hr)) {
    value->assign(buffer.get());
  }
  return hr;
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         std::vector<std::wstring>* value) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);

  return GetValueStaticHelper(full_key_name, value_name, REG_MULTI_SZ, value);
}

HRESULT RegKey::GetValue(const wchar_t* full_key_name,
                         const wchar_t* value_name,
                         uint8** value,
                         DWORD* byte_count) {
  ASSERT(full_key_name != NULL);
  ASSERT(value != NULL);
  ASSERT(byte_count != NULL);

  return GetValueStaticHelper(full_key_name, value_name,
                              REG_BINARY, value, byte_count);
}

HRESULT RegKey::DeleteSubKey(const wchar_t* key_name) {
  ASSERT(key_name != NULL);
  ASSERT(h_key_ != NULL);

  LONG res = ::RegDeleteKey(h_key_, key_name);
  HRESULT hr = HRESULT_FROM_WIN32(res);
  if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
      hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
    hr = S_FALSE;
  }
  return hr;
}

HRESULT RegKey::DeleteValue(const wchar_t* value_name) {
  ASSERT(h_key_ != NULL);

  LONG res = ::RegDeleteValue(h_key_, value_name);
  HRESULT hr = HRESULT_FROM_WIN32(res);
  if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
      hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
    hr = S_FALSE;
  }
  return hr;
}

HRESULT RegKey::Close() {
  HRESULT hr = S_OK;
  if (h_key_ != NULL) {
    LONG res = ::RegCloseKey(h_key_);
    hr = HRESULT_FROM_WIN32(res);
    h_key_ = NULL;
  }
  return hr;
}

HRESULT RegKey::Create(HKEY parent_key,
                       const wchar_t* key_name,
                       wchar_t* lpszClass,
                       DWORD options,
                       REGSAM sam_desired,
                       LPSECURITY_ATTRIBUTES lpSecAttr,
                       LPDWORD lpdwDisposition) {
  ASSERT(key_name != NULL);
  ASSERT(parent_key != NULL);

  DWORD dw = 0;
  HKEY h_key = NULL;
  LONG res = ::RegCreateKeyEx(parent_key, key_name, 0, lpszClass, options,
                              sam_desired, lpSecAttr, &h_key, &dw);
  HRESULT hr = HRESULT_FROM_WIN32(res);

  if (lpdwDisposition) {
    *lpdwDisposition = dw;
  }

  
  
  if (hr == S_OK) {
    hr = Close();
    ASSERT(hr == S_OK);
    h_key_ = h_key;
  }
  return hr;
}

HRESULT RegKey::Open(HKEY parent_key,
                     const wchar_t* key_name,
                     REGSAM sam_desired) {
  ASSERT(key_name != NULL);
  ASSERT(parent_key != NULL);

  HKEY h_key = NULL;
  LONG res = ::RegOpenKeyEx(parent_key, key_name, 0, sam_desired, &h_key);
  HRESULT hr = HRESULT_FROM_WIN32(res);

  
  
  if (hr == S_OK) {
    
    hr = Close();
    ASSERT(hr == S_OK);
    h_key_ = h_key;
  }
  return hr;
}


HRESULT RegKey::Save(const wchar_t* full_key_name, const wchar_t* file_name) {
  ASSERT(full_key_name != NULL);
  ASSERT(file_name != NULL);

  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);
  if (!h_key) {
    return E_FAIL;
  }

  RegKey key;
  HRESULT hr = key.Open(h_key, key_name.c_str(), KEY_READ);
  if (FAILED(hr)) {
    return hr;
  }

  AdjustCurrentProcessPrivilege(SE_BACKUP_NAME, true);
  LONG res = ::RegSaveKey(key.h_key_, file_name, NULL);
  AdjustCurrentProcessPrivilege(SE_BACKUP_NAME, false);

  return HRESULT_FROM_WIN32(res);
}


HRESULT RegKey::Restore(const wchar_t* full_key_name,
                        const wchar_t* file_name) {
  ASSERT(full_key_name != NULL);
  ASSERT(file_name != NULL);

  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);
  if (!h_key) {
    return E_FAIL;
  }

  RegKey key;
  HRESULT hr = key.Open(h_key, key_name.c_str(), KEY_WRITE);
  if (FAILED(hr)) {
    return hr;
  }

  AdjustCurrentProcessPrivilege(SE_RESTORE_NAME, true);
  LONG res = ::RegRestoreKey(key.h_key_, file_name, REG_FORCE_RESTORE);
  AdjustCurrentProcessPrivilege(SE_RESTORE_NAME, false);

  return HRESULT_FROM_WIN32(res);
}


bool RegKey::HasSubkey(const wchar_t* key_name) const {
  ASSERT(key_name != NULL);

  RegKey key;
  HRESULT hr = key.Open(h_key_, key_name, KEY_READ);
  key.Close();
  return hr == S_OK;
}


HRESULT RegKey::FlushKey(const wchar_t* full_key_name) {
  ASSERT(full_key_name != NULL);

  HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  if (h_key != NULL) {
    LONG res = ::RegFlushKey(h_key);
    hr = HRESULT_FROM_WIN32(res);
  }
  return hr;
}


HRESULT RegKey::SetValueStaticHelper(const wchar_t* full_key_name,
                                     const wchar_t* value_name,
                                     DWORD type,
                                     LPVOID value,
                                     DWORD byte_count) {
  ASSERT(full_key_name != NULL);

  HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  if (h_key != NULL) {
    RegKey key;
    hr = key.Create(h_key, key_name.c_str());
    if (hr == S_OK) {
      switch (type) {
        case REG_DWORD:
          hr = key.SetValue(value_name, *(static_cast<DWORD*>(value)));
          break;
        case REG_QWORD:
          hr = key.SetValue(value_name, *(static_cast<DWORD64*>(value)));
          break;
        case REG_SZ:
          hr = key.SetValue(value_name, static_cast<const wchar_t*>(value));
          break;
        case REG_BINARY:
          hr = key.SetValue(value_name, static_cast<const uint8*>(value),
                            byte_count);
          break;
        case REG_MULTI_SZ:
          hr = key.SetValue(value_name, static_cast<const uint8*>(value),
                            byte_count, type);
          break;
        default:
          ASSERT(false);
          hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
          break;
      }
      
      HRESULT temp_hr = key.Close();
      if (hr == S_OK) {
        hr = temp_hr;
      }
    }
  }
  return hr;
}


HRESULT RegKey::GetValueStaticHelper(const wchar_t* full_key_name,
                                     const wchar_t* value_name,
                                     DWORD type,
                                     LPVOID value,
                                     DWORD* byte_count) {
  ASSERT(full_key_name != NULL);

  HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  if (h_key != NULL) {
    RegKey key;
    hr = key.Open(h_key, key_name.c_str(), KEY_READ);
    if (hr == S_OK) {
      switch (type) {
        case REG_DWORD:
          hr = key.GetValue(value_name, reinterpret_cast<DWORD*>(value));
          break;
        case REG_QWORD:
          hr = key.GetValue(value_name, reinterpret_cast<DWORD64*>(value));
          break;
        case REG_SZ:
          hr = key.GetValue(value_name, reinterpret_cast<wchar_t**>(value));
          break;
        case REG_MULTI_SZ:
          hr = key.GetValue(value_name, reinterpret_cast<
                                            std::vector<std::wstring>*>(value));
          break;
        case REG_BINARY:
          hr = key.GetValue(value_name, reinterpret_cast<uint8**>(value),
                            byte_count);
          break;
        default:
          ASSERT(false);
          hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
          break;
      }
      
      HRESULT temp_hr = key.Close();
      if (hr == S_OK) {
        hr = temp_hr;
      }
    }
  }
  return hr;
}


HRESULT RegKey::GetValueHelper(const wchar_t* value_name,
                               DWORD* type,
                               uint8** value,
                               DWORD* byte_count) const {
  ASSERT(byte_count != NULL);
  ASSERT(value != NULL);
  ASSERT(type != NULL);

  
  *value = NULL;

  
  LONG res = ::SHQueryValueEx(h_key_, value_name, NULL, type, NULL, byte_count);
  HRESULT hr = HRESULT_FROM_WIN32(res);

  if (hr == S_OK) {
    
    if (*byte_count != 0) {
      
      *value = new byte[*byte_count];
      ASSERT(*value != NULL);

      
      res = ::SHQueryValueEx(h_key_, value_name, NULL,
                             type, *value, byte_count);
      hr = HRESULT_FROM_WIN32(res);
      ASSERT(hr == S_OK);
    }
  }
  return hr;
}


HRESULT RegKey::GetValue(const wchar_t* value_name, DWORD* value) const {
  ASSERT(value != NULL);

  DWORD type = 0;
  DWORD byte_count = sizeof(DWORD);
  LONG res = ::SHQueryValueEx(h_key_, value_name, NULL, &type,
                              value, &byte_count);
  HRESULT hr = HRESULT_FROM_WIN32(res);
  ASSERT((hr != S_OK) || (type == REG_DWORD));
  ASSERT((hr != S_OK) || (byte_count == sizeof(DWORD)));
  return hr;
}


HRESULT RegKey::GetValue(const wchar_t* value_name, DWORD64* value) const {
  ASSERT(value != NULL);

  DWORD type = 0;
  DWORD byte_count = sizeof(DWORD64);
  LONG res = ::SHQueryValueEx(h_key_, value_name, NULL, &type,
                              value, &byte_count);
  HRESULT hr = HRESULT_FROM_WIN32(res);
  ASSERT((hr != S_OK) || (type == REG_QWORD));
  ASSERT((hr != S_OK) || (byte_count == sizeof(DWORD64)));
  return hr;
}


HRESULT RegKey::GetValue(const wchar_t* value_name, wchar_t** value) const {
  ASSERT(value != NULL);

  DWORD byte_count = 0;
  DWORD type = 0;

  
  LONG res = ::SHQueryValueEx(h_key_, value_name, NULL,
                              &type, NULL, &byte_count);
  HRESULT hr = HRESULT_FROM_WIN32(res);

  if (hr == S_OK) {
    
    *value = new wchar_t[(byte_count / sizeof(wchar_t)) + 1];

    if ((*value) != NULL) {
      if (byte_count != 0) {
        
        res = ::SHQueryValueEx(h_key_, value_name, NULL, &type,
                               *value, &byte_count);
        hr = HRESULT_FROM_WIN32(res);
      } else {
        (*value)[0] = L'\0';
      }

      ASSERT((hr != S_OK) || (type == REG_SZ) ||
             (type == REG_MULTI_SZ) || (type == REG_EXPAND_SZ));
    } else {
      hr = E_OUTOFMEMORY;
    }
  }

  return hr;
}


HRESULT RegKey::GetValue(const wchar_t* value_name, std::wstring* value) const {
  ASSERT(value != NULL);

  DWORD byte_count = 0;
  DWORD type = 0;

  
  LONG res = ::SHQueryValueEx(h_key_, value_name, NULL,
                              &type, NULL, &byte_count);
  HRESULT hr = HRESULT_FROM_WIN32(res);

  if (hr == S_OK) {
    if (byte_count != 0) {
      
      value->resize(byte_count / sizeof(wchar_t) + 1);
      res = ::SHQueryValueEx(h_key_, value_name, NULL, &type,
                             &value->at(0), &byte_count);
      hr = HRESULT_FROM_WIN32(res);
      value->resize(wcslen(value->data()));
    } else {
      value->clear();
    }

    ASSERT((hr != S_OK) || (type == REG_SZ) ||
           (type == REG_MULTI_SZ) || (type == REG_EXPAND_SZ));
  }

  return hr;
}


HRESULT RegKey::MultiSZBytesToStringArray(const uint8* buffer,
                                          DWORD byte_count,
                                          std::vector<std::wstring>* value) {
  ASSERT(buffer != NULL);
  ASSERT(value != NULL);

  const wchar_t* data = reinterpret_cast<const wchar_t*>(buffer);
  DWORD data_len = byte_count / sizeof(wchar_t);
  value->clear();
  if (data_len > 1) {
    
    if (data[data_len - 1] != 0 || data[data_len - 2] != 0) {
      return E_INVALIDARG;
    }

    
    while (*data) {
      std::wstring str(data);
      value->push_back(str);
      data += str.length() + 1;
    }
  }
  return S_OK;
}


HRESULT RegKey::GetValue(const wchar_t* value_name,
                         std::vector<std::wstring>* value) const {
  ASSERT(value != NULL);

  DWORD byte_count = 0;
  DWORD type = 0;
  uint8* buffer = 0;

  
  HRESULT hr = GetValueHelper(value_name, &type, &buffer, &byte_count);
  ASSERT((hr != S_OK) || (type == REG_MULTI_SZ));

  if (SUCCEEDED(hr)) {
    hr = MultiSZBytesToStringArray(buffer, byte_count, value);
  }

  return hr;
}


HRESULT RegKey::GetValue(const wchar_t* value_name,
                         uint8** value,
                         DWORD* byte_count) const {
  ASSERT(byte_count != NULL);
  ASSERT(value != NULL);

  DWORD type = 0;
  HRESULT hr = GetValueHelper(value_name, &type, value, byte_count);
  ASSERT((hr != S_OK) || (type == REG_MULTI_SZ) || (type == REG_BINARY));
  return hr;
}


HRESULT RegKey::GetValue(const wchar_t* value_name,
                         uint8** value,
                         DWORD* byte_count,
                         DWORD*type) const {
  ASSERT(type != NULL);
  ASSERT(byte_count != NULL);
  ASSERT(value != NULL);

  return GetValueHelper(value_name, type, value, byte_count);
}


HRESULT RegKey::SetValue(const wchar_t* value_name, DWORD value) const {
  ASSERT(h_key_ != NULL);

  LONG res = ::RegSetValueEx(h_key_, value_name, NULL, REG_DWORD,
                             reinterpret_cast<const uint8*>(&value),
                             sizeof(DWORD));
  return HRESULT_FROM_WIN32(res);
}


HRESULT RegKey::SetValue(const wchar_t* value_name, DWORD64 value) const {
  ASSERT(h_key_ != NULL);

  LONG res = ::RegSetValueEx(h_key_, value_name, NULL, REG_QWORD,
                             reinterpret_cast<const uint8*>(&value),
                             sizeof(DWORD64));
  return HRESULT_FROM_WIN32(res);
}


HRESULT RegKey::SetValue(const wchar_t* value_name,
                         const wchar_t* value) const {
  ASSERT(value != NULL);
  ASSERT(h_key_ != NULL);

  LONG res = ::RegSetValueEx(h_key_, value_name, NULL, REG_SZ,
                             reinterpret_cast<const uint8*>(value),
                             (lstrlen(value) + 1) * sizeof(wchar_t));
  return HRESULT_FROM_WIN32(res);
}


HRESULT RegKey::SetValue(const wchar_t* value_name,
                         const uint8* value,
                         DWORD byte_count) const {
  ASSERT(h_key_ != NULL);

  
  if (value == NULL) {
    byte_count = 0;
  }

  LONG res = ::RegSetValueEx(h_key_, value_name, NULL,
                             REG_BINARY, value, byte_count);
  return HRESULT_FROM_WIN32(res);
}


HRESULT RegKey::SetValue(const wchar_t* value_name,
                         const uint8* value,
                         DWORD byte_count,
                         DWORD type) const {
  ASSERT(value != NULL);
  ASSERT(h_key_ != NULL);

  LONG res = ::RegSetValueEx(h_key_, value_name, NULL, type, value, byte_count);
  return HRESULT_FROM_WIN32(res);
}

bool RegKey::HasKey(const wchar_t* full_key_name) {
  ASSERT(full_key_name != NULL);

  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  if (h_key != NULL) {
    RegKey key;
    HRESULT hr = key.Open(h_key, key_name.c_str(), KEY_READ);
    key.Close();
    return S_OK == hr;
  }
  return false;
}


bool RegKey::HasValue(const wchar_t* full_key_name, const wchar_t* value_name) {
  ASSERT(full_key_name != NULL);

  bool has_value = false;
  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  if (h_key != NULL) {
    RegKey key;
    if (key.Open(h_key, key_name.c_str(), KEY_READ) == S_OK) {
      has_value = key.HasValue(value_name);
      key.Close();
    }
  }
  return has_value;
}

HRESULT RegKey::GetValueType(const wchar_t* full_key_name,
                             const wchar_t* value_name,
                             DWORD* value_type) {
  ASSERT(full_key_name != NULL);
  ASSERT(value_type != NULL);

  *value_type = REG_NONE;

  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  RegKey key;
  HRESULT hr = key.Open(h_key, key_name.c_str(), KEY_READ);
  if (SUCCEEDED(hr)) {
    LONG res = ::SHQueryValueEx(key.h_key_, value_name, NULL, value_type,
                                NULL, NULL);
    if (res != ERROR_SUCCESS) {
      hr = HRESULT_FROM_WIN32(res);
    }
  }

  return hr;
}

HRESULT RegKey::DeleteKey(const wchar_t* full_key_name) {
  ASSERT(full_key_name != NULL);

  return DeleteKey(full_key_name, true);
}

HRESULT RegKey::DeleteKey(const wchar_t* full_key_name, bool recursively) {
  ASSERT(full_key_name != NULL);

  
  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  
  std::wstring parent_key(GetParentKeyInfo(&key_name));

  RegKey key;
  HRESULT hr = key.Open(h_key, parent_key.c_str());

  if (hr == S_OK) {
    hr = recursively ? key.RecurseDeleteSubKey(key_name.c_str())
                     : key.DeleteSubKey(key_name.c_str());
  } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
             hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
    hr = S_FALSE;
  }

  key.Close();
  return hr;
}

HRESULT RegKey::DeleteValue(const wchar_t* full_key_name,
                            const wchar_t* value_name) {
  ASSERT(full_key_name != NULL);

  HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  if (h_key != NULL) {
    RegKey key;
    hr = key.Open(h_key, key_name.c_str());
    if (hr == S_OK) {
      hr = key.DeleteValue(value_name);
      key.Close();
    }
  }
  return hr;
}

HRESULT RegKey::RecurseDeleteSubKey(const wchar_t* key_name) {
  ASSERT(key_name != NULL);

  RegKey key;
  HRESULT hr = key.Open(h_key_, key_name);

  if (hr == S_OK) {
    
    FILETIME time = {0};
    wchar_t key_name_buf[kMaxKeyNameChars] = {0};
    DWORD key_name_buf_size = kMaxKeyNameChars;
    while (hr == S_OK &&
        ::RegEnumKeyEx(key.h_key_, 0, key_name_buf, &key_name_buf_size,
                       NULL, NULL, NULL,  &time) == ERROR_SUCCESS) {
      hr = key.RecurseDeleteSubKey(key_name_buf);

      
      key_name_buf_size = kMaxKeyNameChars;
    }
    
    key.Close();
  }

  if (hr == S_OK) {
    
    
    hr = DeleteSubKey(key_name);
  }

  return hr;
}

HKEY RegKey::GetRootKeyInfo(std::wstring* full_key_name) {
  ASSERT(full_key_name != NULL);

  HKEY h_key = NULL;
  
  size_t index = full_key_name->find(L'\\');
  std::wstring root_key;

  if (index == -1) {
    root_key = *full_key_name;
    *full_key_name = L"";
  } else {
    root_key = full_key_name->substr(0, index);
    *full_key_name = full_key_name->substr(index + 1,
                                           full_key_name->length() - index - 1);
  }

  for (std::wstring::iterator iter = root_key.begin();
       iter != root_key.end(); ++iter) {
    *iter = toupper(*iter);
  }

  if (!root_key.compare(L"HKLM") ||
      !root_key.compare(L"HKEY_LOCAL_MACHINE")) {
    h_key = HKEY_LOCAL_MACHINE;
  } else if (!root_key.compare(L"HKCU") ||
             !root_key.compare(L"HKEY_CURRENT_USER")) {
    h_key = HKEY_CURRENT_USER;
  } else if (!root_key.compare(L"HKU") ||
             !root_key.compare(L"HKEY_USERS")) {
    h_key = HKEY_USERS;
  } else if (!root_key.compare(L"HKCR") ||
             !root_key.compare(L"HKEY_CLASSES_ROOT")) {
    h_key = HKEY_CLASSES_ROOT;
  }

  return h_key;
}




bool RegKey::SafeKeyNameForDeletion(const wchar_t* key_name) {
  ASSERT(key_name != NULL);
  std::wstring key(key_name);

  HKEY root_key = GetRootKeyInfo(&key);

  if (!root_key) {
    key = key_name;
  }
  if (key.empty()) {
    return false;
  }
  bool found_subkey = false, backslash_found = false;
  for (size_t i = 0 ; i < key.length() ; ++i) {
    if (key[i] == L'\\') {
      backslash_found = true;
    } else if (backslash_found) {
      found_subkey = true;
      break;
    }
  }
  return (root_key == HKEY_USERS) ? found_subkey : true;
}

std::wstring RegKey::GetParentKeyInfo(std::wstring* key_name) {
  ASSERT(key_name != NULL);

  
  size_t index = key_name->rfind(L'\\');
  std::wstring parent_key;
  if (index == -1) {
    parent_key = L"";
  } else {
    parent_key = key_name->substr(0, index);
    *key_name = key_name->substr(index + 1, key_name->length() - index - 1);
  }

  return parent_key;
}


uint32 RegKey::GetValueCount() {
  DWORD num_values = 0;

  if (ERROR_SUCCESS != ::RegQueryInfoKey(
        h_key_,  
        NULL,  
        NULL,  
        NULL,  
        NULL,  
        NULL,  
        NULL,  
        &num_values,  
        NULL,  
        NULL,  
        NULL,  
        NULL)) {  
    ASSERT(false);
  }
  return num_values;
}






HRESULT RegKey::GetValueNameAt(int index, std::wstring* value_name,
                               DWORD* type) {
  ASSERT(value_name != NULL);

  LONG res = ERROR_SUCCESS;
  wchar_t value_name_buf[kMaxValueNameChars] = {0};
  DWORD value_name_buf_size = kMaxValueNameChars;
  res = ::RegEnumValue(h_key_, index, value_name_buf, &value_name_buf_size,
                       NULL, type, NULL, NULL);

  if (res == ERROR_SUCCESS) {
    value_name->assign(value_name_buf);
  }

  return HRESULT_FROM_WIN32(res);
}

uint32 RegKey::GetSubkeyCount() {
  
  DWORD num_subkeys = 0;

  if (ERROR_SUCCESS != ::RegQueryInfoKey(
          h_key_,  
          NULL,  
          NULL,  
          NULL,  
          &num_subkeys,  
          NULL,  
          NULL,  
          NULL,  
          NULL,  
          NULL,  
          NULL,  
          NULL)) { 
    ASSERT(false);
  }
  return num_subkeys;
}

HRESULT RegKey::GetSubkeyNameAt(int index, std::wstring* key_name) {
  ASSERT(key_name != NULL);

  LONG res = ERROR_SUCCESS;
  wchar_t key_name_buf[kMaxKeyNameChars] = {0};
  DWORD key_name_buf_size = kMaxKeyNameChars;

  res = ::RegEnumKeyEx(h_key_, index, key_name_buf, &key_name_buf_size,
                       NULL, NULL, NULL, NULL);

  if (res == ERROR_SUCCESS) {
    key_name->assign(key_name_buf);
  }

  return HRESULT_FROM_WIN32(res);
}


bool RegKey::IsKeyEmpty(const wchar_t* full_key_name) {
  ASSERT(full_key_name != NULL);

  bool is_empty = true;

  
  std::wstring key_name(full_key_name);
  HKEY h_key = GetRootKeyInfo(&key_name);

  
  if (h_key != NULL) {
    RegKey key;
    HRESULT hr = key.Open(h_key, key_name.c_str(), KEY_READ);
    if (SUCCEEDED(hr)) {
      is_empty = key.GetSubkeyCount() == 0 && key.GetValueCount() == 0;
      key.Close();
    }
  }

  return is_empty;
}

bool AdjustCurrentProcessPrivilege(const TCHAR* privilege, bool to_enable) {
  ASSERT(privilege != NULL);

  bool ret = false;
  HANDLE token;
  if (::OpenProcessToken(::GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
    LUID luid;
    memset(&luid, 0, sizeof(luid));
    if (::LookupPrivilegeValue(NULL, privilege, &luid)) {
      TOKEN_PRIVILEGES privs;
      privs.PrivilegeCount = 1;
      privs.Privileges[0].Luid = luid;
      privs.Privileges[0].Attributes = to_enable ? SE_PRIVILEGE_ENABLED : 0;
      if (::AdjustTokenPrivileges(token, FALSE, &privs, 0, NULL, 0)) {
        ret = true;
      } else {
        LOG_GLE(LS_ERROR) << "AdjustTokenPrivileges failed";
      }
    } else {
      LOG_GLE(LS_ERROR) << "LookupPrivilegeValue failed";
    }
    CloseHandle(token);
  } else {
    LOG_GLE(LS_ERROR) << "OpenProcessToken(GetCurrentProcess) failed";
  }

  return ret;
}

}  

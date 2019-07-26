



#include "sandbox/win/src/win_utils.h"

#include <map>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/internal_types.h"
#include "sandbox/win/src/nt_internals.h"

namespace {


struct KnownReservedKey {
  const wchar_t* name;
  HKEY key;
};


const KnownReservedKey kKnownKey[] = {
    { L"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT },
    { L"HKEY_CURRENT_USER", HKEY_CURRENT_USER },
    { L"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE},
    { L"HKEY_USERS", HKEY_USERS},
    { L"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA},
    { L"HKEY_PERFORMANCE_TEXT", HKEY_PERFORMANCE_TEXT},
    { L"HKEY_PERFORMANCE_NLSTEXT", HKEY_PERFORMANCE_NLSTEXT},
    { L"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG},
    { L"HKEY_DYN_DATA", HKEY_DYN_DATA}
};


bool IsPipe(const std::wstring& path) {
  size_t start = 0;
  if (0 == path.compare(0, sandbox::kNTPrefixLen, sandbox::kNTPrefix))
    start = sandbox::kNTPrefixLen;

  const wchar_t kPipe[] = L"pipe\\";
  return (0 == path.compare(start, arraysize(kPipe) - 1, kPipe));
}

}  

namespace sandbox {

HKEY GetReservedKeyFromName(const std::wstring& name) {
  for (size_t i = 0; i < arraysize(kKnownKey); ++i) {
    if (name == kKnownKey[i].name)
      return kKnownKey[i].key;
  }

  return NULL;
}

bool ResolveRegistryName(std::wstring name, std::wstring* resolved_name) {
  for (size_t i = 0; i < arraysize(kKnownKey); ++i) {
    if (name.find(kKnownKey[i].name) == 0) {
      HKEY key;
      DWORD disposition;
      if (ERROR_SUCCESS != ::RegCreateKeyEx(kKnownKey[i].key, L"", 0, NULL, 0,
                                            MAXIMUM_ALLOWED, NULL, &key,
                                            &disposition))
        return false;

      bool result = GetPathFromHandle(key, resolved_name);
      ::RegCloseKey(key);

      if (!result)
        return false;

      *resolved_name += name.substr(wcslen(kKnownKey[i].name));
      return true;
    }
  }

  return false;
}

DWORD IsReparsePoint(const std::wstring& full_path, bool* result) {
  std::wstring path = full_path;

  
  if (0 == path.compare(0, kNTPrefixLen, kNTPrefix))
    path = path.substr(kNTPrefixLen);

  
  if (IsPipe(path)) {
    *result = FALSE;
    return ERROR_SUCCESS;
  }

  std::wstring::size_type last_pos = std::wstring::npos;

  do {
    path = path.substr(0, last_pos);

    DWORD attributes = ::GetFileAttributes(path.c_str());
    if (INVALID_FILE_ATTRIBUTES == attributes) {
      DWORD error = ::GetLastError();
      if (error != ERROR_FILE_NOT_FOUND &&
          error != ERROR_PATH_NOT_FOUND &&
          error != ERROR_INVALID_NAME) {
        
        NOTREACHED();
        return error;
      }
    } else if (FILE_ATTRIBUTE_REPARSE_POINT & attributes) {
      
      *result = true;
      return ERROR_SUCCESS;
    }

    last_pos = path.rfind(L'\\');
  } while (last_pos != std::wstring::npos);

  *result = false;
  return ERROR_SUCCESS;
}



bool SameObject(HANDLE handle, const wchar_t* full_path) {
  std::wstring path(full_path);
  DCHECK(!path.empty());

  
  if (IsPipe(path))
    return true;

  std::wstring actual_path;
  if (!GetPathFromHandle(handle, &actual_path))
    return false;

  
  const wchar_t kBackslash = '\\';
  if (path[path.length() - 1] == kBackslash)
    path = path.substr(0, path.length() - 1);

  
  if (0 == _wcsicmp(actual_path.c_str(), path.c_str()))
    return true;

  
  size_t colon_pos = path.find(L':');
  if (colon_pos == 0 || colon_pos == std::wstring::npos)
    return false;

  
  if (colon_pos > 1 && path[colon_pos - 2] != kBackslash)
    return false;

  
  wchar_t drive[4] = {0};
  wchar_t vol_name[MAX_PATH];
  memcpy(drive, &path[colon_pos - 1], 2 * sizeof(*drive));

  
  DWORD vol_length = ::QueryDosDeviceW(drive, vol_name, MAX_PATH);
  if (vol_length < 2 || vol_length == MAX_PATH)
    return false;

  
  vol_length = static_cast<DWORD>(wcslen(vol_name));

  
  if (vol_length + path.size() - (colon_pos + 1) != actual_path.size())
    return false;

  
  if (0 != _wcsnicmp(actual_path.c_str(), vol_name, vol_length))
    return false;

  
  if (0 != _wcsicmp(&actual_path[vol_length], &path[colon_pos + 1]))
    return false;

  return true;
}

bool ConvertToLongPath(const std::wstring& short_path,
                       std::wstring* long_path) {
  
  bool is_nt_path = false;
  std::wstring path = short_path;
  if (0 == path.compare(0, kNTPrefixLen, kNTPrefix)) {
    path = path.substr(kNTPrefixLen);
    is_nt_path = true;
  }

  DWORD size = MAX_PATH;
  scoped_ptr<wchar_t[]> long_path_buf(new wchar_t[size]);

  DWORD return_value = ::GetLongPathName(path.c_str(), long_path_buf.get(),
                                         size);
  while (return_value >= size) {
    size *= 2;
    long_path_buf.reset(new wchar_t[size]);
    return_value = ::GetLongPathName(path.c_str(), long_path_buf.get(), size);
  }

  DWORD last_error = ::GetLastError();
  if (0 == return_value && (ERROR_FILE_NOT_FOUND == last_error ||
                            ERROR_PATH_NOT_FOUND == last_error ||
                            ERROR_INVALID_NAME == last_error)) {
    
    std::wstring::size_type last_slash = path.rfind(L'\\');
    if (std::wstring::npos == last_slash)
      return false;

    std::wstring begin = path.substr(0, last_slash);
    std::wstring end = path.substr(last_slash);
    if (!ConvertToLongPath(begin, &begin))
      return false;

    
    path = begin + end;
    return_value = 1;
  } else if (0 != return_value) {
    path = long_path_buf.get();
  }

  if (return_value != 0) {
    if (is_nt_path) {
      *long_path = kNTPrefix;
      *long_path += path;
    } else {
      *long_path = path;
    }

    return true;
  }

  return false;
}

bool GetPathFromHandle(HANDLE handle, std::wstring* path) {
  NtQueryObjectFunction NtQueryObject = NULL;
  ResolveNTFunctionPtr("NtQueryObject", &NtQueryObject);

  OBJECT_NAME_INFORMATION initial_buffer;
  OBJECT_NAME_INFORMATION* name = &initial_buffer;
  ULONG size = sizeof(initial_buffer);
  
  NTSTATUS status = NtQueryObject(handle, ObjectNameInformation, name, size,
                                  &size);

  scoped_ptr<OBJECT_NAME_INFORMATION> name_ptr;
  if (size) {
    name = reinterpret_cast<OBJECT_NAME_INFORMATION*>(new BYTE[size]);
    name_ptr.reset(name);

    
    
    status = NtQueryObject(handle, ObjectNameInformation, name, size, &size);
  }

  if (STATUS_SUCCESS != status)
    return false;

  path->assign(name->ObjectName.Buffer, name->ObjectName.Length /
                                        sizeof(name->ObjectName.Buffer[0]));
  return true;
}

bool GetNtPathFromWin32Path(const std::wstring& path, std::wstring* nt_path) {
  HANDLE file = ::CreateFileW(path.c_str(), 0,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return false;
  bool rv = GetPathFromHandle(file, nt_path);
  ::CloseHandle(file);
  return rv;
}

bool WriteProtectedChildMemory(HANDLE child_process, void* address,
                               const void* buffer, size_t length) {
  
  DWORD old_protection;
  if (!::VirtualProtectEx(child_process, address, length,
                          PAGE_WRITECOPY, &old_protection))
    return false;

  SIZE_T written;
  bool ok = ::WriteProcessMemory(child_process, address, buffer, length,
                                 &written) && (length == written);

  
  if (!::VirtualProtectEx(child_process, address, length,
                          old_protection, &old_protection))
    return false;

  return ok;
}

};  






void ResolveNTFunctionPtr(const char* name, void* ptr) {
  const int max_tries = 5;
  const int sleep_threshold = 2;

  static HMODULE ntdll = ::GetModuleHandle(sandbox::kNtdllName);

  FARPROC* function_ptr = reinterpret_cast<FARPROC*>(ptr);
  *function_ptr = ::GetProcAddress(ntdll, name);

  for (int tries = 1; !(*function_ptr) && tries < max_tries; ++tries) {
    if (tries >= sleep_threshold)
      ::Sleep(1);
    ntdll = ::GetModuleHandle(sandbox::kNtdllName);
    *function_ptr = ::GetProcAddress(ntdll, name);
  }

  CHECK(*function_ptr);
}

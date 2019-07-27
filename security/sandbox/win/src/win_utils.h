



#ifndef SANDBOX_SRC_WIN_UTILS_H_
#define SANDBOX_SRC_WIN_UTILS_H_

#include <windows.h>
#include <string>
#include "base/basictypes.h"

namespace sandbox {


const wchar_t kNTPrefix[] = L"\\??\\";
const size_t kNTPrefixLen = arraysize(kNTPrefix) - 1;

const wchar_t kNTObjManPrefix[] = L"\\Device\\";
const size_t kNTObjManPrefixLen = arraysize(kNTObjManPrefix) - 1;



class AutoLock {
 public:
  
  explicit AutoLock(CRITICAL_SECTION *lock) : lock_(lock) {
    ::EnterCriticalSection(lock);
  };

  
  ~AutoLock() {
    ::LeaveCriticalSection(lock_);
  };

 private:
  CRITICAL_SECTION *lock_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(AutoLock);
};



template <typename Derived>
class SingletonBase {
 public:
  static Derived* GetInstance() {
    static Derived* instance = NULL;
    if (NULL == instance) {
      instance = new Derived();
      
      
      _onexit(OnExit);
    }
    return instance;
  }

 private:
  
  
  static int __cdecl OnExit() {
    delete GetInstance();
    return 0;
  }
};




bool ConvertToLongPath(const std::wstring& short_path, std::wstring* long_path);






DWORD IsReparsePoint(const std::wstring& full_path, bool* result);


bool SameObject(HANDLE handle, const wchar_t* full_path);


bool GetPathFromHandle(HANDLE handle, std::wstring* path);



bool GetNtPathFromWin32Path(const std::wstring& path, std::wstring* nt_path);




HKEY GetReservedKeyFromName(const std::wstring& name);





bool ResolveRegistryName(std::wstring name, std::wstring* resolved_name);




bool WriteProtectedChildMemory(HANDLE child_process, void* address,
                               const void* buffer, size_t length);

}  



void ResolveNTFunctionPtr(const char* name, void* ptr);

#endif  

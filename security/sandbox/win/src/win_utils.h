



#ifndef SANDBOX_SRC_WIN_UTILS_H_
#define SANDBOX_SRC_WIN_UTILS_H_

#include <windows.h>
#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"

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




bool ConvertToLongPath(const base::string16& short_path,
                       base::string16* long_path);






DWORD IsReparsePoint(const base::string16& full_path, bool* result);


bool SameObject(HANDLE handle, const wchar_t* full_path);


bool GetPathFromHandle(HANDLE handle, base::string16* path);



bool GetNtPathFromWin32Path(const base::string16& path,
                            base::string16* nt_path);




HKEY GetReservedKeyFromName(const base::string16& name);





bool ResolveRegistryName(base::string16 name, base::string16* resolved_name);




bool WriteProtectedChildMemory(HANDLE child_process, void* address,
                               const void* buffer, size_t length);

}  



void ResolveNTFunctionPtr(const char* name, void* ptr);

#endif  

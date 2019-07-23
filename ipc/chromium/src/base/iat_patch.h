










#ifndef BASE_IAT_PATCH_H__
#define BASE_IAT_PATCH_H__

#include <windows.h>
#include "base/basictypes.h"
#include "base/pe_image.h"

namespace iat_patch {
















DWORD InterceptImportedFunction(HMODULE module_handle,
                                const char* imported_from_module,
                                const char* function_name,
                                void* new_function,
                                void** old_function,
                                IMAGE_THUNK_DATA** iat_thunk);










DWORD RestoreImportedFunction(void* intercept_function,
                              void* original_function,
                              IMAGE_THUNK_DATA* iat_thunk);











DWORD ModifyCode(void* old_code,
                 void* new_code,
                 int length);



class IATPatchFunction {
 public:
  IATPatchFunction();
  ~IATPatchFunction();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  DWORD Patch(const wchar_t* module,
              const char* imported_from_module,
              const char* function_name,
              void* new_function);

  
  
  
  
  
  DWORD Unpatch();

  bool is_patched() const {
    return (NULL != intercept_function_);
  }

 private:
  HMODULE module_handle_;
  void* intercept_function_;
  void* original_function_;
  IMAGE_THUNK_DATA* iat_thunk_;

  DISALLOW_EVIL_CONSTRUCTORS(IATPatchFunction);
};

}  

#endif  

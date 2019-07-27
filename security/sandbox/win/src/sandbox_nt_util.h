



#ifndef SANDBOX_SRC_SANDBOX_NT_UTIL_H_
#define SANDBOX_SRC_SANDBOX_NT_UTIL_H_

#include <intrin.h>

#include "base/basictypes.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_nt_types.h"


void* __cdecl operator new(size_t size, sandbox::AllocationType type,
                           void* near_to = NULL);
void __cdecl operator delete(void* memory, sandbox::AllocationType type);




void __cdecl operator delete(void* memory, sandbox::AllocationType type,
                             void* near_to);


void* __cdecl operator new(size_t size, void* buffer,
                           sandbox::AllocationType type);
void __cdecl operator delete(void* memory, void* buffer,
                             sandbox::AllocationType type);








#ifndef NDEBUG
#define DCHECK_NT(condition) { (condition) ? (void)0 : __debugbreak(); }
#define VERIFY(action) DCHECK_NT(action)
#define VERIFY_SUCCESS(action) DCHECK_NT(NT_SUCCESS(action))
#else
#define DCHECK_NT(condition)
#define VERIFY(action) (action)
#define VERIFY_SUCCESS(action) (action)
#endif

#define NOTREACHED_NT() DCHECK_NT(false)

namespace sandbox {

#if defined(_M_X64)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#elif defined(_M_IX86)
extern "C" long _InterlockedCompareExchange(long volatile* destination,
                                            long exchange, long comperand);

#pragma intrinsic(_InterlockedCompareExchange)



__forceinline void* _InterlockedCompareExchangePointer(
    void* volatile* destination, void* exchange, void* comperand) {
  size_t ret = _InterlockedCompareExchange(
      reinterpret_cast<long volatile*>(destination),
      static_cast<long>(reinterpret_cast<size_t>(exchange)),
      static_cast<long>(reinterpret_cast<size_t>(comperand)));

  return reinterpret_cast<void*>(static_cast<size_t>(ret));
}

#else
#error Architecture not supported.

#endif


void* GetGlobalIPCMemory();


void* GetGlobalPolicyMemory();

enum RequiredAccess {
  READ,
  WRITE
};






bool ValidParameter(void* buffer, size_t size, RequiredAccess intent);



NTSTATUS CopyData(void* destination, const void* source, size_t bytes);


NTSTATUS AllocAndCopyName(const OBJECT_ATTRIBUTES* in_object,
                          wchar_t** out_name, uint32* attributes, HANDLE* root);


bool InitHeap();


bool IsSameProcess(HANDLE process);

enum MappedModuleFlags {
  MODULE_IS_PE_IMAGE     = 1,   
  MODULE_HAS_ENTRY_POINT = 2,   
  MODULE_HAS_CODE =        4    
};















UNICODE_STRING* GetImageInfoFromModule(HMODULE module, uint32* flags);





UNICODE_STRING* GetBackingFilePath(PVOID address);





UNICODE_STRING* ExtractModuleName(const UNICODE_STRING* module_path);


bool IsValidImageSection(HANDLE section, PVOID *base, PLARGE_INTEGER offset,
                         PSIZE_T view_size);


UNICODE_STRING* AnsiToUnicode(const char* string);


class AutoProtectMemory {
 public:
  AutoProtectMemory()
      : changed_(false), address_(NULL), bytes_(0), old_protect_(0) {}

  ~AutoProtectMemory() {
    RevertProtection();
  }

  
  NTSTATUS ChangeProtection(void* address, size_t bytes, ULONG protect);

  
  NTSTATUS RevertProtection();

 private:
  bool changed_;
  void* address_;
  size_t bytes_;
  ULONG old_protect_;

  DISALLOW_COPY_AND_ASSIGN(AutoProtectMemory);
};



bool IsSupportedRenameCall(FILE_RENAME_INFORMATION* file_info, DWORD length,
                           uint32 file_info_class);

}  


#endif  

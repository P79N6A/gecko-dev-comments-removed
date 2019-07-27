



































#ifndef WindowsCrtPatch_h
#define WindowsCrtPatch_h

#include "nsWindowsDllInterceptor.h"
#include "mozilla/WindowsVersion.h"

namespace WindowsCrtPatch {

mozilla::WindowsDllInterceptor NtdllIntercept;

typedef PIMAGE_NT_HEADERS (NTAPI *RtlImageNtHeader_func)(HMODULE module);
static RtlImageNtHeader_func stub_RtlImageNtHeader = 0;


template <typename T>
class RVAPtr
{
public:
  RVAPtr(HMODULE module, size_t rva)
    : _ptr(reinterpret_cast<T*>(reinterpret_cast<char*>(module) + rva)) {}
  operator T*() { return _ptr; }
  T* operator ->() { return _ptr; }
  T* operator ++() { return ++_ptr; }

private:
  T* _ptr;
};

void
PatchModuleImports(HMODULE module, PIMAGE_NT_HEADERS headers)
{
  static const WORD MAGIC_DOS = 0x5a4d; 
  static const DWORD MAGIC_PE = 0x4550; 
  RVAPtr<IMAGE_DOS_HEADER> dosStub(module, 0);

  if (!module ||
      !headers ||
      dosStub->e_magic != MAGIC_DOS ||
      headers != RVAPtr<IMAGE_NT_HEADERS>(module, dosStub->e_lfanew) ||
      headers->Signature != MAGIC_PE ||
      headers->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER)) {
    return;
  }

  
  
  

  IMAGE_DATA_DIRECTORY* importDirectory =
    &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  RVAPtr<IMAGE_IMPORT_DESCRIPTOR> descriptor(module, importDirectory->VirtualAddress);

  for (; descriptor->OriginalFirstThunk; ++descriptor) {
    RVAPtr<char> importedModule(module, descriptor->Name);
    if (!stricmp(importedModule, "kernel32.dll")) {
      RVAPtr<IMAGE_THUNK_DATA> thunk(module, descriptor->OriginalFirstThunk);
      for (; thunk->u1.AddressOfData; ++thunk) {
        RVAPtr<IMAGE_IMPORT_BY_NAME> import(module, thunk->u1.AddressOfData);
        if (!strcmp(import->Name, "GetLogicalProcessorInformation")) {
          memcpy(import->Name, "DebugBreak", sizeof("DebugBreak"));
        }
      }
    }
  }
}

PIMAGE_NT_HEADERS NTAPI
patched_RtlImageNtHeader(HMODULE module)
{
  PIMAGE_NT_HEADERS headers = stub_RtlImageNtHeader(module);

  if (module == GetModuleHandleA("msvcr120.dll")) {
    PatchModuleImports(module, headers);
  }

  return headers;
}


MOZ_NEVER_INLINE void
Init()
{
  
  
  
  
  
  
  
  MOZ_ASSERT(!GetModuleHandleA("mozglue.dll"));
  MOZ_ASSERT(!GetModuleHandleA("msvcr120.dll"));
  MOZ_ASSERT(!GetModuleHandleA("msvcr120d.dll"));

  
  MOZ_ASSERT(!GetModuleHandleA("msvcr100.dll"));
  MOZ_ASSERT(!GetModuleHandleA("msvcr100d.dll"));

#if defined(_M_IX86) && defined(_MSC_VER) && _MSC_VER >= 1800
  if (!mozilla::IsXPSP3OrLater()) {
    NtdllIntercept.Init("ntdll.dll");
    NtdllIntercept.AddHook("RtlImageNtHeader",
                           reinterpret_cast<intptr_t>(patched_RtlImageNtHeader),
                           reinterpret_cast<void**>(&stub_RtlImageNtHeader));
  }
#endif
}

} 

#endif 

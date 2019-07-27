









#include <windows.h>

#include <string>

#include "sandbox/win/wow_helper/service64_resolver.h"
#include "sandbox/win/wow_helper/target_code.h"

namespace sandbox {




int PatchNtdll(HANDLE child, void* thunk, size_t thunk_bytes) {
  wchar_t* ntdll_name = L"ntdll.dll";
  HMODULE ntdll_base = ::GetModuleHandle(ntdll_name);
  if (!ntdll_base)
    return 100;

  Service64ResolverThunk resolver(child);
  size_t used = resolver.GetThunkSize();
  char* code = reinterpret_cast<char*>(thunk) + used;
  NTSTATUS ret = resolver.Setup(ntdll_base, NULL, "NtMapViewOfSection", NULL,
                                code, thunk, thunk_bytes, NULL);
  if (!NT_SUCCESS(ret))
    return 101;

  size_t size = reinterpret_cast<char*>(&TargetEnd) -
                reinterpret_cast<char*>(&TargetNtMapViewOfSection);

  if (size + used > thunk_bytes)
    return 102;

  SIZE_T written;
  if (!::WriteProcessMemory(child, code, &TargetNtMapViewOfSection, size,
                            &written))
    return 103;

  if (size != written)
    return 104;

  return 0;
}

}  








int wWinMain(HINSTANCE, HINSTANCE, wchar_t* command_line, int) {
  COMPILE_ASSERT(sizeof(void*) > sizeof(DWORD), unsupported_32_bits);
  if (!command_line)
    return 1;

  wchar_t* next;
  DWORD process_id = wcstoul(command_line, &next, 0);
  if (!process_id)
    return 2;

  DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE;
  HANDLE child = ::OpenProcess(access, FALSE, process_id);
  if (!child)
    return 3;

  DWORD buffer = wcstoul(next, NULL, 0);
  if (!buffer)
    return 4;

  void* thunk = reinterpret_cast<void*>(static_cast<ULONG_PTR>(buffer));

  const size_t kPageSize = 4096;
  return sandbox::PatchNtdll(child, thunk, kPageSize);
}

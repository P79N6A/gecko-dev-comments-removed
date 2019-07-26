



#include "sandbox/win/src/wow64.h"

#include <sstream>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/win/scoped_process_information.h"
#include "base/win/windows_version.h"
#include "sandbox/win/src/target_process.h"

namespace {





struct PatchInfo32 {
  HANDLE dll_load;  
  ULONG pad1;
  HANDLE continue_load;  
  ULONG pad2;
  HANDLE section;  
  ULONG pad3;
  void* orig_MapViewOfSection;
  ULONG original_high;
  void* signal_and_wait;
  ULONG pad4;
  void* patch_location;
  ULONG patch_high;
};


const SIZE_T kServiceEntry64Size = 0x10;


bool Restore64Code(HANDLE child, PatchInfo32* patch_info) {
  PatchInfo32 local_patch_info;
  SIZE_T actual;
  if (!::ReadProcessMemory(child, patch_info, &local_patch_info,
                           sizeof(local_patch_info), &actual))
    return false;
  if (sizeof(local_patch_info) != actual)
    return false;

  if (local_patch_info.original_high)
    return false;
  if (local_patch_info.patch_high)
    return false;

  char buffer[kServiceEntry64Size];

  if (!::ReadProcessMemory(child, local_patch_info.orig_MapViewOfSection,
                           &buffer, kServiceEntry64Size, &actual))
    return false;
  if (kServiceEntry64Size != actual)
    return false;

  if (!::WriteProcessMemory(child, local_patch_info.patch_location, &buffer,
                            kServiceEntry64Size, &actual))
    return false;
  if (kServiceEntry64Size != actual)
    return false;
  return true;
}

typedef BOOL (WINAPI* IsWow64ProcessFunction)(HANDLE process, BOOL* wow64);

}  

namespace sandbox {

Wow64::~Wow64() {
  if (dll_load_)
    ::CloseHandle(dll_load_);

  if (continue_load_)
    ::CloseHandle(continue_load_);
}








bool Wow64::WaitForNtdll() {
  if (base::win::OSInfo::GetInstance()->wow64_status() !=
      base::win::OSInfo::WOW64_ENABLED)
    return true;

  const size_t page_size = 4096;

  
  dll_load_ = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  continue_load_ = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  HANDLE current_process = ::GetCurrentProcess();
  HANDLE remote_load, remote_continue;
  DWORD access = EVENT_MODIFY_STATE | SYNCHRONIZE;
  if (!::DuplicateHandle(current_process, dll_load_, child_->Process(),
                         &remote_load, access, FALSE, 0))
    return false;
  if (!::DuplicateHandle(current_process, continue_load_, child_->Process(),
                         &remote_continue, access, FALSE, 0))
    return false;

  void* buffer = ::VirtualAllocEx(child_->Process(), NULL, page_size,
                                  MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  DCHECK(buffer);
  if (!buffer)
    return false;

  PatchInfo32* patch_info = reinterpret_cast<PatchInfo32*>(buffer);
  PatchInfo32 local_patch_info = {0};
  local_patch_info.dll_load = remote_load;
  local_patch_info.continue_load = remote_continue;
  SIZE_T written;
  if (!::WriteProcessMemory(child_->Process(), patch_info, &local_patch_info,
                            offsetof(PatchInfo32, section), &written))
    return false;
  if (offsetof(PatchInfo32, section) != written)
    return false;

  if (!RunWowHelper(buffer))
    return false;

  
  if (!DllMapped())
    return false;

  
  return Restore64Code(child_->Process(), patch_info);
}

bool Wow64::RunWowHelper(void* buffer) {
  COMPILE_ASSERT(sizeof(buffer) <= sizeof(DWORD), unsupported_64_bits);

  
  wchar_t prog_name[MAX_PATH];
  GetModuleFileNameW(NULL, prog_name, MAX_PATH);
  std::wstring path(prog_name);
  size_t name_pos = path.find_last_of(L"\\");
  if (std::wstring::npos == name_pos)
    return false;
  path.resize(name_pos + 1);

  std::wstringstream command;
  command << std::hex << std::showbase << L"\"" << path <<
               L"wow_helper.exe\" " << child_->ProcessId() << " " <<
               bit_cast<ULONG>(buffer);

  scoped_ptr_malloc<wchar_t> writable_command(_wcsdup(command.str().c_str()));

  STARTUPINFO startup_info = {0};
  startup_info.cb = sizeof(startup_info);
  base::win::ScopedProcessInformation process_info;
  if (!::CreateProcess(NULL, writable_command.get(), NULL, NULL, FALSE, 0, NULL,
                       NULL, &startup_info, process_info.Receive()))
    return false;

  DWORD reason = ::WaitForSingleObject(process_info.process_handle(), INFINITE);

  DWORD code;
  bool ok =
      ::GetExitCodeProcess(process_info.process_handle(), &code) ? true : false;

  if (WAIT_TIMEOUT == reason)
    return false;

  return ok && (0 == code);
}



bool Wow64::DllMapped() {
  if (1 != ::ResumeThread(child_->MainThread())) {
    NOTREACHED();
    return false;
  }

  for (;;) {
    DWORD reason = ::WaitForSingleObject(dll_load_, INFINITE);
    if (WAIT_TIMEOUT == reason || WAIT_ABANDONED == reason)
      return false;

    if (!::ResetEvent(dll_load_))
      return false;

    bool found = NtdllPresent();
    if (found) {
      if (::SuspendThread(child_->MainThread()))
        return false;
    }

    if (!::SetEvent(continue_load_))
      return false;

    if (found)
      return true;
  }
}

bool Wow64::NtdllPresent() {
  const size_t kBufferSize = 512;
  char buffer[kBufferSize];
  SIZE_T read;
  if (!::ReadProcessMemory(child_->Process(), ntdll_, &buffer, kBufferSize,
                           &read))
    return false;
  if (kBufferSize != read)
    return false;
  return true;
}

}  

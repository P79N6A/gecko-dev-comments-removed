



#include "sandbox/win/src/target_process.h"

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/win/pe_image.h"
#include "base/win/startup_information.h"
#include "base/win/windows_version.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/sharedmem_ipc_server.h"

namespace {

void CopyPolicyToTarget(const void* source, size_t size, void* dest) {
  if (!source || !size)
    return;
  memcpy(dest, source, size);
  sandbox::PolicyGlobal* policy =
      reinterpret_cast<sandbox::PolicyGlobal*>(dest);

  size_t offset = reinterpret_cast<size_t>(source);

  for (size_t i = 0; i < sandbox::kMaxServiceCount; i++) {
    size_t buffer = reinterpret_cast<size_t>(policy->entry[i]);
    if (buffer) {
      buffer -= offset;
      policy->entry[i] = reinterpret_cast<sandbox::PolicyBuffer*>(buffer);
    }
  }
}

}

namespace sandbox {

SANDBOX_INTERCEPT HANDLE g_shared_section;
SANDBOX_INTERCEPT size_t g_shared_IPC_size;
SANDBOX_INTERCEPT size_t g_shared_policy_size;



void* GetBaseAddress(const wchar_t* exe_name, void* entry_point) {
  HMODULE exe = ::LoadLibrary(exe_name);
  if (NULL == exe)
    return exe;

  base::win::PEImage pe(exe);
  if (!pe.VerifyMagic()) {
    ::FreeLibrary(exe);
    return exe;
  }
  PIMAGE_NT_HEADERS nt_header = pe.GetNTHeaders();
  char* base = reinterpret_cast<char*>(entry_point) -
    nt_header->OptionalHeader.AddressOfEntryPoint;

  ::FreeLibrary(exe);
  return base;
}


TargetProcess::TargetProcess(HANDLE initial_token, HANDLE lockdown_token,
                             HANDLE job, ThreadProvider* thread_pool)
  
  
  
    : lockdown_token_(lockdown_token),
      initial_token_(initial_token),
      job_(job),
      thread_pool_(thread_pool),
      base_address_(NULL) {
}

TargetProcess::~TargetProcess() {
  DWORD exit_code = 0;
  
  
  
  
  
  
  
  
  if (sandbox_process_info_.IsValid()) {
    ::WaitForSingleObject(sandbox_process_info_.process_handle(), 50);
    
    if (!::GetExitCodeProcess(sandbox_process_info_.process_handle(),
                              &exit_code) || (STILL_ACTIVE == exit_code)) {
      
      
      
      
      if (shared_section_.IsValid())
        shared_section_.Take();
      ipc_server_.release();
      sandbox_process_info_.TakeProcessHandle();
      return;
    }
  }

  
  
  ipc_server_.reset();
}



DWORD TargetProcess::Create(const wchar_t* exe_path,
                            const wchar_t* command_line,
                            bool inherit_handles,
                            const base::win::StartupInformation& startup_info,
                            base::win::ScopedProcessInformation* target_info) {
  exe_name_.reset(_wcsdup(exe_path));

  
  scoped_ptr<wchar_t, base::FreeDeleter> cmd_line(_wcsdup(command_line));

  
  DWORD flags =
      CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS;

  if (startup_info.has_extended_startup_info())
    flags |= EXTENDED_STARTUPINFO_PRESENT;

  if (job_ && base::win::GetVersion() < base::win::VERSION_WIN8) {
    
    
    flags |= CREATE_BREAKAWAY_FROM_JOB;
  }

  PROCESS_INFORMATION temp_process_info = {};
  if (!::CreateProcessAsUserW(lockdown_token_.Get(),
                              exe_path,
                              cmd_line.get(),
                              NULL,   
                              NULL,   
                              inherit_handles,
                              flags,
                              NULL,   
                              NULL,   
                              startup_info.startup_info(),
                              &temp_process_info)) {
    return ::GetLastError();
  }
  base::win::ScopedProcessInformation process_info(temp_process_info);
  lockdown_token_.Close();

  DWORD win_result = ERROR_SUCCESS;

  if (job_) {
    
    if (!::AssignProcessToJobObject(job_, process_info.process_handle())) {
      win_result = ::GetLastError();
      ::TerminateProcess(process_info.process_handle(), 0);
      return win_result;
    }
  }

  if (initial_token_.IsValid()) {
    
    
    
    HANDLE temp_thread = process_info.thread_handle();
    if (!::SetThreadToken(&temp_thread, initial_token_.Get())) {
      win_result = ::GetLastError();
      
      
      ::TerminateProcess(process_info.process_handle(), 0);
      return win_result;
    }
    initial_token_.Close();
  }

  CONTEXT context;
  context.ContextFlags = CONTEXT_ALL;
  if (!::GetThreadContext(process_info.thread_handle(), &context)) {
    win_result = ::GetLastError();
    ::TerminateProcess(process_info.process_handle(), 0);
    return win_result;
  }

#if defined(_WIN64)
  void* entry_point = reinterpret_cast<void*>(context.Rcx);
#else
#pragma warning(push)
#pragma warning(disable: 4312)
  
  void* entry_point = reinterpret_cast<void*>(context.Eax);
#pragma warning(pop)
#endif  

  if (!target_info->DuplicateFrom(process_info)) {
    win_result = ::GetLastError();  
    ::TerminateProcess(process_info.process_handle(), 0);
    return win_result;
  }

  base_address_ = GetBaseAddress(exe_path, entry_point);
  sandbox_process_info_.Set(process_info.Take());
  return win_result;
}

ResultCode TargetProcess::TransferVariable(const char* name, void* address,
                                           size_t size) {
  if (!sandbox_process_info_.IsValid())
    return SBOX_ERROR_UNEXPECTED_CALL;

  void* child_var = address;

#if SANDBOX_EXPORTS
  HMODULE module = ::LoadLibrary(exe_name_.get());
  if (NULL == module)
    return SBOX_ERROR_GENERIC;

  child_var = ::GetProcAddress(module, name);
  ::FreeLibrary(module);

  if (NULL == child_var)
    return SBOX_ERROR_GENERIC;

  size_t offset = reinterpret_cast<char*>(child_var) -
                  reinterpret_cast<char*>(module);
  child_var = reinterpret_cast<char*>(MainModule()) + offset;
#else
  UNREFERENCED_PARAMETER(name);
#endif

  SIZE_T written;
  if (!::WriteProcessMemory(sandbox_process_info_.process_handle(),
                            child_var, address, size, &written))
    return SBOX_ERROR_GENERIC;

  if (written != size)
    return SBOX_ERROR_GENERIC;

  return SBOX_ALL_OK;
}



DWORD TargetProcess::Init(Dispatcher* ipc_dispatcher, void* policy,
                          uint32 shared_IPC_size, uint32 shared_policy_size) {
  
  
  
  
  

  
  DWORD shared_mem_size = static_cast<DWORD>(shared_IPC_size +
                                             shared_policy_size);
  shared_section_.Set(::CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
                                           PAGE_READWRITE | SEC_COMMIT,
                                           0, shared_mem_size, NULL));
  if (!shared_section_.IsValid()) {
    return ::GetLastError();
  }

  DWORD access = FILE_MAP_READ | FILE_MAP_WRITE;
  HANDLE target_shared_section;
  if (!::DuplicateHandle(::GetCurrentProcess(), shared_section_.Get(),
                         sandbox_process_info_.process_handle(),
                         &target_shared_section, access, FALSE, 0)) {
    return ::GetLastError();
  }

  void* shared_memory = ::MapViewOfFile(shared_section_.Get(),
                                        FILE_MAP_WRITE|FILE_MAP_READ,
                                        0, 0, 0);
  if (NULL == shared_memory) {
    return ::GetLastError();
  }

  CopyPolicyToTarget(policy, shared_policy_size,
                     reinterpret_cast<char*>(shared_memory) + shared_IPC_size);

  ResultCode ret;
  
  g_shared_section = target_shared_section;
  ret = TransferVariable("g_shared_section", &g_shared_section,
                         sizeof(g_shared_section));
  g_shared_section = NULL;
  if (SBOX_ALL_OK != ret) {
    return (SBOX_ERROR_GENERIC == ret)?
           ::GetLastError() : ERROR_INVALID_FUNCTION;
  }
  g_shared_IPC_size = shared_IPC_size;
  ret = TransferVariable("g_shared_IPC_size", &g_shared_IPC_size,
                         sizeof(g_shared_IPC_size));
  g_shared_IPC_size = 0;
  if (SBOX_ALL_OK != ret) {
    return (SBOX_ERROR_GENERIC == ret) ?
           ::GetLastError() : ERROR_INVALID_FUNCTION;
  }
  g_shared_policy_size = shared_policy_size;
  ret = TransferVariable("g_shared_policy_size", &g_shared_policy_size,
                         sizeof(g_shared_policy_size));
  g_shared_policy_size = 0;
  if (SBOX_ALL_OK != ret) {
    return (SBOX_ERROR_GENERIC == ret) ?
           ::GetLastError() : ERROR_INVALID_FUNCTION;
  }

  ipc_server_.reset(
      new SharedMemIPCServer(sandbox_process_info_.process_handle(),
                             sandbox_process_info_.process_id(),
                             job_, thread_pool_, ipc_dispatcher));

  if (!ipc_server_->Init(shared_memory, shared_IPC_size, kIPCChannelSize))
    return ERROR_NOT_ENOUGH_MEMORY;

  
  ::CloseHandle(sandbox_process_info_.TakeThreadHandle());

  return ERROR_SUCCESS;
}

void TargetProcess::Terminate() {
  if (!sandbox_process_info_.IsValid())
    return;

  ::TerminateProcess(sandbox_process_info_.process_handle(), 0);
}

TargetProcess* MakeTestTargetProcess(HANDLE process, HMODULE base_address) {
  TargetProcess* target = new TargetProcess(NULL, NULL, NULL, NULL);
  PROCESS_INFORMATION process_info = {};
  process_info.hProcess = process;
  target->sandbox_process_info_.Set(process_info);
  target->base_address_ = base_address;
  return target;
}

}  

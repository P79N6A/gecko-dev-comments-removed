



#include "sandbox/win/src/process_thread_dispatcher.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_broker.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/process_thread_interception.h"
#include "sandbox/win/src/process_thread_policy.h"
#include "sandbox/win/src/sandbox.h"

namespace {












base::string16 GetPathFromCmdLine(const base::string16 &cmd_line) {
  base::string16 exe_name;
  
  if (cmd_line[0] == L'\"') {
    
    base::string16::size_type pos = cmd_line.find(L'\"', 1);
    if (base::string16::npos == pos)
      return cmd_line;
    exe_name = cmd_line.substr(1, pos - 1);
  } else {
    
    
    base::string16::size_type pos = cmd_line.find(L' ');
    if (base::string16::npos == pos) {
      
      exe_name = cmd_line;
    } else {
      exe_name = cmd_line.substr(0, pos);
    }
  }

  return exe_name;
}



bool IsPathRelative(const base::string16 &path) {
  
  
  if (path.find(L"\\\\") == 0 || path.find(L":\\") == 1)
    return false;
  return true;
}


bool ConvertToAbsolutePath(const base::string16& child_current_directory,
                           bool use_env_path, base::string16 *path) {
  wchar_t file_buffer[MAX_PATH];
  wchar_t *file_part = NULL;

  
  
  DWORD result = 0;
  if (use_env_path) {
    
    result = ::SearchPath(NULL, path->c_str(), NULL, MAX_PATH, file_buffer,
                          &file_part);
  }

  if (0 == result) {
    
    result = ::SearchPath(child_current_directory.c_str(), path->c_str(), NULL,
                          MAX_PATH, file_buffer, &file_part);
  }

  if (0 == result || result >= MAX_PATH)
    return false;

  *path = file_buffer;
  return true;
}

}  
namespace sandbox {

ThreadProcessDispatcher::ThreadProcessDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall open_thread = {
    {IPC_NTOPENTHREAD_TAG, UINT32_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &ThreadProcessDispatcher::NtOpenThread)
  };

  static const IPCCall open_process = {
    {IPC_NTOPENPROCESS_TAG, UINT32_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &ThreadProcessDispatcher::NtOpenProcess)
  };

  static const IPCCall process_token = {
    {IPC_NTOPENPROCESSTOKEN_TAG, VOIDPTR_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &ThreadProcessDispatcher::NtOpenProcessToken)
  };

  static const IPCCall process_tokenex = {
    {IPC_NTOPENPROCESSTOKENEX_TAG, VOIDPTR_TYPE, UINT32_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &ThreadProcessDispatcher::NtOpenProcessTokenEx)
  };

  static const IPCCall create_params = {
    {IPC_CREATEPROCESSW_TAG, WCHAR_TYPE, WCHAR_TYPE, WCHAR_TYPE, INOUTPTR_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &ThreadProcessDispatcher::CreateProcessW)
  };

  ipc_calls_.push_back(open_thread);
  ipc_calls_.push_back(open_process);
  ipc_calls_.push_back(process_token);
  ipc_calls_.push_back(process_tokenex);
  ipc_calls_.push_back(create_params);
}

bool ThreadProcessDispatcher::SetupService(InterceptionManager* manager,
                                           int service) {
  switch (service) {
    case IPC_NTOPENTHREAD_TAG:
    case IPC_NTOPENPROCESS_TAG:
    case IPC_NTOPENPROCESSTOKEN_TAG:
    case IPC_NTOPENPROCESSTOKENEX_TAG:
      
      NOTREACHED();
      return false;

    case IPC_CREATEPROCESSW_TAG:
      return INTERCEPT_EAT(manager, kKerneldllName, CreateProcessW,
                           CREATE_PROCESSW_ID, 44) &&
             INTERCEPT_EAT(manager, L"kernel32.dll", CreateProcessA,
                           CREATE_PROCESSA_ID, 44);

    default:
      return false;
  }
}

bool ThreadProcessDispatcher::NtOpenThread(IPCInfo* ipc,
                                           uint32 desired_access,
                                           uint32 thread_id) {
  HANDLE handle;
  NTSTATUS ret = ProcessPolicy::OpenThreadAction(*ipc->client_info,
                                                 desired_access, thread_id,
                                                 &handle);
  ipc->return_info.nt_status = ret;
  ipc->return_info.handle = handle;
  return true;
}

bool ThreadProcessDispatcher::NtOpenProcess(IPCInfo* ipc,
                                            uint32 desired_access,
                                            uint32 process_id) {
  HANDLE handle;
  NTSTATUS ret = ProcessPolicy::OpenProcessAction(*ipc->client_info,
                                                  desired_access, process_id,
                                                  &handle);
  ipc->return_info.nt_status = ret;
  ipc->return_info.handle = handle;
  return true;
}

bool ThreadProcessDispatcher::NtOpenProcessToken(IPCInfo* ipc,
                                                 HANDLE process,
                                                 uint32 desired_access) {
  HANDLE handle;
  NTSTATUS ret = ProcessPolicy::OpenProcessTokenAction(*ipc->client_info,
                                                       process, desired_access,
                                                       &handle);
  ipc->return_info.nt_status = ret;
  ipc->return_info.handle = handle;
  return true;
}

bool ThreadProcessDispatcher::NtOpenProcessTokenEx(IPCInfo* ipc,
                                                   HANDLE process,
                                                   uint32 desired_access,
                                                   uint32 attributes) {
  HANDLE handle;
  NTSTATUS ret = ProcessPolicy::OpenProcessTokenExAction(*ipc->client_info,
                                                         process,
                                                         desired_access,
                                                         attributes, &handle);
  ipc->return_info.nt_status = ret;
  ipc->return_info.handle = handle;
  return true;
}

bool ThreadProcessDispatcher::CreateProcessW(IPCInfo* ipc, base::string16* name,
                                             base::string16* cmd_line,
                                             base::string16* cur_dir,
                                             CountedBuffer* info) {
  if (sizeof(PROCESS_INFORMATION) != info->Size())
    return false;

  
  base::string16 exe_name;
  if (!name->empty())
    exe_name = *name;
  else
    exe_name = GetPathFromCmdLine(*cmd_line);

  if (IsPathRelative(exe_name)) {
    if (!ConvertToAbsolutePath(*cur_dir, name->empty(), &exe_name)) {
      
      ipc->return_info.win32_result = ERROR_FILE_NOT_FOUND;
      return true;
    }
  }

  const wchar_t* const_exe_name = exe_name.c_str();
  CountedParameterSet<NameBased> params;
  params[NameBased::NAME] = ParamPickerMake(const_exe_name);

  EvalResult eval = policy_base_->EvalPolicy(IPC_CREATEPROCESSW_TAG,
                                             params.GetBase());

  PROCESS_INFORMATION* proc_info =
      reinterpret_cast<PROCESS_INFORMATION*>(info->Buffer());
  
  
  DWORD ret = ProcessPolicy::CreateProcessWAction(eval, *ipc->client_info,
                                                  exe_name, *cmd_line,
                                                  proc_info);

  ipc->return_info.win32_result = ret;
  return true;
}

}  

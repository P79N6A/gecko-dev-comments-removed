



#include "sandbox/win/src/registry_dispatcher.h"

#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/policy_broker.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/registry_interception.h"
#include "sandbox/win/src/registry_policy.h"

namespace {


bool GetCompletePath(HANDLE root, const base::string16& name,
                     base::string16* complete_name) {
  if (root) {
    if (!sandbox::GetPathFromHandle(root, complete_name))
      return false;

    *complete_name += L"\\";
    *complete_name += name;
  } else {
    *complete_name = name;
  }

  return true;
}

}

namespace sandbox {

RegistryDispatcher::RegistryDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall create_params = {
    {IPC_NTCREATEKEY_TAG, WCHAR_TYPE, UINT32_TYPE, VOIDPTR_TYPE, UINT32_TYPE,
     UINT32_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(&RegistryDispatcher::NtCreateKey)
  };

  static const IPCCall open_params = {
    {IPC_NTOPENKEY_TAG, WCHAR_TYPE, UINT32_TYPE, VOIDPTR_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(&RegistryDispatcher::NtOpenKey)
  };

  ipc_calls_.push_back(create_params);
  ipc_calls_.push_back(open_params);
}

bool RegistryDispatcher::SetupService(InterceptionManager* manager,
                                      int service) {
  if (IPC_NTCREATEKEY_TAG == service)
    return INTERCEPT_NT(manager, NtCreateKey, CREATE_KEY_ID, 32);

  if (IPC_NTOPENKEY_TAG == service) {
    bool result = INTERCEPT_NT(manager, NtOpenKey, OPEN_KEY_ID, 16);
    if (base::win::GetVersion() >= base::win::VERSION_WIN7 ||
        (base::win::GetVersion() == base::win::VERSION_VISTA &&
         base::win::OSInfo::GetInstance()->version_type() ==
             base::win::SUITE_SERVER))
      result &= INTERCEPT_NT(manager, NtOpenKeyEx, OPEN_KEY_EX_ID, 20);
    return result;
  }

  return false;
}

bool RegistryDispatcher::NtCreateKey(IPCInfo* ipc,
                                     base::string16* name,
                                     uint32 attributes,
                                     HANDLE root,
                                     uint32 desired_access,
                                     uint32 title_index,
                                     uint32 create_options) {
  base::win::ScopedHandle root_handle;
  base::string16 real_path = *name;

  
  
  if (root) {
    if (!::DuplicateHandle(ipc->client_info->process, root,
                           ::GetCurrentProcess(), &root, 0, FALSE,
                           DUPLICATE_SAME_ACCESS))
      return false;

    root_handle.Set(root);
  }

  if (!GetCompletePath(root, *name, &real_path))
    return false;

  const wchar_t* regname = real_path.c_str();
  CountedParameterSet<OpenKey> params;
  params[OpenKey::NAME] = ParamPickerMake(regname);
  params[OpenKey::ACCESS] = ParamPickerMake(desired_access);

  EvalResult result = policy_base_->EvalPolicy(IPC_NTCREATEKEY_TAG,
                                               params.GetBase());

  HANDLE handle;
  NTSTATUS nt_status;
  ULONG disposition = 0;
  if (!RegistryPolicy::CreateKeyAction(result, *ipc->client_info, *name,
                                       attributes, root, desired_access,
                                       title_index, create_options, &handle,
                                       &nt_status, &disposition)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  
  ipc->return_info.extended[0].unsigned_int = disposition;
  ipc->return_info.nt_status = nt_status;
  ipc->return_info.handle = handle;
  return true;
}

bool RegistryDispatcher::NtOpenKey(IPCInfo* ipc,
                                   base::string16* name,
                                   uint32 attributes,
                                   HANDLE root,
                                   uint32 desired_access) {
  base::win::ScopedHandle root_handle;
  base::string16 real_path = *name;

  
  
  if (root) {
    if (!::DuplicateHandle(ipc->client_info->process, root,
                           ::GetCurrentProcess(), &root, 0, FALSE,
                           DUPLICATE_SAME_ACCESS))
      return false;
      root_handle.Set(root);
  }

  if (!GetCompletePath(root, *name, &real_path))
    return false;

  const wchar_t* regname = real_path.c_str();
  CountedParameterSet<OpenKey> params;
  params[OpenKey::NAME] = ParamPickerMake(regname);
  params[OpenKey::ACCESS] = ParamPickerMake(desired_access);

  EvalResult result = policy_base_->EvalPolicy(IPC_NTOPENKEY_TAG,
                                               params.GetBase());
  HANDLE handle;
  NTSTATUS nt_status;
  if (!RegistryPolicy::OpenKeyAction(result, *ipc->client_info, *name,
                                     attributes, root, desired_access, &handle,
                                     &nt_status)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  
  ipc->return_info.nt_status = nt_status;
  ipc->return_info.handle = handle;
  return true;
}

}  

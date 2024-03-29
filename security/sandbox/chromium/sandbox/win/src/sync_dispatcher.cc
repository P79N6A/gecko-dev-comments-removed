



#include "sandbox/win/src/sync_dispatcher.h"

#include "base/win/windows_version.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_broker.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sync_interception.h"
#include "sandbox/win/src/sync_policy.h"

namespace sandbox {

SyncDispatcher::SyncDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall create_params = {
    {IPC_CREATEEVENT_TAG, WCHAR_TYPE, UINT32_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(&SyncDispatcher::CreateEvent)
  };

  static const IPCCall open_params = {
    {IPC_OPENEVENT_TAG, WCHAR_TYPE, UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(&SyncDispatcher::OpenEvent)
  };

  ipc_calls_.push_back(create_params);
  ipc_calls_.push_back(open_params);
}

bool SyncDispatcher::SetupService(InterceptionManager* manager,
                                  int service) {
  if (service == IPC_CREATEEVENT_TAG) {
    return INTERCEPT_NT(manager, NtCreateEvent, CREATE_EVENT_ID, 24);
  }
  return (service == IPC_OPENEVENT_TAG) &&
      INTERCEPT_NT(manager, NtOpenEvent, OPEN_EVENT_ID, 16);
}

bool SyncDispatcher::CreateEvent(IPCInfo* ipc,
                                 base::string16* name,
                                 uint32 event_type,
                                 uint32 initial_state) {
  const wchar_t* event_name = name->c_str();
  CountedParameterSet<NameBased> params;
  params[NameBased::NAME] = ParamPickerMake(event_name);

  EvalResult result = policy_base_->EvalPolicy(IPC_CREATEEVENT_TAG,
                                               params.GetBase());
  HANDLE handle = NULL;
  
  ipc->return_info.nt_status = SyncPolicy::CreateEventAction(
      result, *ipc->client_info, *name, event_type, initial_state, &handle);
  ipc->return_info.handle = handle;
  return true;
}

bool SyncDispatcher::OpenEvent(IPCInfo* ipc,
                               base::string16* name,
                               uint32 desired_access) {
  const wchar_t* event_name = name->c_str();

  CountedParameterSet<OpenEventParams> params;
  params[OpenEventParams::NAME] = ParamPickerMake(event_name);
  params[OpenEventParams::ACCESS] = ParamPickerMake(desired_access);

  EvalResult result = policy_base_->EvalPolicy(IPC_OPENEVENT_TAG,
                                               params.GetBase());
  HANDLE handle = NULL;
  
  ipc->return_info.nt_status = SyncPolicy::OpenEventAction(
      result, *ipc->client_info, *name, desired_access, &handle);
  ipc->return_info.handle = handle;
  return true;
}

}  

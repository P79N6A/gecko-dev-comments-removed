



#include "sandbox/win/src/registry_interception.h"

#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/target_services.h"
#ifdef MOZ_CONTENT_SANDBOX 
#include "mozilla/warnonlysandbox/warnOnlySandbox.h"
#endif

namespace sandbox {

NTSTATUS WINAPI TargetNtCreateKey(NtCreateKeyFunction orig_CreateKey,
                                  PHANDLE key, ACCESS_MASK desired_access,
                                  POBJECT_ATTRIBUTES object_attributes,
                                  ULONG title_index, PUNICODE_STRING class_name,
                                  ULONG create_options, PULONG disposition) {
  
  NTSTATUS status = orig_CreateKey(key, desired_access, object_attributes,
                                   title_index, class_name, create_options,
                                   disposition);
  if (NT_SUCCESS(status))
    return status;

#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogBlocked("NtCreateKey",
                                       object_attributes->ObjectName->Buffer,
                                       object_attributes->ObjectName->Length);
#endif

  
  if (!SandboxFactory::GetTargetServices()->GetState()->InitCalled())
    return status;

  do {
    if (!ValidParameter(key, sizeof(HANDLE), WRITE))
      break;

    if (disposition && !ValidParameter(disposition, sizeof(ULONG), WRITE))
      break;

    
    if (class_name && class_name->Buffer && class_name->Length)
      break;

    
    if (create_options)
      break;

    void* memory = GetGlobalIPCMemory();
    if (NULL == memory)
      break;

    wchar_t* name;
    uint32 attributes = 0;
    HANDLE root_directory = 0;
    NTSTATUS ret = AllocAndCopyName(object_attributes, &name, &attributes,
                                    &root_directory);
    if (!NT_SUCCESS(ret) || NULL == name)
      break;

    SharedMemIPCClient ipc(memory);
    CrossCallReturn answer = {0};

    ResultCode code = CrossCall(ipc, IPC_NTCREATEKEY_TAG, name, attributes,
                                root_directory, desired_access, title_index,
                                create_options, &answer);

    operator delete(name, NT_ALLOC);

    if (SBOX_ALL_OK != code)
      break;

    if (!NT_SUCCESS(answer.nt_status))
        
        
        
        
        
        
        break;

    __try {
      *key = answer.handle;

      if (disposition)
       *disposition = answer.extended[0].unsigned_int;

      status = answer.nt_status;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
      break;
    }
#ifdef MOZ_CONTENT_SANDBOX
    mozilla::warnonlysandbox::LogAllowed("NtCreateKey",
                                         object_attributes->ObjectName->Buffer,
                                         object_attributes->ObjectName->Length);
#endif
  } while (false);

  return status;
}

NTSTATUS WINAPI CommonNtOpenKey(NTSTATUS status, PHANDLE key,
                                ACCESS_MASK desired_access,
                                POBJECT_ATTRIBUTES object_attributes) {
  
  if (!SandboxFactory::GetTargetServices()->GetState()->InitCalled())
    return status;

  do {
    if (!ValidParameter(key, sizeof(HANDLE), WRITE))
      break;

    void* memory = GetGlobalIPCMemory();
    if (NULL == memory)
      break;

    wchar_t* name;
    uint32 attributes;
    HANDLE root_directory;
    NTSTATUS ret = AllocAndCopyName(object_attributes, &name, &attributes,
                                    &root_directory);
    if (!NT_SUCCESS(ret) || NULL == name)
      break;

    SharedMemIPCClient ipc(memory);
    CrossCallReturn answer = {0};
    ResultCode code = CrossCall(ipc, IPC_NTOPENKEY_TAG, name, attributes,
                                root_directory, desired_access, &answer);

    operator delete(name, NT_ALLOC);

    if (SBOX_ALL_OK != code)
      break;

    if (!NT_SUCCESS(answer.nt_status))
        
        
        
        
        
        
        break;

    __try {
      *key = answer.handle;
      status = answer.nt_status;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
      break;
    }
#ifdef MOZ_CONTENT_SANDBOX
    mozilla::warnonlysandbox::LogAllowed("NtOpenKey[Ex]",
                                         object_attributes->ObjectName->Buffer,
                                         object_attributes->ObjectName->Length);
#endif
  } while (false);

  return status;
}

NTSTATUS WINAPI TargetNtOpenKey(NtOpenKeyFunction orig_OpenKey, PHANDLE key,
                                ACCESS_MASK desired_access,
                                POBJECT_ATTRIBUTES object_attributes) {
  
  NTSTATUS status = orig_OpenKey(key, desired_access, object_attributes);
  if (NT_SUCCESS(status))
    return status;

#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogBlocked("NtOpenKey",
                                       object_attributes->ObjectName->Buffer,
                                       object_attributes->ObjectName->Length);
#endif

  return CommonNtOpenKey(status, key, desired_access, object_attributes);
}

NTSTATUS WINAPI TargetNtOpenKeyEx(NtOpenKeyExFunction orig_OpenKeyEx,
                                  PHANDLE key, ACCESS_MASK desired_access,
                                  POBJECT_ATTRIBUTES object_attributes,
                                  ULONG open_options) {
  
  NTSTATUS status = orig_OpenKeyEx(key, desired_access, object_attributes,
                                   open_options);

  
  
  
  if (NT_SUCCESS(status) || open_options != 0)
    return status;

#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogBlocked("NtOpenKeyEx",
                                       object_attributes->ObjectName->Buffer,
                                       object_attributes->ObjectName->Length);
#endif

  return CommonNtOpenKey(status, key, desired_access, object_attributes);
}

}  

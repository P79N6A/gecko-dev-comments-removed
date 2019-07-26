



#include "sandbox/win/src/service_resolver.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/win_utils.h"

namespace {
#pragma pack(push, 1)

const ULONG kMmovR10EcxMovEax = 0xB8D18B4C;
const USHORT kSyscall = 0x050F;
const BYTE kRetNp = 0xC3;
const ULONG64 kMov1 = 0x54894808244C8948;
const ULONG64 kMov2 = 0x4C182444894C1024;
const ULONG kMov3 = 0x20244C89;


struct ServiceEntry {
  
  
  
  
  
  
  

  ULONG mov_r10_rcx_mov_eax;  
  ULONG service_id;
  USHORT syscall;             
  BYTE ret;                   
  BYTE pad;                   
  USHORT xchg_ax_ax1;         
  USHORT xchg_ax_ax2;         
};


struct ServiceEntryW8 {
  
  
  
  
  
  
  
  
  
  

  ULONG64 mov_1;              
  ULONG64 mov_2;              
  ULONG mov_3;                
  ULONG mov_r10_rcx_mov_eax;  
  ULONG service_id;
  USHORT syscall;             
  BYTE ret;                   
  BYTE nop;                   
};


struct ServiceFullThunk {
  union {
    ServiceEntry original;
    ServiceEntryW8 original_w8;
  };
};

#pragma pack(pop)

bool IsService(const void* source) {
  const ServiceEntry* service =
      reinterpret_cast<const ServiceEntry*>(source);

  return (kMmovR10EcxMovEax == service->mov_r10_rcx_mov_eax &&
          kSyscall == service->syscall && kRetNp == service->ret);
}

};  

namespace sandbox {

NTSTATUS ServiceResolverThunk::Setup(const void* target_module,
                                     const void* interceptor_module,
                                     const char* target_name,
                                     const char* interceptor_name,
                                     const void* interceptor_entry_point,
                                     void* thunk_storage,
                                     size_t storage_bytes,
                                     size_t* storage_used) {
  NTSTATUS ret = Init(target_module, interceptor_module, target_name,
                      interceptor_name, interceptor_entry_point,
                      thunk_storage, storage_bytes);
  if (!NT_SUCCESS(ret))
    return ret;

  size_t thunk_bytes = GetThunkSize();
  scoped_ptr<char[]> thunk_buffer(new char[thunk_bytes]);
  ServiceFullThunk* thunk = reinterpret_cast<ServiceFullThunk*>(
                                thunk_buffer.get());

  if (!IsFunctionAService(&thunk->original))
    return STATUS_UNSUCCESSFUL;

  ret = PerformPatch(thunk, thunk_storage);

  if (NULL != storage_used)
    *storage_used = thunk_bytes;

  return ret;
}

size_t ServiceResolverThunk::GetThunkSize() const {
  return sizeof(ServiceFullThunk);
}

bool ServiceResolverThunk::IsFunctionAService(void* local_thunk) const {
  ServiceFullThunk function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (!IsService(&function_code)) {
    
    ServiceEntryW8* w8_service = &function_code.original_w8;
    if (!IsService(&w8_service->mov_r10_rcx_mov_eax) ||
        w8_service->mov_1 != kMov1 || w8_service->mov_1 != kMov1 ||
        w8_service->mov_1 != kMov1) {
      return false;
    }
  }

  
  memcpy(local_thunk, &function_code, sizeof(function_code));

  return true;
}

NTSTATUS ServiceResolverThunk::PerformPatch(void* local_thunk,
                                            void* remote_thunk) {
  ServiceFullThunk* full_local_thunk = reinterpret_cast<ServiceFullThunk*>(
                                           local_thunk);
  ServiceFullThunk* full_remote_thunk = reinterpret_cast<ServiceFullThunk*>(
                                            remote_thunk);

  
  ServiceEntry local_service;
  DCHECK_GE(GetInternalThunkSize(), sizeof(local_service));
  if (!SetInternalThunk(&local_service, sizeof(local_service), NULL,
                        interceptor_))
    return STATUS_UNSUCCESSFUL;

  
  SIZE_T actual;
  if (!::WriteProcessMemory(process_, remote_thunk, local_thunk,
                            sizeof(ServiceFullThunk), &actual))
    return STATUS_UNSUCCESSFUL;

  if (sizeof(ServiceFullThunk) != actual)
    return STATUS_UNSUCCESSFUL;

  
  if (NULL != ntdll_base_) {
    
    if (!::WriteProcessMemory(process_, target_, &local_service,
                              sizeof(local_service), &actual))
      return STATUS_UNSUCCESSFUL;
  } else {
    if (!WriteProtectedChildMemory(process_, target_, &local_service,
                                   sizeof(local_service)))
      return STATUS_UNSUCCESSFUL;
  }

  return STATUS_SUCCESS;
}

bool Wow64ResolverThunk::IsFunctionAService(void* local_thunk) const {
  NOTREACHED();
  return false;
}

bool Win2kResolverThunk::IsFunctionAService(void* local_thunk) const {
  NOTREACHED();
  return false;
}

}  

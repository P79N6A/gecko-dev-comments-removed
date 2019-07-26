



#include "sandbox/win/src/sidestep_resolver.h"

#include "base/win/pe_image.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sidestep/preamble_patcher.h"

namespace {

const size_t kSizeOfSidestepStub = sidestep::kMaxPreambleStubSize;

struct SidestepThunk {
  char sidestep[kSizeOfSidestepStub];  
  int internal_thunk;  
};

struct SmartThunk {
  const void* module_base;  
  const void* interceptor;  
  SidestepThunk sidestep;  
};

}  

namespace sandbox {

NTSTATUS SidestepResolverThunk::Setup(const void* target_module,
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

  SidestepThunk* thunk = reinterpret_cast<SidestepThunk*>(thunk_storage);

  size_t internal_bytes = storage_bytes - kSizeOfSidestepStub;
  if (!SetInternalThunk(&thunk->internal_thunk, internal_bytes, thunk_storage,
                        interceptor_))
    return STATUS_BUFFER_TOO_SMALL;

  AutoProtectMemory memory;
  ret = memory.ChangeProtection(target_, kSizeOfSidestepStub, PAGE_READWRITE);
  if (!NT_SUCCESS(ret))
    return ret;

  sidestep::SideStepError rv = sidestep::PreamblePatcher::Patch(
      target_, reinterpret_cast<void*>(&thunk->internal_thunk), thunk_storage,
      kSizeOfSidestepStub);

  if (sidestep::SIDESTEP_INSUFFICIENT_BUFFER == rv)
    return STATUS_BUFFER_TOO_SMALL;

  if (sidestep::SIDESTEP_SUCCESS != rv)
    return STATUS_UNSUCCESSFUL;

  if (storage_used)
    *storage_used = GetThunkSize();

  return ret;
}

size_t SidestepResolverThunk::GetThunkSize() const {
  return GetInternalThunkSize() + kSizeOfSidestepStub;
}






NTSTATUS SmartSidestepResolverThunk::Setup(const void* target_module,
                                           const void* interceptor_module,
                                           const char* target_name,
                                           const char* interceptor_name,
                                           const void* interceptor_entry_point,
                                           void* thunk_storage,
                                           size_t storage_bytes,
                                           size_t* storage_used) {
  if (storage_bytes < GetThunkSize())
    return STATUS_BUFFER_TOO_SMALL;

  SmartThunk* thunk = reinterpret_cast<SmartThunk*>(thunk_storage);
  thunk->module_base = target_module;

  NTSTATUS ret;
  if (interceptor_entry_point) {
    thunk->interceptor = interceptor_entry_point;
  } else {
    ret = ResolveInterceptor(interceptor_module, interceptor_name,
                             &thunk->interceptor);
    if (!NT_SUCCESS(ret))
      return ret;
  }

  
  
  size_t standard_bytes = storage_bytes - offsetof(SmartThunk, sidestep);
  ret = SidestepResolverThunk::Setup(target_module, interceptor_module,
                                     target_name, NULL, &SmartStub,
                                     &thunk->sidestep, standard_bytes, NULL);
  if (!NT_SUCCESS(ret))
    return ret;

  
  SetInternalThunk(&thunk->sidestep.internal_thunk, GetInternalThunkSize(),
                   thunk_storage, &SmartStub);

  if (storage_used)
    *storage_used = GetThunkSize();

  return ret;
}

size_t SmartSidestepResolverThunk::GetThunkSize() const {
  return GetInternalThunkSize() + kSizeOfSidestepStub +
         offsetof(SmartThunk, sidestep);
}





















__declspec(naked)
void SmartSidestepResolverThunk::SmartStub() {
  __asm {
    push eax                  
    push eax                  
    push ebx
    push ecx
    push edx
    mov ebx, [esp + 0x18]     
    mov edx, [esp + 0x14]     
    mov eax, [ebx]SmartThunk.module_base
    push edx
    push eax
    call SmartSidestepResolverThunk::IsInternalCall
    add esp, 8

    test eax, eax
    lea edx, [ebx]SmartThunk.sidestep   
    jz call_interceptor

    
    mov ecx, [esp + 0x14]               
    mov [esp + 0x18], ecx               
    mov [esp + 0x10], edx
    pop edx                             
    pop ecx
    pop ebx
    pop eax
    ret 4                               

  call_interceptor:
    mov ecx, [ebx]SmartThunk.interceptor
    mov [esp + 0x18], edx               
    mov [esp + 0x10], ecx
    pop edx                             
    pop ecx
    pop ebx
    pop eax
    ret                                 
  }
}

bool SmartSidestepResolverThunk::IsInternalCall(const void* base,
                                                void* return_address) {
  DCHECK_NT(base);
  DCHECK_NT(return_address);

  base::win::PEImage pe(base);
  if (pe.GetImageSectionFromAddr(return_address))
    return true;
  return false;
}

}  

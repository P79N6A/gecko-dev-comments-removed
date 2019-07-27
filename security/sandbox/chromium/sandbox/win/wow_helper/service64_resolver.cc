



#include "sandbox/win/wow_helper/service64_resolver.h"

#include "base/memory/scoped_ptr.h"
#include "sandbox/win/wow_helper/target_code.h"

namespace {
#pragma pack(push, 1)

const BYTE kMovEax = 0xB8;
const BYTE kMovEdx = 0xBA;
const USHORT kCallPtrEdx = 0x12FF;
const BYTE kRet = 0xC2;
const BYTE kNop = 0x90;
const USHORT kJmpEdx = 0xE2FF;
const USHORT kXorEcx = 0xC933;
const ULONG kLeaEdx = 0x0424548D;
const ULONG kCallFs1 = 0xC015FF64;
const ULONG kCallFs2Ret = 0xC2000000;
const BYTE kPopEdx = 0x5A;
const BYTE kPushEdx = 0x52;
const BYTE kPush32 = 0x68;

const ULONG kMmovR10EcxMovEax = 0xB8D18B4C;
const USHORT kSyscall = 0x050F;
const BYTE kRetNp = 0xC3;
const BYTE kPad = 0x66;
const USHORT kNop16 = 0x9066;
const BYTE kRelJmp = 0xE9;

const ULONG kXorRaxMovEax = 0xB8C03148;
const ULONG kSaveRcx = 0x10488948;
const ULONG kMovRcxRaxJmp = 0xE9C88B48;


struct ServiceEntry {
  
  
  
  
  
  
  

  ULONG mov_r10_ecx_mov_eax;  
  ULONG service_id;
  USHORT syscall;             
  BYTE ret;                   
  BYTE pad;                   
  USHORT xchg_ax_ax1;         
  USHORT xchg_ax_ax2;         
};

struct Redirected {
  
  
  

  Redirected() {
    jmp = kRelJmp;
    relative = 0;
    pad = kPad;
    xchg_ax_ax = kNop16;
  };
  BYTE jmp;             
  ULONG relative;
  BYTE pad;             
  USHORT xchg_ax_ax;    
};

struct InternalThunk {
  
  
  
  
  
  

  InternalThunk() {
    xor_rax_mov_eax = kXorRaxMovEax;
    patch_info = 0;
    save_rcx = kSaveRcx;
    mov_rcx_rax_jmp = kMovRcxRaxJmp;
    relative = 0;
  };
  ULONG xor_rax_mov_eax;  
  ULONG patch_info;
  ULONG save_rcx;         
  ULONG mov_rcx_rax_jmp;  
  ULONG relative;
};

struct ServiceFullThunk {
  sandbox::PatchInfo patch_info;
  ServiceEntry original;
  InternalThunk internal_thunk;
};

#pragma pack(pop)









bool WriteProtectedChildMemory(HANDLE child_process,
                               void* address,
                               const void* buffer,
                               size_t length) {
  
  DWORD old_protection;
  if (!::VirtualProtectEx(child_process, address, length,
                          PAGE_WRITECOPY, &old_protection))
    return false;

  SIZE_T written;
  bool ok = ::WriteProcessMemory(child_process, address, buffer, length,
                                 &written) && (length == written);

  
  if (!::VirtualProtectEx(child_process, address, length,
                          old_protection, &old_protection))
    return false;

  return ok;
}


NTSTATUS ResolveNtdll(sandbox::PatchInfo* patch_info) {
  wchar_t* ntdll_name = L"ntdll.dll";
  HMODULE ntdll = ::GetModuleHandle(ntdll_name);
  if (!ntdll)
    return STATUS_PROCEDURE_NOT_FOUND;

  void* signal = ::GetProcAddress(ntdll, "NtSignalAndWaitForSingleObject");
  if (!signal)
    return STATUS_PROCEDURE_NOT_FOUND;

  patch_info->signal_and_wait =
      reinterpret_cast<NtSignalAndWaitForSingleObjectFunction>(signal);

  return STATUS_SUCCESS;
}

};  

namespace sandbox {

NTSTATUS ResolverThunk::Init(const void* target_module,
                             const void* interceptor_module,
                             const char* target_name,
                             const char* interceptor_name,
                             const void* interceptor_entry_point,
                             void* thunk_storage,
                             size_t storage_bytes) {
  if (NULL == thunk_storage || 0 == storage_bytes ||
      NULL == target_module || NULL == target_name)
    return STATUS_INVALID_PARAMETER;

  if (storage_bytes < GetThunkSize())
    return STATUS_BUFFER_TOO_SMALL;

  NTSTATUS ret = STATUS_SUCCESS;
  if (NULL == interceptor_entry_point) {
    ret = ResolveInterceptor(interceptor_module, interceptor_name,
                             &interceptor_entry_point);
    if (!NT_SUCCESS(ret))
      return ret;
  }

  ret = ResolveTarget(target_module, target_name, &target_);
  if (!NT_SUCCESS(ret))
    return ret;

  interceptor_ = interceptor_entry_point;

  return ret;
}

NTSTATUS ResolverThunk::ResolveInterceptor(const void* interceptor_module,
                                           const char* interceptor_name,
                                           const void** address) {
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS ResolverThunk::ResolveTarget(const void* module,
                                      const char* function_name,
                                      void** address) {
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS Service64ResolverThunk::Setup(const void* target_module,
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

NTSTATUS Service64ResolverThunk::ResolveInterceptor(
    const void* interceptor_module,
    const char* interceptor_name,
    const void** address) {
  
  
  return ResolveTarget(interceptor_module, interceptor_name,
                       const_cast<void**>(address));
}



NTSTATUS Service64ResolverThunk::ResolveTarget(const void* module,
                                             const char* function_name,
                                             void** address) {
  if (NULL == module)
    return STATUS_UNSUCCESSFUL;

  *address = ::GetProcAddress(bit_cast<HMODULE>(module), function_name);

  if (NULL == *address)
    return STATUS_UNSUCCESSFUL;

  return STATUS_SUCCESS;
}

size_t Service64ResolverThunk::GetThunkSize() const {
  return sizeof(ServiceFullThunk);
}

bool Service64ResolverThunk::IsFunctionAService(void* local_thunk) const {
  ServiceEntry function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kMmovR10EcxMovEax != function_code.mov_r10_ecx_mov_eax ||
      kSyscall != function_code.syscall || kRetNp != function_code.ret)
    return false;

  
  memcpy(local_thunk, &function_code, sizeof(function_code));

  return true;
}

NTSTATUS Service64ResolverThunk::PerformPatch(void* local_thunk,
                                              void* remote_thunk) {
  ServiceFullThunk* full_local_thunk = reinterpret_cast<ServiceFullThunk*>(
                                           local_thunk);
  ServiceFullThunk* full_remote_thunk = reinterpret_cast<ServiceFullThunk*>(
                                           remote_thunk);

  
  if (reinterpret_cast<ULONG_PTR>(full_remote_thunk) >
      static_cast<ULONG_PTR>(ULONG_MAX))
    return STATUS_CONFLICTING_ADDRESSES;

  if (reinterpret_cast<ULONG_PTR>(target_) > static_cast<ULONG_PTR>(ULONG_MAX))
    return STATUS_CONFLICTING_ADDRESSES;

  
  Redirected local_service;
  Redirected* remote_service = reinterpret_cast<Redirected*>(target_);
  ULONG_PTR diff = reinterpret_cast<BYTE*>(&full_remote_thunk->internal_thunk) -
                   &remote_service->pad;
  local_service.relative = static_cast<ULONG>(diff);

  
  SIZE_T actual;
  if (!::ReadProcessMemory(process_, remote_thunk, local_thunk,
                           sizeof(PatchInfo), &actual))
    return STATUS_UNSUCCESSFUL;
  if (sizeof(PatchInfo) != actual)
    return STATUS_UNSUCCESSFUL;

  full_local_thunk->patch_info.orig_MapViewOfSection = reinterpret_cast<
      NtMapViewOfSectionFunction>(&full_remote_thunk->original);
  full_local_thunk->patch_info.patch_location = target_;
  NTSTATUS ret = ResolveNtdll(&full_local_thunk->patch_info);
  if (!NT_SUCCESS(ret))
    return ret;

  
  
  InternalThunk my_thunk;
  ULONG_PTR patch_info = reinterpret_cast<ULONG_PTR>(remote_thunk);
  my_thunk.patch_info = static_cast<ULONG>(patch_info);
  diff = reinterpret_cast<const BYTE*>(interceptor_) -
         reinterpret_cast<BYTE*>(full_remote_thunk + 1);
  my_thunk.relative = static_cast<ULONG>(diff);

  memcpy(&full_local_thunk->internal_thunk, &my_thunk, sizeof(my_thunk));

  
  if (!::WriteProcessMemory(process_, remote_thunk, local_thunk,
                            sizeof(ServiceFullThunk), &actual))
    return STATUS_UNSUCCESSFUL;

  if (sizeof(ServiceFullThunk) != actual)
    return STATUS_UNSUCCESSFUL;

  
  if (!::WriteProtectedChildMemory(process_, target_, &local_service,
                                   sizeof(local_service)))
    return STATUS_UNSUCCESSFUL;

  return STATUS_SUCCESS;
}

}  

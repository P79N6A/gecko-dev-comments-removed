



#include "sandbox/win/src/service_resolver.h"

#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/win_utils.h"

namespace {
#pragma pack(push, 1)

const BYTE kMovEax = 0xB8;
const BYTE kMovEdx = 0xBA;
const USHORT kMovEdxEsp = 0xD48B;
const USHORT kCallPtrEdx = 0x12FF;
const USHORT kCallEdx = 0xD2FF;
const BYTE kCallEip = 0xE8;
const BYTE kRet = 0xC2;
const BYTE kRet2 = 0xC3;
const BYTE kNop = 0x90;
const USHORT kJmpEdx = 0xE2FF;
const USHORT kXorEcx = 0xC933;
const ULONG kLeaEdx = 0x0424548D;
const ULONG kCallFs1 = 0xC015FF64;
const USHORT kCallFs2 = 0;
const BYTE kCallFs3 = 0;
const BYTE kAddEsp1 = 0x83;
const USHORT kAddEsp2 = 0x4C4;
const BYTE kJmp32 = 0xE9;
const USHORT kSysenter = 0x340F;

const int kMaxService = 1000;



struct ServiceEntry {
  
  
  
  
  
  
  BYTE mov_eax;         
  ULONG service_id;
  BYTE mov_edx;         
  ULONG stub;
  USHORT call_ptr_edx;  
  BYTE ret;             
  USHORT num_params;
  BYTE nop;
};


struct ServiceEntryW8 {
  
  
  
  
  
  
  
  
  BYTE mov_eax;         
  ULONG service_id;
  BYTE call_eip;        
  ULONG call_offset;
  BYTE ret_p;           
  USHORT num_params;
  USHORT mov_edx_esp;   
  USHORT sysenter;      
  BYTE ret;             
  USHORT nop;
};


struct Wow64Entry {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  BYTE mov_eax;         
  ULONG service_id;
  USHORT xor_ecx;       
  ULONG lea_edx;        
  ULONG call_fs1;       
  USHORT call_fs2;      
  BYTE call_fs3;        
  BYTE add_esp1;        
  USHORT add_esp2;      
  BYTE ret;             
  USHORT num_params;
};


struct Wow64EntryW8 {
  
  
  
  
  BYTE mov_eax;         
  ULONG service_id;
  ULONG call_fs1;       
  USHORT call_fs2;      
  BYTE call_fs3;        
  BYTE ret;             
  USHORT num_params;
  BYTE nop;
};


const size_t kMinServiceSize = offsetof(ServiceEntry, ret);
COMPILE_ASSERT(sizeof(ServiceEntryW8) >= kMinServiceSize, wrong_service_len);
COMPILE_ASSERT(sizeof(Wow64Entry) >= kMinServiceSize, wrong_service_len);
COMPILE_ASSERT(sizeof(Wow64EntryW8) >= kMinServiceSize, wrong_service_len);

struct ServiceFullThunk {
  union {
    ServiceEntry original;
    ServiceEntryW8 original_w8;
    Wow64Entry wow_64;
    Wow64EntryW8 wow_64_w8;
  };
  int internal_thunk;  
};

#pragma pack(pop)

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

  relative_jump_ = 0;
  size_t thunk_bytes = GetThunkSize();
  scoped_ptr<char[]> thunk_buffer(new char[thunk_bytes]);
  ServiceFullThunk* thunk = reinterpret_cast<ServiceFullThunk*>(
                                thunk_buffer.get());

  if (!IsFunctionAService(&thunk->original) &&
      (!relaxed_ || !SaveOriginalFunction(&thunk->original, thunk_storage)))
    return STATUS_UNSUCCESSFUL;

  ret = PerformPatch(thunk, thunk_storage);

  if (NULL != storage_used)
    *storage_used = thunk_bytes;

  return ret;
}

size_t ServiceResolverThunk::GetThunkSize() const {
  return offsetof(ServiceFullThunk, internal_thunk) + GetInternalThunkSize();
}

bool ServiceResolverThunk::IsFunctionAService(void* local_thunk) const {
  ServiceEntry function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kMovEax != function_code.mov_eax ||
      kMovEdx != function_code.mov_edx ||
      (kCallPtrEdx != function_code.call_ptr_edx &&
       kCallEdx != function_code.call_ptr_edx) ||
      kRet != function_code.ret)
    return false;

  
  if (kCallEdx != function_code.call_ptr_edx) {
    DWORD ki_system_call;
    if (!::ReadProcessMemory(process_,
                             bit_cast<const void*>(function_code.stub),
                             &ki_system_call, sizeof(ki_system_call), &read))
      return false;

    if (sizeof(ki_system_call) != read)
      return false;

    HMODULE module_1, module_2;
    
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           bit_cast<const wchar_t*>(ki_system_call), &module_1))
      return false;

    if (NULL != ntdll_base_) {
      
      
      module_2 = ntdll_base_;
    } else {
      if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                 GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             reinterpret_cast<const wchar_t*>(target_),
                             &module_2))
        return false;
    }

    if (module_1 != module_2)
      return false;
  }

  
  memcpy(local_thunk, &function_code, sizeof(function_code));

  return true;
}

NTSTATUS ServiceResolverThunk::PerformPatch(void* local_thunk,
                                            void* remote_thunk) {
  ServiceEntry intercepted_code;
  size_t bytes_to_write = sizeof(intercepted_code);
  ServiceFullThunk *full_local_thunk = reinterpret_cast<ServiceFullThunk*>(
      local_thunk);
  ServiceFullThunk *full_remote_thunk = reinterpret_cast<ServiceFullThunk*>(
      remote_thunk);

  
  memcpy(&intercepted_code, &full_local_thunk->original,
         sizeof(intercepted_code));
  intercepted_code.mov_eax = kMovEax;
  intercepted_code.service_id = full_local_thunk->original.service_id;
  intercepted_code.mov_edx = kMovEdx;
  intercepted_code.stub = bit_cast<ULONG>(&full_remote_thunk->internal_thunk);
  intercepted_code.call_ptr_edx = kJmpEdx;
  bytes_to_write = kMinServiceSize;

  if (relative_jump_) {
    intercepted_code.mov_eax = kJmp32;
    intercepted_code.service_id = relative_jump_;
    bytes_to_write = offsetof(ServiceEntry, mov_edx);
  }

  
  SetInternalThunk(&full_local_thunk->internal_thunk, GetInternalThunkSize(),
                   remote_thunk, interceptor_);

  size_t thunk_size = GetThunkSize();

  
  SIZE_T written;
  if (!::WriteProcessMemory(process_, remote_thunk, local_thunk,
                            thunk_size, &written))
    return STATUS_UNSUCCESSFUL;

  if (thunk_size != written)
    return STATUS_UNSUCCESSFUL;

  
  if (NULL != ntdll_base_) {
    
    if (!::WriteProcessMemory(process_, target_, &intercepted_code,
                              bytes_to_write, &written))
      return STATUS_UNSUCCESSFUL;
  } else {
    if (!WriteProtectedChildMemory(process_, target_, &intercepted_code,
                                   bytes_to_write))
      return STATUS_UNSUCCESSFUL;
  }

  return STATUS_SUCCESS;
}

bool ServiceResolverThunk::SaveOriginalFunction(void* local_thunk,
                                                void* remote_thunk) {
  ServiceEntry function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kJmp32 == function_code.mov_eax) {
    
    ULONG relative = function_code.service_id;

    
    relative += bit_cast<ULONG>(target_) - bit_cast<ULONG>(remote_thunk);

    function_code.service_id = relative;

    
    ServiceFullThunk *full_thunk =
        reinterpret_cast<ServiceFullThunk*>(remote_thunk);

    const ULONG kJmp32Size = 5;

    relative_jump_ = bit_cast<ULONG>(&full_thunk->internal_thunk) -
                     bit_cast<ULONG>(target_) - kJmp32Size;
  }

  
  memcpy(local_thunk, &function_code, sizeof(function_code));

  return true;
}

bool Wow64ResolverThunk::IsFunctionAService(void* local_thunk) const {
  Wow64Entry function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kMovEax != function_code.mov_eax || kXorEcx != function_code.xor_ecx ||
      kLeaEdx != function_code.lea_edx || kCallFs1 != function_code.call_fs1 ||
      kCallFs2 != function_code.call_fs2 || kCallFs3 != function_code.call_fs3)
    return false;

  if ((kAddEsp1 == function_code.add_esp1 &&
       kAddEsp2 == function_code.add_esp2 &&
       kRet == function_code.ret) || kRet == function_code.add_esp1) {
    
    memcpy(local_thunk, &function_code, sizeof(function_code));
    return true;
  }

  return false;
}

bool Wow64W8ResolverThunk::IsFunctionAService(void* local_thunk) const {
  Wow64EntryW8 function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kMovEax != function_code.mov_eax || kCallFs1 != function_code.call_fs1 ||
      kCallFs2 != function_code.call_fs2 ||
      kCallFs3 != function_code.call_fs3 || kRet != function_code.ret) {
    return false;
  }

  
  memcpy(local_thunk, &function_code, sizeof(function_code));
  return true;
}

bool Win2kResolverThunk::IsFunctionAService(void* local_thunk) const {
  ServiceEntry function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kMovEax != function_code.mov_eax ||
      function_code.service_id > kMaxService)
    return false;

  
  memcpy(local_thunk, &function_code, sizeof(function_code));

  return true;
}

bool Win8ResolverThunk::IsFunctionAService(void* local_thunk) const {
  ServiceEntryW8 function_code;
  SIZE_T read;
  if (!::ReadProcessMemory(process_, target_, &function_code,
                           sizeof(function_code), &read))
    return false;

  if (sizeof(function_code) != read)
    return false;

  if (kMovEax != function_code.mov_eax || kCallEip != function_code.call_eip ||
      function_code.call_offset != 3 || kRet != function_code.ret_p ||
      kMovEdxEsp != function_code.mov_edx_esp ||
      kSysenter != function_code.sysenter || kRet2 != function_code.ret) {
    return false;
  }

  
  memcpy(local_thunk, &function_code, sizeof(function_code));

  return true;
}

}  





#include "sandbox/win/src/resolver.h"



#include <new>

#include "sandbox/win/src/sandbox_nt_util.h"

namespace {

const BYTE kPushRax = 0x50;
const USHORT kMovRax = 0xB848;
const ULONG kMovRspRax = 0x24048948;
const BYTE kRetNp = 0xC3;

#pragma pack(push, 1)
struct InternalThunk {
  
  
  
  
  
  
  
  

  InternalThunk() {
    push_rax = kPushRax;
    mov_rax = kMovRax;
    interceptor_function = 0;
    mov_rsp_rax = kMovRspRax;
    ret = kRetNp;
  };
  BYTE push_rax;        
  USHORT mov_rax;       
  ULONG_PTR interceptor_function;
  ULONG mov_rsp_rax;    
  BYTE ret;             
};
#pragma pack(pop)

} 

namespace sandbox {

size_t ResolverThunk::GetInternalThunkSize() const {
  return sizeof(InternalThunk);
}

bool ResolverThunk::SetInternalThunk(void* storage, size_t storage_bytes,
                                     const void* original_function,
                                     const void* interceptor) {
  if (storage_bytes < sizeof(InternalThunk))
    return false;

  InternalThunk* thunk = new(storage) InternalThunk;
  thunk->interceptor_function = reinterpret_cast<ULONG_PTR>(interceptor);

  return true;
}

NTSTATUS ResolverThunk::ResolveTarget(const void* module,
                                      const char* function_name,
                                      void** address) {
  
  return STATUS_NOT_IMPLEMENTED;
}

}  





#include "sandbox/win/src/resolver.h"



#include <new>

#include "sandbox/win/src/sandbox_nt_util.h"

namespace {

#pragma pack(push, 1)
struct InternalThunk {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  InternalThunk() {
    opcodes_1 = 0x5208ec83;
    opcodes_2 = 0x0c24548b;
    opcodes_3 = 0x08245489;
    opcodes_4 = 0x0c2444c7;
    opcodes_5 = 0x042444c7;
    opcodes_6 = 0xc35a;
    extra_argument = 0;
    interceptor_function = 0;
  };
  ULONG opcodes_1;         
  ULONG opcodes_2;         
  ULONG opcodes_3;         
  ULONG opcodes_4;         
  ULONG extra_argument;
  ULONG opcodes_5;         
  ULONG interceptor_function;
  USHORT opcodes_6;         
};
#pragma pack(pop)

};  

namespace sandbox {

bool ResolverThunk::SetInternalThunk(void* storage, size_t storage_bytes,
                                     const void* original_function,
                                     const void* interceptor) {
  if (storage_bytes < sizeof(InternalThunk))
    return false;

  InternalThunk* thunk = new(storage) InternalThunk;

#pragma warning(push)
#pragma warning(disable: 4311)
  
  thunk->interceptor_function = reinterpret_cast<ULONG>(interceptor);
  thunk->extra_argument = reinterpret_cast<ULONG>(original_function);
#pragma warning(pop)

  return true;
}

size_t ResolverThunk::GetInternalThunkSize() const {
  return sizeof(InternalThunk);
}

NTSTATUS ResolverThunk::ResolveTarget(const void* module,
                                      const char* function_name,
                                      void** address) {
  const void** casted = const_cast<const void**>(address);
  return ResolverThunk::ResolveInterceptor(module, function_name, casted);
}

}  

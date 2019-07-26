







#ifndef SANDBOX_SRC_INTERCEPTION_INTERNAL_H_
#define SANDBOX_SRC_INTERCEPTION_INTERNAL_H_

#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

const int kMaxThunkDataBytes = 64;

enum InterceptorId;





#pragma pack(push, 4)




struct FunctionInfo {
  size_t record_bytes;            
  InterceptionType type;
  InterceptorId id;
  const void* interceptor_address;
  char function[1];               
  
};


struct DllPatchInfo {
  size_t record_bytes;            
  size_t offset_to_functions;
  int num_functions;
  bool unload_module;
  wchar_t dll_name[1];            
  
};


struct SharedMemory {
  int num_intercepted_dlls;
  void* interceptor_base;
  DllPatchInfo dll_list[1];       
};


struct ThunkData {
  char data[kMaxThunkDataBytes];
};


struct DllInterceptionData {
  size_t data_bytes;
  size_t used_bytes;
  void* base;
  int num_thunks;
#if defined(_WIN64)
  int dummy;                      
#endif
  ThunkData thunks[1];
};

#pragma pack(pop)

}  

#endif  

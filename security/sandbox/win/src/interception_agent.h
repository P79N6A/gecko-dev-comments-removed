







#ifndef SANDBOX_SRC_INTERCEPTION_AGENT_H__
#define SANDBOX_SRC_INTERCEPTION_AGENT_H__

#include "base/basictypes.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {


struct DllInterceptionData;
struct SharedMemory;
struct DllPatchInfo;

class ResolverThunk;












class InterceptionAgent {
 public:
  
  static InterceptionAgent* GetInterceptionAgent();

  
  
  
  
  
  
  
  bool OnDllLoad(const UNICODE_STRING* full_path, const UNICODE_STRING* name,
                 void* base_address);

  
  void OnDllUnload(void* base_address);

 private:
  ~InterceptionAgent() {}

  
  bool Init(SharedMemory* shared_memory);

  
  
  bool DllMatch(const UNICODE_STRING* full_path, const UNICODE_STRING* name,
                const DllPatchInfo* dll_info);

  
  
  
  
  bool PatchDll(const DllPatchInfo* dll_info, DllInterceptionData* thunks);

  
  ResolverThunk* GetResolver(InterceptionType type);

  
  SharedMemory* interceptions_;

  
  
  
  DllInterceptionData* dlls_[1];

  DISALLOW_IMPLICIT_CONSTRUCTORS(InterceptionAgent);
};

}  

#endif  

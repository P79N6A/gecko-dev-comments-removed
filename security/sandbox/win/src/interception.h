







#ifndef SANDBOX_SRC_INTERCEPTION_H_
#define SANDBOX_SRC_INTERCEPTION_H_

#include <list>
#include <string>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

class TargetProcess;
enum InterceptorId;


struct DllPatchInfo;
struct DllInterceptionData;































class InterceptionManager {
  
  
  
  FRIEND_TEST_ALL_PREFIXES(InterceptionManagerTest, BufferLayout1);
  FRIEND_TEST_ALL_PREFIXES(InterceptionManagerTest, BufferLayout2);

 public:
  
  
  
  
  InterceptionManager(TargetProcess* child_process, bool relaxed);
  ~InterceptionManager();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool AddToPatchedFunctions(const wchar_t* dll_name,
                             const char* function_name,
                             InterceptionType interception_type,
                             const void* replacement_code_address,
                             InterceptorId id);

  
  
  bool AddToPatchedFunctions(const wchar_t* dll_name,
                             const char* function_name,
                             InterceptionType interception_type,
                             const char* replacement_function_name,
                             InterceptorId id);

  
  bool AddToUnloadModules(const wchar_t* dll_name);

  
  
  
  
  
  
  
  
  
  
  bool InitializeInterceptions();

 private:
  
  struct InterceptionData {
    InterceptionType type;            
    InterceptorId id;                 
    base::string16 dll;               
    std::string function;             
    std::string interceptor;          
    const void* interceptor_address;  
  };

  
  size_t GetBufferSize() const;

  
  
  
  static inline size_t RoundUpToMultiple(size_t value, size_t alignment) {
    return ((value + alignment -1) / alignment) * alignment;
  }

  
  
  
  
  
  bool SetupConfigBuffer(void* buffer, size_t buffer_bytes);

  
  
  
  
  
  
  
  
  
  bool SetupDllInfo(const InterceptionData& data,
                    void** buffer, size_t* buffer_bytes) const;

  
  
  
  
  
  bool SetupInterceptionInfo(const InterceptionData& data, void** buffer,
                             size_t* buffer_bytes,
                             DllPatchInfo* dll_info) const;

  
  
  bool IsInterceptionPerformedByChild(const InterceptionData& data) const;

  
  
  
  bool CopyDataToChild(const void* local_buffer, size_t buffer_bytes,
                       void** remote_buffer) const;

  
  
  
  
  
  bool PatchNtdll(bool hot_patch_needed);

  
  
  
  
  bool PatchClientFunctions(DllInterceptionData* thunks,
                            size_t thunk_bytes,
                            DllInterceptionData* dll_data);

  
  TargetProcess* child_;
  
  
  std::list<InterceptionData> interceptions_;

  
  bool names_used_;

  
  bool relaxed_;

  DISALLOW_COPY_AND_ASSIGN(InterceptionManager);
};







#if SANDBOX_EXPORTS
#if defined(_WIN64)
#define MAKE_SERVICE_NAME(service, params) "Target" # service "64"
#else
#define MAKE_SERVICE_NAME(service, params) "_Target" # service "@" # params
#endif

#define ADD_NT_INTERCEPTION(service, id, num_params) \
  AddToPatchedFunctions(kNtdllName, #service, \
                        sandbox::INTERCEPTION_SERVICE_CALL, \
                        MAKE_SERVICE_NAME(service, num_params), id)

#define INTERCEPT_NT(manager, service, id, num_params) \
  ((&Target##service) ? \
    manager->ADD_NT_INTERCEPTION(service, id, num_params) : false)





#define INTERCEPT_EAT(manager, dll, function, id, num_params) \
  ((&Target##function) ? \
    manager->AddToPatchedFunctions(dll, #function, sandbox::INTERCEPTION_EAT, \
                                   MAKE_SERVICE_NAME(function, num_params), \
                                   id) : \
    false)
#else  
#if defined(_WIN64)
#define MAKE_SERVICE_NAME(service) &Target##service##64
#else
#define MAKE_SERVICE_NAME(service) &Target##service
#endif

#define ADD_NT_INTERCEPTION(service, id, num_params) \
  AddToPatchedFunctions(kNtdllName, #service, \
                        sandbox::INTERCEPTION_SERVICE_CALL, \
                        MAKE_SERVICE_NAME(service), id)

#define INTERCEPT_NT(manager, service, id, num_params) \
  manager->ADD_NT_INTERCEPTION(service, id, num_params)





#define INTERCEPT_EAT(manager, dll, function, id, num_params) \
  manager->AddToPatchedFunctions(dll, #function, sandbox::INTERCEPTION_EAT, \
                                 MAKE_SERVICE_NAME(function), id)
#endif  

}  

#endif  

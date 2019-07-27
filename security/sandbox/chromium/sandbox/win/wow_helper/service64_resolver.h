



#ifndef SANDBOX_WOW_HELPER_SERVICE64_RESOLVER_H__
#define SANDBOX_WOW_HELPER_SERVICE64_RESOLVER_H__

#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/resolver.h"

namespace sandbox {



class Service64ResolverThunk : public ResolverThunk {
 public:
  
  explicit Service64ResolverThunk(HANDLE process)
      : process_(process), ntdll_base_(NULL) {}
  virtual ~Service64ResolverThunk() {}

  
  virtual NTSTATUS Setup(const void* target_module,
                         const void* interceptor_module,
                         const char* target_name,
                         const char* interceptor_name,
                         const void* interceptor_entry_point,
                         void* thunk_storage,
                         size_t storage_bytes,
                         size_t* storage_used);

  
  virtual NTSTATUS ResolveInterceptor(const void* module,
                                      const char* function_name,
                                      const void** address);

  
  virtual NTSTATUS ResolveTarget(const void* module,
                                 const char* function_name,
                                 void** address);

  
  virtual size_t GetThunkSize() const;

 protected:
  
  HMODULE ntdll_base_;

  
  HANDLE process_;

 private:
  
  
  
  virtual bool IsFunctionAService(void* local_thunk) const;

  
  
  
  
  
  
  virtual NTSTATUS PerformPatch(void* local_thunk, void* remote_thunk);

  DISALLOW_COPY_AND_ASSIGN(Service64ResolverThunk);
};

}  


#endif  

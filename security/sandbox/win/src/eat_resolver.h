



#ifndef SANDBOX_SRC_EAT_RESOLVER_H__
#define SANDBOX_SRC_EAT_RESOLVER_H__

#include "base/basictypes.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/resolver.h"

namespace sandbox {


class EatResolverThunk : public ResolverThunk {
 public:
  EatResolverThunk() : eat_entry_(NULL) {}
  virtual ~EatResolverThunk() {}

  
  virtual NTSTATUS Setup(const void* target_module,
                         const void* interceptor_module,
                         const char* target_name,
                         const char* interceptor_name,
                         const void* interceptor_entry_point,
                         void* thunk_storage,
                         size_t storage_bytes,
                         size_t* storage_used);

  
  virtual NTSTATUS ResolveTarget(const void* module,
                                 const char* function_name,
                                 void** address);

  
  virtual size_t GetThunkSize() const;

 private:
  
  DWORD* eat_entry_;

  DISALLOW_COPY_AND_ASSIGN(EatResolverThunk);
};

}  


#endif  

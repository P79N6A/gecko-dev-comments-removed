



#ifndef SANDBOX_SRC_SIDESTEP_RESOLVER_H__
#define SANDBOX_SRC_SIDESTEP_RESOLVER_H__

#include "base/basictypes.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/resolver.h"

namespace sandbox {


class SidestepResolverThunk : public ResolverThunk {
 public:
  SidestepResolverThunk() {}
  virtual ~SidestepResolverThunk() {}

  
  virtual NTSTATUS Setup(const void* target_module,
                         const void* interceptor_module,
                         const char* target_name,
                         const char* interceptor_name,
                         const void* interceptor_entry_point,
                         void* thunk_storage,
                         size_t storage_bytes,
                         size_t* storage_used);

  
  virtual size_t GetThunkSize() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(SidestepResolverThunk);
};





class SmartSidestepResolverThunk : public SidestepResolverThunk {
 public:
  SmartSidestepResolverThunk() {}
  virtual ~SmartSidestepResolverThunk() {}

  
  virtual NTSTATUS Setup(const void* target_module,
                         const void* interceptor_module,
                         const char* target_name,
                         const char* interceptor_name,
                         const void* interceptor_entry_point,
                         void* thunk_storage,
                         size_t storage_bytes,
                         size_t* storage_used);

  
  virtual size_t GetThunkSize() const;

 private:
  
  
  static void SmartStub();

  
  static bool IsInternalCall(const void* base, void* return_address);

  DISALLOW_COPY_AND_ASSIGN(SmartSidestepResolverThunk);
};

}  


#endif  

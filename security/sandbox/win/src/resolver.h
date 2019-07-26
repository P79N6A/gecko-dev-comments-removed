







#include "base/basictypes.h"
#include "sandbox/win/src/nt_internals.h"

#ifndef SANDBOX_SRC_RESOLVER_H__
#define SANDBOX_SRC_RESOLVER_H__

namespace sandbox {




class ResolverThunk {
 public:
  ResolverThunk() {}
  virtual ~ResolverThunk() {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual NTSTATUS Setup(const void* target_module,
                         const void* interceptor_module,
                         const char* target_name,
                         const char* interceptor_name,
                         const void* interceptor_entry_point,
                         void* thunk_storage,
                         size_t storage_bytes,
                         size_t* storage_used) = 0;

  
  virtual NTSTATUS ResolveInterceptor(const void* module,
                                      const char* function_name,
                                      const void** address);

  
  virtual NTSTATUS ResolveTarget(const void* module,
                                 const char* function_name,
                                 void** address);

  
  virtual size_t GetThunkSize() const = 0;

 protected:
  
  
  
  
  
  
  
  
  
  
  virtual NTSTATUS Init(const void* target_module,
                        const void* interceptor_module,
                        const char* target_name,
                        const char* interceptor_name,
                        const void* interceptor_entry_point,
                        void* thunk_storage,
                        size_t storage_bytes);

  
  size_t GetInternalThunkSize() const;

  
  
  bool SetInternalThunk(void* storage, size_t storage_bytes,
                        const void* original_function, const void* interceptor);

  
  void* target_;
  
  const void* interceptor_;

  DISALLOW_COPY_AND_ASSIGN(ResolverThunk);
};

}  

#endif  

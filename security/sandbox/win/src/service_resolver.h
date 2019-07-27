



#ifndef SANDBOX_SRC_SERVICE_RESOLVER_H__
#define SANDBOX_SRC_SERVICE_RESOLVER_H__

#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/resolver.h"

namespace sandbox {



class ServiceResolverThunk : public ResolverThunk {
 public:
  
  ServiceResolverThunk(HANDLE process, bool relaxed)
      : process_(process), ntdll_base_(NULL),
        relaxed_(relaxed), relative_jump_(0) {}
  virtual ~ServiceResolverThunk() {}

  
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

  
  virtual void AllowLocalPatches();

  
  
  
  virtual NTSTATUS CopyThunk(const void* target_module,
                             const char* target_name,
                             BYTE* thunk_storage,
                             size_t storage_bytes,
                             size_t* storage_used);

 protected:
  
  HMODULE ntdll_base_;

  
  HANDLE process_;

 private:
  
  
  
  virtual bool IsFunctionAService(void* local_thunk) const;

  
  
  
  
  
  
  virtual NTSTATUS PerformPatch(void* local_thunk, void* remote_thunk);

  
  
  
  bool SaveOriginalFunction(void* local_thunk, void* remote_thunk);

  
  bool relaxed_;
  ULONG relative_jump_;

  DISALLOW_COPY_AND_ASSIGN(ServiceResolverThunk);
};



class Wow64ResolverThunk : public ServiceResolverThunk {
 public:
  
  Wow64ResolverThunk(HANDLE process, bool relaxed)
      : ServiceResolverThunk(process, relaxed) {}
  virtual ~Wow64ResolverThunk() {}

 private:
  virtual bool IsFunctionAService(void* local_thunk) const;

  DISALLOW_COPY_AND_ASSIGN(Wow64ResolverThunk);
};



class Wow64W8ResolverThunk : public ServiceResolverThunk {
 public:
  
  Wow64W8ResolverThunk(HANDLE process, bool relaxed)
      : ServiceResolverThunk(process, relaxed) {}
  virtual ~Wow64W8ResolverThunk() {}

 private:
  virtual bool IsFunctionAService(void* local_thunk) const;

  DISALLOW_COPY_AND_ASSIGN(Wow64W8ResolverThunk);
};



class Win8ResolverThunk : public ServiceResolverThunk {
 public:
  
  Win8ResolverThunk(HANDLE process, bool relaxed)
      : ServiceResolverThunk(process, relaxed) {}
  virtual ~Win8ResolverThunk() {}

 private:
  virtual bool IsFunctionAService(void* local_thunk) const;

  DISALLOW_COPY_AND_ASSIGN(Win8ResolverThunk);
};

}  


#endif  

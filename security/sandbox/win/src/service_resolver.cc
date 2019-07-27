



#include "sandbox/win/src/service_resolver.h"

#include "base/win/pe_image.h"
#include "sandbox/win/src/internal_types.h"
#include "sandbox/win/src/sandbox_nt_util.h"

namespace sandbox {

NTSTATUS ServiceResolverThunk::ResolveInterceptor(
    const void* interceptor_module,
    const char* interceptor_name,
    const void** address) {
  
  
  return ResolveTarget(interceptor_module, interceptor_name,
                       const_cast<void**>(address));
}



NTSTATUS ServiceResolverThunk::ResolveTarget(const void* module,
                                             const char* function_name,
                                             void** address) {
  if (NULL == module)
    return STATUS_UNSUCCESSFUL;

  base::win::PEImage module_image(module);
  *address = module_image.GetProcAddress(function_name);

  if (NULL == *address) {
    NOTREACHED_NT();
    return STATUS_UNSUCCESSFUL;
  }

  return STATUS_SUCCESS;
}

void ServiceResolverThunk::AllowLocalPatches() {
  ntdll_base_ = ::GetModuleHandle(kNtdllName);
}

}  

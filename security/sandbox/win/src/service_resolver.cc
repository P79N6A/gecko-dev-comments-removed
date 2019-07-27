



#include "sandbox/win/src/service_resolver.h"

#include "base/logging.h"
#include "base/win/pe_image.h"

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
  DCHECK(address);
  if (NULL == module)
    return STATUS_UNSUCCESSFUL;

  base::win::PEImage module_image(module);
  *address = module_image.GetProcAddress(function_name);

  if (NULL == *address) {
    NOTREACHED();
    return STATUS_UNSUCCESSFUL;
  }

  return STATUS_SUCCESS;
}

}  

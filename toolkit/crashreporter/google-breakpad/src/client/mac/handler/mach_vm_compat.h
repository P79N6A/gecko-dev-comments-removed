




























#ifndef CLIENT_MAC_GENERATOR_MACH_VM_COMPAT_H_
#define CLIENT_MAC_GENERATOR_MACH_VM_COMPAT_H_

#include <TargetConditionals.h>



#if TARGET_OS_IPHONE
#include <mach/vm_map.h>
#define mach_vm_address_t vm_address_t
#define mach_vm_deallocate vm_deallocate
#define mach_vm_read vm_read
#define mach_vm_region vm_region
#define mach_vm_region_recurse vm_region_recurse
#define mach_vm_size_t vm_size_t
#else
#include <mach/mach_vm.h>
#endif  

#endif  






























#ifndef COMMON_MAC_BOOTSTRAP_COMPAT_H_
#define COMMON_MAC_BOOTSTRAP_COMPAT_H_

#include <servers/bootstrap.h>

namespace breakpad {












kern_return_t BootstrapRegister(mach_port_t bp,
                                name_t service_name,
                                mach_port_t sp);

}  

#endif  

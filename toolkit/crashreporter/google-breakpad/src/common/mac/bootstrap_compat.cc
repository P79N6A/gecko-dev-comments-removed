




























#include "common/mac/bootstrap_compat.h"

namespace breakpad {

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
kern_return_t BootstrapRegister(mach_port_t bp,
                                name_t service_name,
                                mach_port_t sp) {
  return bootstrap_register(bp, service_name, sp);
}
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

}  

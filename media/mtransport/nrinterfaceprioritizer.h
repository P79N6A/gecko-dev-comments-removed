


#ifndef nrinterfacepriority_h__
#define nrinterfacepriority_h__

extern "C" {
#include "nr_api.h"
#include "nr_interface_prioritizer.h"
}

namespace mozilla {

nr_interface_prioritizer* CreateInterfacePrioritizer();

} 

#endif





#ifndef RLZ_LIB_MACHINE_ID_H_
#define RLZ_LIB_MACHINE_ID_H_

#include "base/string16.h"

#include <string>

namespace rlz_lib {




bool GetRawMachineId(string16* data, int* more_data);

}  

#endif  

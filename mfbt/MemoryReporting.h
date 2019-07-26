







#ifndef mozilla_MemoryReporting_h_
#define mozilla_MemoryReporting_h_

#include <stddef.h>

#ifdef __cplusplus

namespace mozilla {





typedef size_t (*MallocSizeOf)(const void* p);

} 

#endif 

typedef size_t (*MozMallocSizeOf)(const void* p);

#endif 

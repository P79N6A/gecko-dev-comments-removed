








































#ifndef nsIAllocator_h___
#define nsIAllocator_h___

#include "nsMemory.h"

#define nsIAllocator            nsIMemory
#define nsAllocator             nsMemory
#define GetGlobalAllocator      GetGlobalMemoryService

#define NS_IALLOCATOR_IID       NS_GET_IID(nsIMemory)
#define NS_ALLOCATOR_CONTRACTID NS_MEMORY_CONTRACTID
#define NS_ALLOCATOR_CLASSNAME  NS_MEMORY_CLASSNAME
#define NS_ALLOCATOR_CID        NS_MEMORY_CID

#endif 

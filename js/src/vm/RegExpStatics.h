





#ifndef vm_RegExpStatics_h
#define vm_RegExpStatics_h

#include "mozilla/MemoryReporting.h"

#include <stddef.h>

#include "jspubtd.h"

#include "js/Utility.h"

namespace js {

class PreserveRegExpStatics;
class RegExpStatics;

size_t SizeOfRegExpStaticsData(const JSObject *obj, mozilla::MallocSizeOf mallocSizeOf);

} 

#endif 

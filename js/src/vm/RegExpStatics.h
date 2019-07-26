





#ifndef RegExpStatics_h__
#define RegExpStatics_h__

#include <stddef.h>

#include "jspubtd.h"

#include "js/Utility.h"

namespace js {

class PreserveRegExpStatics;
class RegExpStatics;

size_t SizeOfRegExpStaticsData(const JSObject *obj, JSMallocSizeOfFun mallocSizeOf);

} 

#endif 

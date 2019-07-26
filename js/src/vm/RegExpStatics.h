






#ifndef RegExpStatics_h__
#define RegExpStatics_h__

#include "mozilla/GuardObjects.h"

#include "jscntxt.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/Vector.h"

#include "vm/MatchPairs.h"
#include "vm/RegExpObject.h"

namespace js {

class PreserveRegExpStatics;
class RegExpStatics;

size_t SizeOfRegExpStaticsData(const JSObject *obj, JSMallocSizeOfFun mallocSizeOf);

} 

#endif 

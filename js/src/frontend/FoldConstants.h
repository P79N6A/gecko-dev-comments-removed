







































#ifndef FoldConstants_h__
#define FoldConstants_h__

#include "jsprvtd.h"

namespace js {

bool
FoldConstants(JSContext *cx, ParseNode *pn, TreeContext *tc, bool inCond = false);

} 

#endif 

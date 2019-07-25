






#ifndef FoldConstants_h__
#define FoldConstants_h__

#include "jsprvtd.h"

namespace js {

bool
FoldConstants(JSContext *cx, ParseNode *pn, Parser *parser, bool inGenexpLambda = false,
              bool inCond = false);

} 

#endif 

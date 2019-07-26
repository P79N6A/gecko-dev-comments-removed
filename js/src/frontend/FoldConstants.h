






#ifndef FoldConstants_h__
#define FoldConstants_h__

#include "jsprvtd.h"

namespace js {
namespace frontend {














template <typename ParseHandler>
bool
FoldConstants(JSContext *cx, typename ParseHandler::Node *pnp,
              Parser<ParseHandler> *parser,
              bool inGenexpLambda = false, bool inCond = false);

} 
} 

#endif 

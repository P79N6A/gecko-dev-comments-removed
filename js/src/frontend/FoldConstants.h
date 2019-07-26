





#ifndef frontend_FoldConstants_h
#define frontend_FoldConstants_h

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

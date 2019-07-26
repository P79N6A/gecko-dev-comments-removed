





#ifndef frontend_FoldConstants_h
#define frontend_FoldConstants_h

#include "jsprvtd.h"

#include "frontend/SyntaxParseHandler.h"

namespace js {
namespace frontend {














bool
FoldConstants(JSContext *cx, ParseNode **pnp, Parser<FullParseHandler> *parser);

inline bool
FoldConstants(JSContext *cx, SyntaxParseHandler::Node *pnp, Parser<SyntaxParseHandler> *parser)
{
    return true;
}

} 
} 

#endif 

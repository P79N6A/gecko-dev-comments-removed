





#ifndef frontend_FoldConstants_h
#define frontend_FoldConstants_h

#include "jsprvtd.h"

#include "frontend/SyntaxParseHandler.h"

namespace js {
namespace frontend {














bool
FoldConstants(ExclusiveContext *cx, ParseNode **pnp, Parser<FullParseHandler> *parser);

inline bool
FoldConstants(ExclusiveContext *cx, SyntaxParseHandler::Node *pnp,
              Parser<SyntaxParseHandler> *parser)
{
    return true;
}

} 
} 

#endif 

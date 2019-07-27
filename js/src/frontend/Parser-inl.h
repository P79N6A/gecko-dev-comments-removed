





#ifndef frontend_Parser_inl_h
#define frontend_Parser_inl_h

#include "frontend/Parser.h"

#include "frontend/ParseMaps-inl.h"

namespace js {
namespace frontend {

template <typename ParseHandler>
bool
ParseContext<ParseHandler>::init(TokenStream &ts)
{
    if (!frontend::GenerateBlockId(ts, this, this->bodyid))
        return false;

    return decls_.init() && lexdeps.ensureMap(sc->context);
}

template <typename ParseHandler>
ParseContext<ParseHandler>::~ParseContext()
{
    
    
    JS_ASSERT(*parserPC == this);
    *parserPC = this->oldpc;
}

} 
} 

#endif 

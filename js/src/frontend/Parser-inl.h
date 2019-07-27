





#ifndef frontend_Parser_inl_h
#define frontend_Parser_inl_h

#include "frontend/Parser.h"

#include "frontend/ParseMaps-inl.h"

namespace js {
namespace frontend {

template <typename ParseHandler>
bool
ParseContext<ParseHandler>::init(TokenStream& ts)
{
    if (!frontend::GenerateBlockId(ts, this, this->bodyid))
        return false;

    if (!decls_.init() || !lexdeps.ensureMap(sc->context)) {
        ReportOutOfMemory(sc->context);
        return false;
    }

    return true;
}

template <typename ParseHandler>
ParseContext<ParseHandler>::~ParseContext()
{
    
    
    MOZ_ASSERT(*parserPC == this);
    *parserPC = this->oldpc;
}

} 
} 

#endif 

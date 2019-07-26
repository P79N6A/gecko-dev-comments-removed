





#ifndef frontend_BytecodeEmitter_inl_h
#define frontend_BytecodeEmitter_inl_h

#include "frontend/BytecodeEmitter.h"
#include "frontend/Parser.h"

namespace js {
namespace frontend {

inline TokenStream *
BytecodeEmitter::tokenStream()
{
    return &parser->tokenStream;
}

} 
} 

#endif 







#ifndef BytecodeEmitter_inl_h__
#define BytecodeEmitter_inl_h__

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








#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

UnrootedScript
CompileScript(JSContext *cx, HandleObject scopeChain, AbstractFramePtr callerFrame,
              const CompileOptions &options, StableCharPtr chars, size_t length,
              JSString *source_ = NULL, unsigned staticLevel = 0);

bool
CompileFunctionBody(JSContext *cx, HandleFunction fun, CompileOptions options,
                    const AutoNameVector &formals, StableCharPtr chars, size_t length);

} 
} 

#endif 

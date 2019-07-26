






#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

RawScript
CompileScript(JSContext *cx, HandleObject scopeChain, HandleScript evalCaller,
              const CompileOptions &options, const jschar *chars, size_t length,
              JSString *source_ = NULL, unsigned staticLevel = 0,
              SourceCompressionToken *extraSct = NULL);

bool
ParseScript(JSContext *cx, HandleObject scopeChain,
            const CompileOptions &options, StableCharPtr chars, size_t length);

bool
CompileFunctionBody(JSContext *cx, HandleFunction fun, CompileOptions options,
                    const AutoNameVector &formals, const jschar *chars, size_t length);

} 
} 

#endif 







#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

JSScript *
CompileScript(JSContext *cx, HandleObject scopeChain, HandleScript evalCaller,
              const CompileOptions &options, const jschar *chars, size_t length,
              JSString *source_ = NULL, unsigned staticLevel = 0,
              SourceCompressionToken *extraSct = NULL);

bool
CompileLazyFunction(JSContext *cx, HandleFunction fun, LazyScript *lazy,
                    const jschar *chars, size_t length);

bool
CompileFunctionBody(JSContext *cx, MutableHandleFunction fun, CompileOptions options,
                    const AutoNameVector &formals, const jschar *chars, size_t length,
                    bool isAsmJSRecompile = false);

} 
} 

#endif 

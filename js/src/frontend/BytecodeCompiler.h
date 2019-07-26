






#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

JSScript *
CompileScript(JSContext *cx, HandleObject scopeChain, StackFrame *callerFrame,
              const CompileOptions &options, const jschar *chars, size_t length,
              JSString *source_ = NULL, unsigned staticLevel = 0);

bool
CompileFunctionBody(JSContext *cx, HandleFunction fun, CompileOptions options,
                    const AutoNameVector &formals, const jschar *chars, size_t length);

} 
} 

#endif 

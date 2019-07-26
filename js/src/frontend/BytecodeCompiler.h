






#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

JSScript *
CompileScript(JSContext *cx, HandleObject scopeChain, StackFrame *callerFrame,
              JSPrincipals *principals, JSPrincipals *originPrincipals,
              bool compileAndGo, bool noScriptRval,
              const jschar *chars, size_t length,
              const char *filename, unsigned lineno, JSVersion version,
              JSString *source_ = NULL, unsigned staticLevel = 0);

bool
CompileFunctionBody(JSContext *cx, HandleFunction fun,
                    JSPrincipals *principals, JSPrincipals *originPrincipals,
                    Bindings *bindings, const jschar *chars, size_t length,
                    const char *filename, unsigned lineno, JSVersion version);

} 
} 

#endif 

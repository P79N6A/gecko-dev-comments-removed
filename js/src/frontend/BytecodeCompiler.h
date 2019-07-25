







































#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

bool
CompileFunctionBody(JSContext *cx, JSFunction *fun,
                    JSPrincipals *principals, JSPrincipals *originPrincipals,
                    Bindings *bindings, const jschar *chars, size_t length,
                    const char *filename, unsigned lineno, JSVersion version);

JSScript *
CompileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
              JSPrincipals *principals, JSPrincipals *originPrincipals,
              bool compileAndGo, bool noScriptRval, bool needScriptGlobal,
              const jschar *chars, size_t length,
              const char *filename, unsigned lineno, JSVersion version,
              JSString *source = NULL, unsigned staticLevel = 0);

} 
} 

#endif 

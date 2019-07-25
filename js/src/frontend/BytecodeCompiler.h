







































#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

bool
CompileFunctionBody(JSContext *cx, JSFunction *fun,
                    JSPrincipals *principals, JSPrincipals *originPrincipals,
                    Bindings *bindings, const jschar *chars, size_t length,
                    const char *filename, uintN lineno, JSVersion version);

JSScript *
CompileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
              JSPrincipals *principals, JSPrincipals *originPrincipals,
              uint32_t tcflags, const jschar *chars, size_t length,
              const char *filename, uintN lineno, JSVersion version,
              JSString *source = NULL, uintN staticLevel = 0);

} 
} 

#endif 

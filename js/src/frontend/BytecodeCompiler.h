







































#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

bool
CompileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                    Bindings *bindings, const jschar *chars, size_t length,
                    const char *filename, uintN lineno, JSVersion version);

JSScript *
CompileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
              JSPrincipals *principals, uint32 tcflags,
              const jschar *chars, size_t length,
              const char *filename, uintN lineno, JSVersion version,
              JSString *source = NULL, uintN staticLevel = 0);

} 
} 

#endif 

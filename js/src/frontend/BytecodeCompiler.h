







































#ifndef BytecodeCompiler_h__
#define BytecodeCompiler_h__

#include "frontend/Parser.h"

namespace js {

struct BytecodeCompiler
{
    Parser      parser;

    BytecodeCompiler(JSContext *cx, JSPrincipals *prin = NULL, StackFrame *cfp = NULL);

    JSContext *context() {
        return parser.context;
    }

    bool init(const jschar *base, size_t length, const char *filename, uintN lineno,
              JSVersion version) {
        return parser.init(base, length, filename, lineno, version);
    }

    static bool
    compileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                        Bindings *bindings, const jschar *chars, size_t length,
                        const char *filename, uintN lineno, JSVersion version);

    static JSScript *
    compileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
                  JSPrincipals *principals, uint32 tcflags,
                  const jschar *chars, size_t length,
                  const char *filename, uintN lineno, JSVersion version,
                  JSString *source = NULL, uintN staticLevel = 0);

  private:
    static bool defineGlobals(JSContext *cx, GlobalScope &globalScope, JSScript *script);
};

} 

#endif 

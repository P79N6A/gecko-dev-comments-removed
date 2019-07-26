





#ifndef frontend_BytecodeCompiler_h
#define frontend_BytecodeCompiler_h

#include "jsapi.h"

class JSLinearString;

namespace js {

class AutoNameVector;
class LazyScript;
struct SourceCompressionToken;

namespace frontend {

JSScript *
CompileScript(JSContext *cx, HandleObject scopeChain, HandleScript evalCaller,
              const CompileOptions &options, const jschar *chars, size_t length,
              JSString *source_ = NULL, unsigned staticLevel = 0,
              SourceCompressionToken *extraSct = NULL);

bool
CompileLazyFunction(JSContext *cx, LazyScript *lazy, const jschar *chars, size_t length);

bool
CompileFunctionBody(JSContext *cx, MutableHandleFunction fun, CompileOptions options,
                    const AutoNameVector &formals, const jschar *chars, size_t length,
                    bool isAsmJSRecompile = false);










bool
IsIdentifier(JSLinearString *str);


bool
IsKeyword(JSLinearString *str);


void
MarkParser(JSTracer *trc, AutoGCRooter *parser);

} 
} 

#endif 

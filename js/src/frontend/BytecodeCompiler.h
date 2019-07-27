





#ifndef frontend_BytecodeCompiler_h
#define frontend_BytecodeCompiler_h

#include "NamespaceImports.h"

class JSLinearString;

namespace js {

class AutoNameVector;
class LazyScript;
class LifoAlloc;
class ScriptSourceObject;
struct SourceCompressionTask;

namespace frontend {

JSScript *
CompileScript(ExclusiveContext *cx, LifoAlloc *alloc,
              HandleObject scopeChain, HandleScript evalCaller,
              const ReadOnlyCompileOptions &options, SourceBufferHolder &srcBuf,
              JSString *source_ = nullptr, unsigned staticLevel = 0,
              SourceCompressionTask *extraSct = nullptr);

bool
CompileLazyFunction(JSContext *cx, Handle<LazyScript*> lazy, const char16_t *chars, size_t length);

bool
CompileFunctionBody(JSContext *cx, MutableHandleFunction fun,
                    const ReadOnlyCompileOptions &options,
                    const AutoNameVector &formals, JS::SourceBufferHolder &srcBuf);
bool
CompileStarGeneratorBody(JSContext *cx, MutableHandleFunction fun,
                         const ReadOnlyCompileOptions &options,
                         const AutoNameVector &formals, JS::SourceBufferHolder &srcBuf);

ScriptSourceObject *
CreateScriptSourceObject(ExclusiveContext *cx, const ReadOnlyCompileOptions &options);










bool
IsIdentifier(JSLinearString *str);


bool
IsKeyword(JSLinearString *str);


void
MarkParser(JSTracer *trc, JS::AutoGCRooter *parser);

} 
} 

#endif 

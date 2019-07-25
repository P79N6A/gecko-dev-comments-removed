












































#if !defined jsjaeger_retcon_h__ && defined JS_METHODJIT
#define jsjaeger_retcon_h__

#include "jscntxt.h"
#include "jsscript.h"
#include "MethodJIT.h"
#include "Compiler.h"

namespace js {
namespace mjit {







class AutoScriptRetrapper
{
  public:
    AutoScriptRetrapper(JSContext *cx, JSScript *script1) :
        script(script1), traps(cx) {};
    ~AutoScriptRetrapper();

    bool untrap(jsbytecode *pc);

  private:
    JSScript *script;
    Vector<jsbytecode*> traps;
};








class Recompiler {
    struct PatchableAddress {
        void **location;
        CallSite callSite;
    };

    struct PatchableNative {
        jsbytecode *pc;
        JSObject *guardedNative;
        JSC::ExecutablePool *pool;
        JSC::CodeLocationLabel nativeStart;
        JSC::CodeLocationJump nativeFunGuard;
        JSC::CodeLocationJump nativeJump;
    };

public:
    Recompiler(JSContext *cx, JSScript *script);
    
    bool recompile();

    static void
    expandInlineFrames(JSContext *cx, JSStackFrame *fp, mjit::CallSite *inlined,
                       JSStackFrame *next, VMFrame *f);

private:
    JSContext *cx;
    JSScript *script;

    static PatchableAddress findPatch(JITScript *jit, void **location);
    static void * findRejoin(JITScript *jit, const CallSite &callSite);

    static void applyPatch(JITScript *jit, PatchableAddress& toPatch);
    PatchableNative stealNative(JITScript *jit, jsbytecode *pc);
    void patchNative(JITScript *jit, PatchableNative &native);
    bool recompile(JSScript *script, bool isConstructing,
                   Vector<PatchableFrame> &frames,
                   Vector<PatchableAddress> &patches, Vector<CallSite> &sites,
                   Vector<PatchableNative> &natives);

    static JSStackFrame *
    expandInlineFrameChain(JSContext *cx, JSStackFrame *outer, InlineFrame *inner);

    
    bool cleanup(JITScript *jit, Vector<CallSite> *sites);
};

} 
} 

#endif


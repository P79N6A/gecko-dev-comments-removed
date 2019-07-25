












































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
public:
    Recompiler(JSContext *cx, JSScript *script);

    void recompile(bool resetUses = true);

    static void
    expandInlineFrames(JSCompartment *compartment, StackFrame *fp, mjit::CallSite *inlined,
                       StackFrame *next, VMFrame *f);

    static void patchFrame(JSCompartment *compartment, VMFrame *f, JSScript *script);

private:
    JSContext *cx;
    JSScript *script;

    static void patchCall(JITScript *jit, StackFrame *fp, void **location);
    static void patchNative(JSCompartment *compartment, JITScript *jit, StackFrame *fp,
                            jsbytecode *pc, CallSite *inlined, RejoinState rejoin);

    static StackFrame *
    expandInlineFrameChain(StackFrame *outer, InlineFrame *inner);

    
    static void cleanup(JITScript *jit);
};

} 
} 

#endif


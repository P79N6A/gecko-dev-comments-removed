











#if !defined jsjaeger_retcon_h__ && defined JS_METHODJIT
#define jsjaeger_retcon_h__

#include "jscntxt.h"
#include "jsscript.h"
#include "MethodJIT.h"
#include "Compiler.h"

namespace js {
namespace mjit {









class Recompiler {
public:

    
    
    static void
    clearStackReferences(FreeOp *fop, JSScript *script);

    static void
    expandInlineFrames(JSCompartment *compartment, StackFrame *fp, mjit::CallSite *inlined,
                       StackFrame *next, VMFrame *f);

    static void patchFrame(JSCompartment *compartment, VMFrame *f, JSScript *script);

private:

    static void patchCall(JITChunk *chunk, StackFrame *fp, void **location);
    static void patchNative(JSCompartment *compartment, JITChunk *chunk, StackFrame *fp,
                            jsbytecode *pc, RejoinState rejoin);

    static StackFrame *
    expandInlineFrameChain(StackFrame *outer, InlineFrame *inner);
};

} 
} 

#endif


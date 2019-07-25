







































#ifdef JS_METHODJIT

#include "Retcon.h"
#include "MethodJIT.h"
#include "Compiler.h"
#include "StubCalls.h"
#include "jsdbgapi.h"
#include "jsnum.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"

#include "jscntxtinlines.h"
#include "jsinterpinlines.h"

#include "MethodJIT-inl.h"

using namespace js;
using namespace js::mjit;

namespace js {
namespace mjit {

AutoScriptRetrapper::~AutoScriptRetrapper()
{
    while (!traps.empty()) {
        jsbytecode *pc = traps.back();
        traps.popBack();
        *pc = JSOP_TRAP;
    }
}

bool
AutoScriptRetrapper::untrap(jsbytecode *pc)
{
    if (!traps.append(pc))
        return false;
    *pc = JS_GetTrapOpcode(traps.allocPolicy().context(), script, pc);
    return true;
}

static inline JSRejoinState ScriptedRejoin(uint32 pcOffset)
{
    return REJOIN_SCRIPTED | (pcOffset << 1);
}

static inline JSRejoinState StubRejoin(RejoinState rejoin)
{
    return rejoin << 1;
}

static inline void
SetRejoinState(StackFrame *fp, const CallSite &site, void **location)
{
    if (site.rejoin == REJOIN_SCRIPTED) {
        fp->setRejoin(ScriptedRejoin(site.pcOffset));
        *location = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolineScripted);
    } else {
        fp->setRejoin(StubRejoin(site.rejoin));
        *location = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
    }
}

void
Recompiler::patchCall(JITScript *jit, StackFrame *fp, void **location)
{
    uint8* codeStart = (uint8 *)jit->code.m_code.executableAddress();

    CallSite *callSites_ = jit->callSites();
    for (uint32 i = 0; i < jit->nCallSites; i++) {
        if (callSites_[i].codeOffset + codeStart == *location) {
            JS_ASSERT(callSites_[i].inlineIndex == analyze::CrossScriptSSA::OUTER_FRAME);
            SetRejoinState(fp, callSites_[i], location);
            return;
        }
    }

    JS_NOT_REACHED("failed to find call site");
}

void
Recompiler::patchNative(JSContext *cx, JITScript *jit, StackFrame *fp,
                        jsbytecode *pc, CallSite *inlined, RejoinState rejoin)
{
    






    fp->setRejoin(StubRejoin(rejoin));

    
    cx->compartment->jaegerCompartment->orphanedNativeFrames.append(fp);

    unsigned i;
    ic::CallICInfo *callICs = jit->callICs();
    for (i = 0; i < jit->nCallICs; i++) {
        CallSite *call = callICs[i].call;
        if (inlined) {
            





            if (call->inlineIndex == inlined->inlineIndex && call->pcOffset == inlined->pcOffset)
                break;
        } else if (call->inlineIndex == uint32(-1) &&
                   call->pcOffset == uint32(pc - jit->script->code)) {
            break;
        }
    }
    JS_ASSERT(i < jit->nCallICs);
    ic::CallICInfo &ic = callICs[i];
    JS_ASSERT(ic.fastGuardedNative);

    JSC::ExecutablePool *&pool = ic.pools[ic::CallICInfo::Pool_NativeStub];

    if (!pool) {
        
        return;
    }

    
    {
#ifdef _WIN64
        
        void *interpoline = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolinePatched);
#else
        void *interpoline = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
#endif
        uint8 *start = (uint8 *)ic.nativeJump.executableAddress();
        JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));
#ifdef JS_CPU_X64
        repatch.repatch(ic.nativeJump, interpoline);
#else
        repatch.relink(ic.nativeJump, JSC::CodeLocationLabel(interpoline));
#endif
    }

    
    cx->compartment->jaegerCompartment->orphanedNativePools.append(pool);

    
    pool = NULL;
}

StackFrame *
Recompiler::expandInlineFrameChain(JSContext *cx, StackFrame *outer, InlineFrame *inner)
{
    StackFrame *parent;
    if (inner->parent)
        parent = expandInlineFrameChain(cx, outer, inner->parent);
    else
        parent = outer;

    JaegerSpew(JSpew_Recompile, "Expanding inline frame\n");

    StackFrame *fp = (StackFrame *) ((uint8 *)outer + sizeof(Value) * inner->depth);
    fp->initInlineFrame(inner->fun, parent, inner->parentpc);
    uint32 pcOffset = inner->parentpc - parent->script()->code;

    void **location = fp->addressOfNativeReturnAddress();
    *location = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolineScripted);
    parent->setRejoin(ScriptedRejoin(pcOffset));

    return fp;
}





static inline bool
JITCodeReturnAddress(void *data)
{
    return data != NULL  
        && data != JS_FUNC_TO_DATA_PTR(void *, JaegerTrampolineReturn)
        && data != JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline)
#ifdef _WIN64
        && data != JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolinePatched)
#endif
        && data != JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolineScripted);
}





void
Recompiler::expandInlineFrames(JSContext *cx, StackFrame *fp, mjit::CallSite *inlined,
                               StackFrame *next, VMFrame *f)
{
    JS_ASSERT_IF(next, next->prev() == fp && next->prevInline() == inlined);

    



    cx->compartment->types.frameExpansions++;

    




    void **frameAddr = f->returnAddressLocation();
    uint8* codeStart = (uint8 *)fp->jit()->code.m_code.executableAddress();

    InlineFrame *inner = &fp->jit()->inlineFrames()[inlined->inlineIndex];
    jsbytecode *innerpc = inner->fun->script()->code + inlined->pcOffset;

    StackFrame *innerfp = expandInlineFrameChain(cx, fp, inner);

    
    if (f->stubRejoin && f->fp() == fp) {
        
        JS_ASSERT(f->stubRejoin != REJOIN_NATIVE &&
                  f->stubRejoin != REJOIN_NATIVE_LOWERED &&
                  f->stubRejoin != REJOIN_NATIVE_PATCHED);
        innerfp->setRejoin(StubRejoin((RejoinState) f->stubRejoin));
        *frameAddr = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
        f->stubRejoin = 0;
    }
    if (*frameAddr == codeStart + inlined->codeOffset) {
        
        SetRejoinState(innerfp, *inlined, frameAddr);
    }

    if (f->fp() == fp) {
        JS_ASSERT(f->regs.inlined() == inlined);
        f->regs.expandInline(innerfp, innerpc);
    }

    







    if (next) {
        next->resetInlinePrev(innerfp, innerpc);
        void **addr = next->addressOfNativeReturnAddress();
        if (JITCodeReturnAddress(*addr)) {
            innerfp->setRejoin(ScriptedRejoin(inlined->pcOffset));
            *addr = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolineScripted);
        }
    }
}

void
ExpandInlineFrames(JSContext *cx, bool all)
{
    if (!all) {
        VMFrame *f = cx->compartment->jaegerCompartment->activeFrame();
        if (f && f->regs.inlined() && cx->fp() == f->fp())
            mjit::Recompiler::expandInlineFrames(cx, f->fp(), f->regs.inlined(), NULL, f);
        return;
    }

    for (VMFrame *f = cx->compartment->jaegerCompartment->activeFrame();
         f != NULL;
         f = f->previous) {

        if (f->regs.inlined()) {
            StackSegment &seg = cx->stack.space().containingSegment(f->fp());
            FrameRegs &regs = seg.currentRegs();
            if (regs.fp() == f->fp()) {
                JS_ASSERT(&regs == &f->regs);
                mjit::Recompiler::expandInlineFrames(cx, f->fp(), f->regs.inlined(), NULL, f);
            } else {
                StackFrame *nnext = seg.computeNextFrame(f->fp());
                mjit::Recompiler::expandInlineFrames(cx, f->fp(), f->regs.inlined(), nnext, f);
            }
        }

        StackFrame *end = f->entryfp->prev();
        StackFrame *next = NULL;
        for (StackFrame *fp = f->fp(); fp != end; fp = fp->prev()) {
            mjit::CallSite *inlined;
            fp->pc(cx, next, &inlined);
            if (next && inlined) {
                mjit::Recompiler::expandInlineFrames(cx, fp, inlined, next, f);
                fp = next;
                next = NULL;
            } else {
                next = fp;
            }
        }
    }
}

Recompiler::Recompiler(JSContext *cx, JSScript *script)
  : cx(cx), script(script)
{    
}




















void
Recompiler::recompile(bool resetUses)
{
    JS_ASSERT(script->hasJITCode());

    JaegerSpew(JSpew_Recompile, "recompiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
               script->filename, script->lineno, script->length);

    types::AutoEnterTypeInference enter(cx, true);

    









    
    
    VMFrame *nextf = NULL;
    for (VMFrame *f = script->compartment->jaegerCompartment->activeFrame();
         f != NULL;
         f = f->previous) {

        
        StackFrame *end = f->entryfp->prev();
        StackFrame *next = NULL;
        for (StackFrame *fp = f->fp(); fp != end; fp = fp->prev()) {
            if (fp->script() != script) {
                next = fp;
                continue;
            }

            if (next) {
                
                
                
                void **addr = next->addressOfNativeReturnAddress();

                if (JITCodeReturnAddress(*addr)) {
                    JS_ASSERT(fp->jit()->isValidCode(*addr));
                    patchCall(fp->jit(), fp, addr);
                } else if (nextf && nextf->entryfp == next &&
                           JITCodeReturnAddress(nextf->entryncode)) {
                    JS_ASSERT(fp->jit()->isValidCode(nextf->entryncode));
                    patchCall(fp->jit(), fp, &nextf->entryncode);
                }
            }

            next = fp;
        }

        





        StackFrame *fp = f->fp();
        void **addr = f->returnAddressLocation();
        RejoinState rejoin = (RejoinState) f->stubRejoin;
        if (rejoin == REJOIN_NATIVE ||
            rejoin == REJOIN_NATIVE_LOWERED) {
            
            if (fp->script() == script) {
                patchNative(cx, fp->jit(), fp, fp->pc(cx, NULL), NULL, rejoin);
                f->stubRejoin = REJOIN_NATIVE_PATCHED;
            }
        } else if (rejoin == REJOIN_NATIVE_PATCHED) {
            
        } else if (rejoin) {
            
            if (fp->script() == script) {
                fp->setRejoin(StubRejoin(rejoin));
                *addr = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
                f->stubRejoin = 0;
            }
        } else if (script->jitCtor && script->jitCtor->isValidCode(*addr)) {
            patchCall(script->jitCtor, fp, addr);
        } else if (script->jitNormal && script->jitNormal->isValidCode(*addr)) {
            patchCall(script->jitNormal, fp, addr);
        }

        nextf = f;
    }

    if (script->jitNormal) {
        cleanup(script->jitNormal);
        ReleaseScriptCode(cx, script, true);
    }
    if (script->jitCtor) {
        cleanup(script->jitCtor);
        ReleaseScriptCode(cx, script, false);
    }

    if (resetUses) {
        



        script->resetUseCount();
    }

    cx->compartment->types.recompilations++;
}

void
Recompiler::cleanup(JITScript *jit)
{
    while (!JS_CLIST_IS_EMPTY(&jit->callers)) {
        JaegerSpew(JSpew_Recompile, "Purging IC caller\n");

        JS_STATIC_ASSERT(offsetof(ic::CallICInfo, links) == 0);
        ic::CallICInfo *ic = (ic::CallICInfo *) jit->callers.next;

        uint8 *start = (uint8 *)ic->funGuard.executableAddress();
        JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));

        repatch.repatch(ic->funGuard, NULL);
        repatch.relink(ic->funJump, ic->slowPathStart);
        ic->purgeGuardedObject();
    }
}

} 
} 

#endif


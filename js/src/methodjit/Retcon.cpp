







































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

using namespace js;
using namespace js::mjit;

namespace js {
namespace mjit {

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

static inline bool
CallsiteMatches(uint8_t *codeStart, const CallSite &site, void *location)
{
    if (codeStart + site.codeOffset == location)
        return true;

#ifdef JS_CPU_ARM
    if (codeStart + site.codeOffset + 4 == location)
        return true;
#endif

    return false;
}

void
Recompiler::patchCall(JITChunk *chunk, StackFrame *fp, void **location)
{
    uint8_t* codeStart = (uint8_t *)chunk->code.m_code.executableAddress();

    CallSite *callSites_ = chunk->callSites();
    for (uint32_t i = 0; i < chunk->nCallSites; i++) {
        if (CallsiteMatches(codeStart, callSites_[i], *location)) {
            JS_ASSERT(callSites_[i].inlineIndex == analyze::CrossScriptSSA::OUTER_FRAME);
            SetRejoinState(fp, callSites_[i], location);
            return;
        }
    }

    JS_NOT_REACHED("failed to find call site");
}

void
Recompiler::patchNative(JSCompartment *compartment, JITChunk *chunk, StackFrame *fp,
                        jsbytecode *pc, RejoinState rejoin)
{
    










    fp->setRejoin(StubRejoin(rejoin));

    
    compartment->jaegerCompartment()->orphanedNativeFrames.append(fp);

    DebugOnly<bool> found = false;

    



    for (unsigned i = 0; i < chunk->nativeCallStubs.length(); i++) {
        NativeCallStub &stub = chunk->nativeCallStubs[i];
        if (stub.pc != pc)
            continue;

        found = true;

        
        if (!stub.pool)
            continue;

        
        {
#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
            
            void *interpoline = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolinePatched);
#else
            void *interpoline = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
#endif
            uint8_t *start = (uint8_t *)stub.jump.executableAddress();
            JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));
#ifdef JS_CPU_X64
            repatch.repatch(stub.jump, interpoline);
#else
            repatch.relink(stub.jump, JSC::CodeLocationLabel(interpoline));
#endif
        }

        
        compartment->jaegerCompartment()->orphanedNativePools.append(stub.pool);

        
        stub.pool = NULL;
    }

    JS_ASSERT(found);
}

void
Recompiler::patchFrame(JSCompartment *compartment, VMFrame *f, JSScript *script)
{
    





    StackFrame *fp = f->fp();
    void **addr = f->returnAddressLocation();
    RejoinState rejoin = (RejoinState) f->stubRejoin;
    if (rejoin == REJOIN_NATIVE ||
        rejoin == REJOIN_NATIVE_LOWERED ||
        rejoin == REJOIN_NATIVE_GETTER) {
        
        if (fp->script() == script) {
            patchNative(compartment, fp->jit()->chunk(f->regs.pc), fp, f->regs.pc, rejoin);
            f->stubRejoin = REJOIN_NATIVE_PATCHED;
        }
    } else if (rejoin == REJOIN_NATIVE_PATCHED) {
        
    } else if (rejoin) {
        
        if (fp->script() == script) {
            fp->setRejoin(StubRejoin(rejoin));
            *addr = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
            f->stubRejoin = 0;
        }
    } else {
        if (script->jitCtor) {
            JITChunk *chunk = script->jitCtor->findCodeChunk(*addr);
            if (chunk)
                patchCall(chunk, fp, addr);
        }
        if (script->jitNormal) {
            JITChunk *chunk = script->jitNormal->findCodeChunk(*addr);
            if (chunk)
                patchCall(chunk, fp, addr);
        }
    }
}

StackFrame *
Recompiler::expandInlineFrameChain(StackFrame *outer, InlineFrame *inner)
{
    StackFrame *parent;
    if (inner->parent)
        parent = expandInlineFrameChain(outer, inner->parent);
    else
        parent = outer;

    JaegerSpew(JSpew_Recompile, "Expanding inline frame\n");

    StackFrame *fp = (StackFrame *) ((uint8_t *)outer + sizeof(Value) * inner->depth);
    fp->initInlineFrame(inner->fun, parent, inner->parentpc);
    uint32_t pcOffset = inner->parentpc - parent->script()->code;

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
#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
        && data != JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolinePatched)
#endif
        && data != JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolineScripted);
}





void
Recompiler::expandInlineFrames(JSCompartment *compartment,
                               StackFrame *fp, mjit::CallSite *inlined,
                               StackFrame *next, VMFrame *f)
{
    JS_ASSERT_IF(next, next->prev() == fp && next->prevInline() == inlined);

    



    compartment->types.frameExpansions++;

    jsbytecode *pc = next ? next->prevpc(NULL) : f->regs.pc;
    JITChunk *chunk = fp->jit()->chunk(pc);

    




    void **frameAddr = f->returnAddressLocation();
    uint8_t* codeStart = (uint8_t *)chunk->code.m_code.executableAddress();

    InlineFrame *inner = &chunk->inlineFrames()[inlined->inlineIndex];
    jsbytecode *innerpc = inner->fun->script()->code + inlined->pcOffset;

    StackFrame *innerfp = expandInlineFrameChain(fp, inner);

    
    if (f->stubRejoin && f->fp() == fp) {
        
        JS_ASSERT(f->stubRejoin != REJOIN_NATIVE &&
                  f->stubRejoin != REJOIN_NATIVE_LOWERED &&
                  f->stubRejoin != REJOIN_NATIVE_GETTER &&
                  f->stubRejoin != REJOIN_NATIVE_PATCHED);
        innerfp->setRejoin(StubRejoin((RejoinState) f->stubRejoin));
        *frameAddr = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);
        f->stubRejoin = 0;
    }
    if (CallsiteMatches(codeStart, *inlined, *frameAddr)) {
        
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
ExpandInlineFrames(JSCompartment *compartment)
{
    if (!compartment || !compartment->hasJaegerCompartment())
        return;

    for (VMFrame *f = compartment->jaegerCompartment()->activeFrame();
         f != NULL;
         f = f->previous) {

        if (f->regs.inlined())
            mjit::Recompiler::expandInlineFrames(compartment, f->fp(), f->regs.inlined(), NULL, f);

        StackFrame *end = f->entryfp->prev();
        StackFrame *next = NULL;
        for (StackFrame *fp = f->fp(); fp != end; fp = fp->prev()) {
            if (!next) {
                next = fp;
                continue;
            }
            mjit::CallSite *inlined;
            next->prevpc(&inlined);
            if (inlined) {
                mjit::Recompiler::expandInlineFrames(compartment, fp, inlined, next, f);
                fp = next;
                next = NULL;
            } else {
                if (fp->downFramesExpanded())
                    break;
                next = fp;
            }
            fp->setDownFramesExpanded();
        }
    }
}

void
ClearAllFrames(JSCompartment *compartment)
{
    if (!compartment || !compartment->hasJaegerCompartment())
        return;

    ExpandInlineFrames(compartment);

    for (VMFrame *f = compartment->jaegerCompartment()->activeFrame();
         f != NULL;
         f = f->previous) {

        Recompiler::patchFrame(compartment, f, f->fp()->script());

        
        
        
        
        
        
        

        for (StackFrame *fp = f->fp(); fp != f->entryfp; fp = fp->prev())
            fp->setNativeReturnAddress(NULL);
    }
}




















void
Recompiler::clearStackReferences(FreeOp *fop, JSScript *script)
{
    JS_ASSERT(script->hasJITCode());

    JaegerSpew(JSpew_Recompile, "recompiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
               script->filename, script->lineno, script->length);

    JSCompartment *comp = script->compartment();
    types::AutoEnterTypeInference enter(fop, comp);

    









    
    
    for (VMFrame *f = comp->jaegerCompartment()->activeFrame();
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
                    JITChunk *chunk = fp->jit()->findCodeChunk(*addr);
                    patchCall(chunk, fp, addr);
                }
            }

            next = fp;
        }

        patchFrame(comp, f, script);
    }

    comp->types.recompilations++;
}

void
Recompiler::clearStackReferencesAndChunk(FreeOp *fop, JSScript *script,
                                         JITScript *jit, size_t chunkIndex,
                                         bool resetUses)
{
    Recompiler::clearStackReferences(fop, script);

    bool releaseChunk = true;
    if (jit->nchunks > 1) {
        
        
        
        
        for (VMFrame *f = script->compartment()->jaegerCompartment()->activeFrame();
             f != NULL;
             f = f->previous) {
            if (f->fp()->script() == script) {
                JS_ASSERT(f->stubRejoin != REJOIN_NATIVE &&
                          f->stubRejoin != REJOIN_NATIVE_LOWERED &&
                          f->stubRejoin != REJOIN_NATIVE_GETTER);
                if (f->stubRejoin == REJOIN_NATIVE_PATCHED) {
                    mjit::ReleaseScriptCode(fop, script);
                    releaseChunk = false;
                    break;
                }
            }
        }
    }

    if (releaseChunk)
        jit->destroyChunk(fop, chunkIndex, resetUses);
}

} 
} 

#endif 


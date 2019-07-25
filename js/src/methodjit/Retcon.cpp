







































#ifdef JS_METHODJIT

#include "Retcon.h"
#include "MethodJIT.h"
#include "Compiler.h"
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

Recompiler::PatchableAddress
Recompiler::findPatch(JITScript *jit, void **location)
{
    uint8* codeStart = (uint8 *)jit->code.m_code.executableAddress();

    CallSite *callSites_ = jit->callSites();
    for (uint32 i = 0; i < jit->nCallSites; i++) {
        if (callSites_[i].codeOffset + codeStart == *location) {
            JS_ASSERT(callSites_[i].inlineIndex == uint32(-1));
            PatchableAddress result;
            result.location = location;
            result.callSite = callSites_[i];
            return result;
        }
    }

    RejoinSite *rejoinSites_ = jit->rejoinSites();
    for (uint32 i = 0; i < jit->nRejoinSites; i++) {
        const RejoinSite &rs = rejoinSites_[i];
        if (rs.codeOffset + codeStart == *location) {
            PatchableAddress result;
            result.location = location;
            result.callSite.initialize(rs.codeOffset, uint32(-1), rs.pcOffset, rs.id);
            return result;
        }
    }

    JS_NOT_REACHED("failed to find call site");
    return PatchableAddress();
}

void *
Recompiler::findRejoin(JITScript *jit, const CallSite &callSite)
{
    JS_ASSERT(callSite.inlineIndex == uint32(-1));

    RejoinSite *rejoinSites_ = jit->rejoinSites();
    for (uint32 i = 0; i < jit->nRejoinSites; i++) {
        RejoinSite &rs = rejoinSites_[i];
        if (rs.pcOffset == callSite.pcOffset &&
            (rs.id == callSite.id || rs.id == RejoinSite::VARIADIC_ID)) {
            



            JS_ASSERT_IF(rs.id == RejoinSite::VARIADIC_ID,
                         callSite.id != CallSite::NCODE_RETURN_ID);
            uint8* codeStart = (uint8 *)jit->code.m_code.executableAddress();
            return codeStart + rs.codeOffset;
        }
    }

    
    JS_NOT_REACHED("Call site vanished.");
    return NULL;
}

void
Recompiler::applyPatch(JITScript *jit, PatchableAddress& toPatch)
{
    void *result = findRejoin(jit, toPatch.callSite);
    JS_ASSERT(result);
    *toPatch.location = result;
}

Recompiler::PatchableNative
Recompiler::stealNative(JITScript *jit, jsbytecode *pc)
{
    







    unsigned i;
    ic::CallICInfo *callICs = jit->callICs();
    for (i = 0; i < jit->nCallICs; i++) {
        CallSite *call = callICs[i].call;
        if (call->inlineIndex == uint32(-1) && call->pcOffset == uint32(pc - jit->script->code))
            break;
    }
    JS_ASSERT(i < jit->nCallICs);
    ic::CallICInfo &ic = callICs[i];
    JS_ASSERT(ic.fastGuardedNative);

    JSC::ExecutablePool *&pool = ic.pools[ic::CallICInfo::Pool_NativeStub];

    if (!pool) {
        
        PatchableNative native;
        native.pc = NULL;
        native.guardedNative = NULL;
        native.pool = NULL;
        return native;
    }

    PatchableNative native;
    native.pc = pc;
    native.guardedNative = ic.fastGuardedNative;
    native.pool = pool;
    native.nativeStart = ic.nativeStart;
    native.nativeFunGuard = ic.nativeFunGuard;
    native.nativeJump = ic.nativeJump;

    



    pool = NULL;

    return native;
}

void
Recompiler::patchNative(JITScript *jit, PatchableNative &native)
{
    if (!native.pc)
        return;

    unsigned i;
    ic::CallICInfo *callICs = jit->callICs();
    for (i = 0; i < jit->nCallICs; i++) {
        CallSite *call = callICs[i].call;
        if (call->inlineIndex == uint32(-1) && call->pcOffset == uint32(native.pc - jit->script->code))
            break;
    }
    JS_ASSERT(i < jit->nCallICs);
    ic::CallICInfo &ic = callICs[i];

    ic.fastGuardedNative = native.guardedNative;
    ic.pools[ic::CallICInfo::Pool_NativeStub] = native.pool;
    ic.nativeStart = native.nativeStart;
    ic.nativeFunGuard = native.nativeFunGuard;
    ic.nativeJump = native.nativeJump;

    
    {
        uint8 *start = (uint8 *)ic.funJump.executableAddress();
        JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));
        repatch.relink(ic.funJump, ic.nativeStart);
    }

    
    {
        uint8 *start = (uint8 *)native.nativeFunGuard.executableAddress();
        JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));
        repatch.relink(native.nativeFunGuard, ic.slowPathStart);
    }

    
    {
        JSC::CodeLocationLabel joinPoint = ic.slowPathStart.labelAtOffset(ic.slowJoinOffset);
        uint8 *start = (uint8 *)native.nativeJump.executableAddress();
        JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));
        repatch.relink(native.nativeJump, joinPoint);
    }
}

JSStackFrame *
Recompiler::expandInlineFrameChain(JSContext *cx, JSStackFrame *outer, InlineFrame *inner)
{
    JSStackFrame *parent;
    if (inner->parent)
        parent = expandInlineFrameChain(cx, outer, inner->parent);
    else
        parent = outer;

    JaegerSpew(JSpew_Recompile, "Expanding inline frame, %u unsynced entries\n",
               inner->nUnsyncedEntries);

    





    for (unsigned i = 0; i < inner->nUnsyncedEntries; i++) {
        const UnsyncedEntry &e = inner->unsyncedEntries[i];
        Value *slot = (Value *) ((uint8 *)outer + e.offset);
        if (e.copy) {
            Value *copied = (Value *) ((uint8 *)outer + e.u.copiedOffset);
            *slot = *copied;
        } else if (e.constant) {
            *slot = e.u.value;
        } else if (e.knownType) {
            slot->boxNonDoubleFrom(e.u.type, (uint64 *) slot);
        }
    }

    JSStackFrame *fp = (JSStackFrame *) ((uint8 *)outer + sizeof(Value) * inner->depth);
    fp->initInlineFrame(inner->fun, parent, inner->parentpc);
    uint32 pcOffset = inner->parentpc - parent->script()->code;

    







    JS_ASSERT(fp->jit() && fp->jit()->rejoinPoints);

    PatchableAddress patch;
    patch.location = fp->addressOfNativeReturnAddress();
    patch.callSite.initialize(0, uint32(-1), pcOffset, CallSite::NCODE_RETURN_ID);
    applyPatch(parent->jit(), patch);

    return fp;
}





void
Recompiler::expandInlineFrames(JSContext *cx, JSStackFrame *fp, mjit::CallSite *inlined,
                               JSStackFrame *next, VMFrame *f)
{
    JS_ASSERT_IF(next, next->prev() == fp && next->prevInline() == inlined);

    



    cx->compartment->types.frameExpansions++;

    
    void **frameAddr = f->returnAddressLocation();
    uint8* codeStart = (uint8 *)fp->jit()->code.m_code.executableAddress();
    bool patchFrameReturn =
        (f->scratch != NATIVE_CALL_SCRATCH_VALUE) &&
        (*frameAddr == codeStart + inlined->codeOffset);

    InlineFrame *inner = &fp->jit()->inlineFrames()[inlined->inlineIndex];
    jsbytecode *innerpc = inner->fun->script()->code + inlined->pcOffset;

    JSStackFrame *innerfp = expandInlineFrameChain(cx, fp, inner);
    JITScript *jit = innerfp->jit();

    if (f->regs.fp == fp) {
        JS_ASSERT(f->regs.inlined == inlined);
        f->regs.fp = innerfp;
        f->regs.pc = innerpc;
        f->regs.inlined = NULL;
    }

    if (patchFrameReturn) {
        PatchableAddress patch;
        patch.location = frameAddr;
        patch.callSite.initialize(0, uint32(-1), inlined->pcOffset, inlined->id);
        applyPatch(jit, patch);
    }

    if (next) {
        next->resetInlinePrev(innerfp, innerpc);
        void **addr = next->addressOfNativeReturnAddress();
        if (*addr != NULL && *addr != JS_FUNC_TO_DATA_PTR(void *, JaegerTrampolineReturn)) {
            PatchableAddress patch;
            patch.location = addr;
            patch.callSite.initialize(0, uint32(-1), inlined->pcOffset, CallSite::NCODE_RETURN_ID);
            applyPatch(jit, patch);
        }
    }
}

void
ExpandInlineFrames(JSContext *cx, bool all)
{
    if (!all) {
        VMFrame *f = cx->compartment->jaegerCompartment->activeFrame();
        if (f && f->regs.inlined && cx->fp() == f->fp())
            mjit::Recompiler::expandInlineFrames(cx, f->fp(), f->regs.inlined, NULL, f);
        return;
    }

    for (VMFrame *f = cx->compartment->jaegerCompartment->activeFrame();
         f != NULL;
         f = f->previous) {

        if (f->regs.inlined) {
            StackSegment *seg = cx->stack().containingSegment(f->fp());
            JSFrameRegs *regs = seg->getCurrentRegs();
            if (regs->fp == f->fp()) {
                JS_ASSERT(regs == &f->regs);
                mjit::Recompiler::expandInlineFrames(cx, f->fp(), f->regs.inlined, NULL, f);
            } else {
                JSStackFrame *nnext = seg->computeNextFrame(f->fp());
                mjit::Recompiler::expandInlineFrames(cx, f->fp(), f->regs.inlined, nnext, f);
            }
        }

        JSStackFrame *end = f->entryfp->prev();
        JSStackFrame *next = NULL;
        for (JSStackFrame *fp = f->fp(); fp != end; fp = fp->prev()) {
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






























bool
Recompiler::recompile()
{
    JS_ASSERT(script->hasJITCode());

    JaegerSpew(JSpew_Recompile, "recompiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
               script->filename, script->lineno, script->length);

    types::AutoEnterTypeInference enter(cx, true);

    











    Vector<PatchableAddress> normalPatches(cx);
    Vector<PatchableAddress> ctorPatches(cx);
    Vector<PatchableNative> normalNatives(cx);
    Vector<PatchableNative> ctorNatives(cx);

    
    Vector<PatchableFrame> normalFrames(cx);
    Vector<PatchableFrame> ctorFrames(cx);

    
    
    for (VMFrame *f = script->compartment->jaegerCompartment->activeFrame();
         f != NULL;
         f = f->previous) {

        
        JSStackFrame *end = f->entryfp->prev();
        JSStackFrame *next = NULL;
        for (JSStackFrame *fp = f->fp(); fp != end; fp = fp->prev()) {
            if (fp->script() != script) {
                next = fp;
                continue;
            }

            
            PatchableFrame frame;
            frame.fp = fp;
            frame.pc = fp->pc(cx, next);
            frame.scriptedCall = false;

            if (next) {
                
                
                
                void **addr = next->addressOfNativeReturnAddress();

                if (!*addr) {
                    
                } else if (*addr == JS_FUNC_TO_DATA_PTR(void *, JaegerTrampolineReturn)) {
                    
                } else if (fp->isConstructing()) {
                    JS_ASSERT(script->jitCtor && script->jitCtor->isValidCode(*addr));
                    frame.scriptedCall = true;
                    if (!ctorPatches.append(findPatch(script->jitCtor, addr)))
                        return false;
                } else {
                    JS_ASSERT(script->jitNormal && script->jitNormal->isValidCode(*addr));
                    frame.scriptedCall = true;
                    if (!normalPatches.append(findPatch(script->jitNormal, addr)))
                        return false;
                }
            }

            if (fp->isConstructing() && !ctorFrames.append(frame))
                return false;
            if (!fp->isConstructing() && !normalFrames.append(frame))
                return false;

            next = fp;
        }

        
        JSStackFrame *fp = f->fp();
        void **addr = f->returnAddressLocation();
        if (f->scratch == NATIVE_CALL_SCRATCH_VALUE) {
            
            if (fp->script() == script && fp->isConstructing()) {
                if (!ctorNatives.append(stealNative(script->jitCtor, fp->pc(cx, NULL))))
                    return false;
            } else if (fp->script() == script) {
                if (!normalNatives.append(stealNative(script->jitNormal, fp->pc(cx, NULL))))
                    return false;
            }
        } else if (script->jitCtor && script->jitCtor->isValidCode(*addr)) {
            if (!ctorPatches.append(findPatch(script->jitCtor, addr)))
                return false;
        } else if (script->jitNormal && script->jitNormal->isValidCode(*addr)) {
            if (!normalPatches.append(findPatch(script->jitNormal, addr)))
                return false;
        }
    }

    Vector<CallSite> normalSites(cx);
    Vector<CallSite> ctorSites(cx);

    if (script->jitNormal && !cleanup(script->jitNormal, &normalSites))
        return false;
    if (script->jitCtor && !cleanup(script->jitCtor, &ctorSites))
        return false;

    ReleaseScriptCode(cx, script, true);
    ReleaseScriptCode(cx, script, false);

    





    JSStackFrame *top = (cx->regs && cx->fp() && cx->fp()->isScriptFrame()) ? cx->fp() : NULL;
    bool keepNormal = !normalFrames.empty() || script->inlineParents ||
        (top && top->script() == script && !top->isConstructing());
    bool keepCtor = !ctorFrames.empty() ||
        (top && top->script() == script && top->isConstructing());

    if (keepNormal && !recompile(script, false,
                                 normalFrames, normalPatches, normalSites, normalNatives)) {
        return false;
    }
    if (keepCtor && !recompile(script, true,
                               ctorFrames, ctorPatches, ctorSites, ctorNatives)) {
        return false;
    }

    JS_ASSERT_IF(keepNormal, script->jitNormal);
    JS_ASSERT_IF(keepCtor, script->jitCtor);

    cx->compartment->types.recompilations++;

    if (!cx->compartment->types.checkPendingRecompiles(cx))
        return Compile_Error;

    return true;
}

bool
Recompiler::cleanup(JITScript *jit, Vector<CallSite> *sites)
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

    CallSite *callSites_ = jit->callSites();
    for (uint32 i = 0; i < jit->nCallSites; i++) {
        CallSite &site = callSites_[i];
        if (site.isTrap() && !sites->append(site))
            return false;
    }

    return true;
}

bool
Recompiler::recompile(JSScript *script, bool isConstructing,
                      Vector<PatchableFrame> &frames,
                      Vector<PatchableAddress> &patches, Vector<CallSite> &sites,
                      Vector<PatchableNative> &natives)
{
    JaegerSpew(JSpew_Recompile, "On stack recompilation, %u frames, %u patches, %u natives\n",
               frames.length(), patches.length(), natives.length());

    CompileStatus status = Compile_Retry;
    while (status == Compile_Retry) {
        Compiler cc(cx, script, isConstructing, &frames);
        if (!cc.loadOldTraps(sites))
            return false;
        status = cc.compile();
    }
    if (status != Compile_Okay)
        return false;

    JITScript *jit = script->getJIT(isConstructing);

    
    for (uint32 i = 0; i < patches.length(); i++)
        applyPatch(jit, patches[i]);
    for (uint32 i = 0; i < natives.length(); i++)
        patchNative(jit, natives[i]);

    return true;
}

} 
} 

#endif 


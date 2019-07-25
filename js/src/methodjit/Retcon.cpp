







































#ifdef JS_METHODJIT

#include "Retcon.h"
#include "MethodJIT.h"
#include "Compiler.h"
#include "jsdbgapi.h"
#include "jsnum.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"

#include "jscntxtinlines.h"

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
    *pc = JS_GetTrapOpcode(cx, script, pc);
    return true;
}

Recompiler::PatchableAddress
Recompiler::findPatch(JITScript *jit, void **location)
{ 
    uint8* codeStart = (uint8 *)jit->code.m_code.executableAddress();
    CallSite *callSites_ = jit->callSites();
    for (uint32 i = 0; i < jit->nCallSites; i++) {
        if (callSites_[i].codeOffset + codeStart == *location) {
            PatchableAddress result;
            result.location = location;
            result.callSite = callSites_[i];
            return result;
        }
    }

    JS_NOT_REACHED("failed to find call site");
    return PatchableAddress();
}

void
Recompiler::applyPatch(Compiler& c, PatchableAddress& toPatch)
{
    void *result = c.findCallSite(toPatch.callSite);
    JS_ASSERT(result);
    *toPatch.location = result;
}

Recompiler::PatchableNative
Recompiler::stealNative(JITScript *jit, jsbytecode *pc)
{
    







    unsigned i;
    ic::CallICInfo *callICs = jit->callICs();
    for (i = 0; i < jit->nCallICs; i++) {
        if (callICs[i].pc == pc)
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
        if (callICs[i].pc == native.pc)
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
            if (fp->script() == script) {
                
                PatchableFrame frame;
                frame.fp = fp;
                frame.pc = fp->pc(cx, next);
                if (fp->isConstructing() && !ctorFrames.append(frame))
                    return false;
                if (!fp->isConstructing() && !normalFrames.append(frame))
                    return false;
            }

            
            void **addr = fp->addressOfNativeReturnAddress();
            if (script->jitCtor && script->jitCtor->isValidCode(*addr)) {
                if (!ctorPatches.append(findPatch(script->jitCtor, addr)))
                    return false;
            } else if (script->jitNormal && script->jitNormal->isValidCode(*addr)) {
                if (!normalPatches.append(findPatch(script->jitNormal, addr)))
                    return false;
            }

            next = fp;
        }

        void **addr = f->returnAddressLocation();
        if (f->fp()->script() == script && f->scratch == NATIVE_CALL_SCRATCH_VALUE) {
            
            if (f->fp()->isConstructing()) {
                if (!ctorNatives.append(stealNative(script->jitCtor, f->fp()->pc(cx, NULL))))
                    return false;
            } else {
                if (!normalNatives.append(stealNative(script->jitNormal, f->fp()->pc(cx, NULL))))
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
    uint32 normalRecompilations;
    uint32 ctorRecompilations;

    if (script->jitNormal && !cleanup(script->jitNormal, &normalSites, &normalRecompilations))
        return false;
    if (script->jitCtor && !cleanup(script->jitCtor, &ctorSites, &ctorRecompilations))
        return false;

    ReleaseScriptCode(cx, script);

    if (normalFrames.length() &&
        !recompile(normalFrames, normalPatches, normalSites, normalNatives,
                   normalRecompilations)) {
        return false;
    }

    if (ctorFrames.length() &&
        !recompile(ctorFrames, ctorPatches, ctorSites, ctorNatives,
                   ctorRecompilations)) {
        return false;
    }

    return true;
}

bool
Recompiler::cleanup(JITScript *jit, Vector<CallSite> *sites, uint32 *recompilations)
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

    *recompilations = jit->recompilations;

    return true;
}

bool
Recompiler::recompile(Vector<PatchableFrame> &frames, Vector<PatchableAddress> &patches, Vector<CallSite> &sites,
                      Vector<PatchableNative> &natives,
                      uint32 recompilations)
{
    JSStackFrame *fp = frames[0].fp;

    JaegerSpew(JSpew_Recompile, "On stack recompilation, %u patches, %u natives\n",
               patches.length(), natives.length());

    Compiler c(cx, fp, &frames);
    if (!c.loadOldTraps(sites))
        return false;
    if (c.compile() != Compile_Okay)
        return false;

    script->getJIT(fp->isConstructing())->recompilations = recompilations + 1;

    
    for (uint32 i = 0; i < patches.length(); i++)
        applyPatch(c, patches[i]);
    for (uint32 i = 0; i < natives.length(); i++)
        patchNative(script->getJIT(fp->isConstructing()), natives[i]);

    return true;
}

} 
} 

#endif 


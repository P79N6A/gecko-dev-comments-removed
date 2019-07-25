







































#ifdef JS_METHODJIT

#include "Retcon.h"
#include "MethodJIT.h"
#include "Compiler.h"
#include "jsdbgapi.h"
#include "jsnum.h"

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

Recompiler::Recompiler(JSContext *cx, JSScript *script)
  : cx(cx), script(script)
{    
}













bool
Recompiler::recompile()
{
    JS_ASSERT(script->hasJITCode());

    Vector<PatchableAddress> normalPatches(cx);
    Vector<PatchableAddress> ctorPatches(cx);

    JSStackFrame *firstCtorFrame = NULL;
    JSStackFrame *firstNormalFrame = NULL;

    
    
    for (VMFrame *f = script->compartment->jaegerCompartment->activeFrame();
         f != NULL;
         f = f->previous) {

        
        JSStackFrame *end = f->entryfp->prev();
        for (JSStackFrame *fp = f->fp(); fp != end; fp = fp->prev()) {
            
            
            if (!firstCtorFrame && fp->script() == script && fp->isConstructing())
                firstCtorFrame = fp;
            else if (!firstNormalFrame && fp->script() == script && !fp->isConstructing())
                firstNormalFrame = fp;

            void **addr = fp->addressOfNativeReturnAddress();
            if (script->jitCtor && script->jitCtor->isValidCode(*addr)) {
                if (!ctorPatches.append(findPatch(script->jitCtor, addr)))
                    return false;
            } else if (script->jitNormal && script->jitNormal->isValidCode(*addr)) {
                if (!normalPatches.append(findPatch(script->jitNormal, addr)))
                    return false;
            }
        }

        void **addr = f->returnAddressLocation();
        if (script->jitCtor && script->jitCtor->isValidCode(*addr)) {
            if (!ctorPatches.append(findPatch(script->jitCtor, addr)))
                return false;
        } else if (script->jitNormal && script->jitNormal->isValidCode(*addr)) {
            if (!normalPatches.append(findPatch(script->jitNormal, addr)))
                return false;
        }
    }

    Vector<CallSite> normalSites(cx);
    Vector<CallSite> ctorSites(cx);

    if (script->jitNormal && !saveTraps(script->jitNormal, &normalSites))
        return false;
    if (script->jitCtor && !saveTraps(script->jitCtor, &ctorSites))
        return false;

    ReleaseScriptCode(cx, script);

    if (normalPatches.length() &&
        !recompile(firstNormalFrame, normalPatches, normalSites)) {
        return false;
    }

    if (ctorPatches.length() &&
        !recompile(firstCtorFrame, ctorPatches, ctorSites)) {
        return false;
    }

    return true;
}

bool
Recompiler::saveTraps(JITScript *jit, Vector<CallSite> *sites)
{
    CallSite *callSites_ = jit->callSites();
    for (uint32 i = 0; i < jit->nCallSites; i++) {
        CallSite &site = callSites_[i];
        if (site.isTrap() && !sites->append(site))
            return false;
    }
    return true;
}

bool
Recompiler::recompile(JSStackFrame *fp, Vector<PatchableAddress> &patches,
                      Vector<CallSite> &sites)
{
    
    JS_ASSERT(cx->compartment->debugMode);
    JS_ASSERT(fp);

    Compiler c(cx, fp);
    if (!c.loadOldTraps(sites))
        return false;
    if (c.compile() != Compile_Okay)
        return false;

    
    for (uint32 i = 0; i < patches.length(); i++)
        applyPatch(c, patches[i]);

    return true;
}

} 
} 

#endif 










































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
    *pc = JS_GetTrapOpcode(cx, script, pc);
    return true;
}

Recompiler::PatchableAddress
Recompiler::findPatch(JITScript *jit, void **location)
{ 
    uint8* codeStart = (uint8 *)jit->code.m_code.executableAddress();
    for (uint32 i = 0; i < jit->nCallSites; i++) {
        if (jit->callSites[i].codeOffset + codeStart == *location) {
            PatchableAddress result;
            result.location = location;
            result.callSite = jit->callSites[i];
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

        
        JSStackFrame *end = f->entryFp->prev();
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

    ReleaseScriptCode(cx, script);

    if (normalPatches.length() && !recompile(firstNormalFrame, normalPatches))
        return false;

    if (ctorPatches.length() && !recompile(firstCtorFrame, ctorPatches))
        return false;

    return true;
}

bool
Recompiler::recompile(JSStackFrame *fp, Vector<PatchableAddress> &patches)
{
    
    JS_ASSERT(cx->compartment->debugMode);
    JS_ASSERT(fp);

    Compiler c(cx, fp);
    if (c.compile() != Compile_Okay)
        return false;

    
    for (uint32 i = 0; i < patches.length(); i++)
        applyPatch(c, patches[i]);

    return true;
}

} 
} 

#endif 


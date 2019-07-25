




































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
Recompiler::findPatch(void **location)
{ 
    for (uint32 i = 0; i < script->callSites[-1].nCallSites; i++) {
        if (script->callSites[i].c.codeOffset + (uint8*)script->nmap[-1] == *location) {
            PatchableAddress result;
            result.location = location;
            result.callSite = script->callSites[i];
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

Recompiler::Recompiler(JSContext *cx, JSScript *script) : cx(cx), script(script)
{    
}













bool
Recompiler::recompile()
{
    JS_ASSERT(script->ncode && script->ncode != JS_UNJITTABLE_METHOD);

    Vector<PatchableAddress> toPatch(cx);

    
    JSStackFrame *firstFrame = NULL;
    for (CallStackIterator cs(cx); !cs.done(); ++cs) {
        FrameIterator fp = cs.top();
        for (FrameIterator fp = cs.top(); fp != cs.bottom(); ++fp) {
            if (!firstFrame && fp.fp()->script == script)
                firstFrame = fp.fp();
            if (script->isValidJitCode(fp.fp()->ncode)) {
                if (!toPatch.append(findPatch(&fp.fp()->ncode)))
                    return false;
            }
        }
    }

    
    for (VMFrame *f = JS_METHODJIT_DATA(cx).activeFrame;
         f != NULL;
         f = f->previous) {

        if (script->isValidJitCode(f->scriptedReturn)) {
            if (!toPatch.append(findPatch(&f->scriptedReturn)))
                return false;
        }

        void **machineReturn = f->returnAddressLocation();
        if (script->isValidJitCode(*machineReturn))
            if (!toPatch.append(findPatch(machineReturn)))
                return false;
    }

    ReleaseScriptCode(cx, script);

    
    if (!firstFrame)
        return true;

    Compiler c(cx, script, firstFrame->fun, firstFrame->scopeChain);
    if (c.Compile() != Compile_Okay)
        return false;

    
    for (uint32 i = 0; i < toPatch.length(); i++)
        applyPatch(c, toPatch[i]);

    return true;
}

FrameIterator&
FrameIterator::operator++() {
    JS_ASSERT(curfp);
    curfp = curfp->down;
    return *this;
}

bool
FrameIterator::operator==(const FrameIterator& other) const {
    return curfp == other.curfp;
}

bool
FrameIterator::operator!=(const FrameIterator& other) const {
    return curfp != other.curfp;
}

CallStackIterator&
CallStackIterator::operator++() {
    JS_ASSERT(curcs);
    curcs = curcs->getPreviousInMemory();
    return *this;
}

FrameIterator
CallStackIterator::top() const {
    JS_ASSERT(curcs);
    return FrameIterator(curcs->getCurrentFrame());
}

FrameIterator
CallStackIterator::bottom() const {
    JS_ASSERT(curcs);
    return FrameIterator(curcs->getInitialFrame()->down);
}

}
}

#endif

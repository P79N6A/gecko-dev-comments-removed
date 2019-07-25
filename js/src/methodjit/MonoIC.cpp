






































#include "jsscope.h"
#include "jsnum.h"
#include "MonoIC.h"
#include "StubCalls.h"
#include "assembler/assembler/RepatchBuffer.h"

#include "jsscopeinlines.h"

using namespace js;
using namespace js::mjit;
using namespace js::mjit::ic;

static void
PatchFallback(VMFrame &f, ic::MICInfo &mic)
{
    JSC::RepatchBuffer repatch(mic.stubEntry.executableAddress(), 64);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, stubs::GetGlobalName));
    repatch.relink(mic.stubCall, fptr);
}

void JS_FASTCALL
ic::GetGlobalName(VMFrame &f, uint32 index)
{
    JSObject *obj = f.fp->scopeChainObj()->getGlobal();
    ic::MICInfo &mic = f.fp->script->mics[index];
    JSAtom *atom = f.fp->script->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_LOCK_OBJ(f.cx, obj);
    JSScope *scope = obj->scope();
    JSScopeProperty *sprop = scope->lookup(id);
    if (!sprop ||
        !sprop->hasDefaultGetterOrIsMethod() ||
        !SPROP_HAS_VALID_SLOT(sprop, scope))
    {
        JS_UNLOCK_SCOPE(f.cx, scope);
        if (sprop)
            PatchFallback(f, mic);
        stubs::GetGlobalName(f);
        return;
    }
    uint32 shape = obj->shape();
    uint32 slot = sprop->slot;
    JS_UNLOCK_SCOPE(f.cx, scope);

    





    if (slot < JS_INITIAL_NSLOTS &&
        (!mic.touched || mic.lastSlot >= JS_INITIAL_NSLOTS)) {
        
        JS_NOT_REACHED("waat");
    } else if (slot >= JS_INITIAL_NSLOTS && mic.touched &&
               mic.lastSlot < JS_INITIAL_NSLOTS) {
        
        JS_NOT_REACHED("waat");
    }

    mic.touched = true;
    mic.lastSlot = slot;

    
    JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
    repatch.repatch(mic.shape, reinterpret_cast<void*>(shape));

    
    if (slot >= JS_INITIAL_NSLOTS)
        slot -= JS_INITIAL_NSLOTS;
    slot *= sizeof(Value);
    JSC::RepatchBuffer loads(mic.load.executableAddress(), 32, false);
    loads.repatch(mic.load.dataLabel32AtOffset(MICInfo::TYPE_OFFSET), slot);
    loads.repatch(mic.load.dataLabel32AtOffset(MICInfo::DATA_OFFSET), slot + 4);

    
    stubs::GetGlobalName(f);
}


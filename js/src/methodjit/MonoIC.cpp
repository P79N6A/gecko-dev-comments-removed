






































#include "jsscope.h"
#include "jsnum.h"
#include "MonoIC.h"
#include "StubCalls.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "jsobj.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;
using namespace js::mjit;
using namespace js::mjit::ic;

#if defined JS_MONOIC

static void
PatchGetFallback(VMFrame &f, ic::MICInfo &mic)
{
    JSC::RepatchBuffer repatch(mic.stubEntry.executableAddress(), 64);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, stubs::GetGlobalName));
    repatch.relink(mic.stubCall, fptr);
}

void JS_FASTCALL
ic::GetGlobalName(VMFrame &f, uint32 index)
{
    JSObject *obj = f.fp->scopeChain->getGlobal();
    ic::MICInfo &mic = f.fp->script->mics[index];
    JSAtom *atom = f.fp->script->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_ASSERT(mic.kind == ic::MICInfo::GET);

    JS_LOCK_OBJ(f.cx, obj);
    JSScope *scope = obj->scope();
    JSScopeProperty *sprop = scope->lookup(id);
    if (!sprop ||
        !sprop->hasDefaultGetterOrIsMethod() ||
        !SPROP_HAS_VALID_SLOT(sprop, scope))
    {
        JS_UNLOCK_SCOPE(f.cx, scope);
        if (sprop)
            PatchGetFallback(f, mic);
        stubs::GetGlobalName(f);
        return;
    }
    uint32 shape = obj->shape();
    uint32 slot = sprop->slot;
    JS_UNLOCK_SCOPE(f.cx, scope);

    mic.u.name.touched = true;

    
    JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
    repatch.repatch(mic.shape, reinterpret_cast<void*>(shape));

    
    JS_ASSERT(slot >= JS_INITIAL_NSLOTS);
    slot -= JS_INITIAL_NSLOTS;
    slot *= sizeof(Value);
    JSC::RepatchBuffer loads(mic.load.executableAddress(), 32, false);
    loads.repatch(mic.load.dataLabel32AtOffset(MICInfo::GET_TYPE_OFFSET), slot + 4);
    loads.repatch(mic.load.dataLabel32AtOffset(MICInfo::GET_DATA_OFFSET), slot);

    
    stubs::GetGlobalName(f);
}

static void JS_FASTCALL
SetGlobalNameSlow(VMFrame &f, uint32 index)
{
    JSAtom *atom = f.fp->script->getAtom(GET_INDEX(f.regs.pc));
    stubs::SetGlobalName(f, atom);
}

static void
PatchSetFallback(VMFrame &f, ic::MICInfo &mic)
{
    JSC::RepatchBuffer repatch(mic.stubEntry.executableAddress(), 64);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, SetGlobalNameSlow));
    repatch.relink(mic.stubCall, fptr);
}

void JS_FASTCALL
ic::SetGlobalName(VMFrame &f, uint32 index)
{
    JSObject *obj = f.fp->scopeChain->getGlobal();
    ic::MICInfo &mic = f.fp->script->mics[index];
    JSAtom *atom = f.fp->script->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_ASSERT(mic.kind == ic::MICInfo::SET);

    JS_LOCK_OBJ(f.cx, obj);
    JSScope *scope = obj->scope();
    JSScopeProperty *sprop = scope->lookup(id);
    if (!sprop ||
        !sprop->hasDefaultGetterOrIsMethod() ||
        !SPROP_HAS_VALID_SLOT(sprop, scope))
    {
        JS_UNLOCK_SCOPE(f.cx, scope);
        if (sprop)
            PatchSetFallback(f, mic);
        stubs::SetGlobalName(f, atom);
        return;
    }
    uint32 shape = obj->shape();
    uint32 slot = sprop->slot;
    JS_UNLOCK_SCOPE(f.cx, scope);

    mic.u.name.touched = true;

    
    JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
    repatch.repatch(mic.shape, reinterpret_cast<void*>(shape));

    
    JS_ASSERT(slot >= JS_INITIAL_NSLOTS);
    slot -= JS_INITIAL_NSLOTS;
    slot *= sizeof(Value);

    JSC::RepatchBuffer stores(mic.load.executableAddress(), 32, false);
    stores.repatch(mic.load.dataLabel32AtOffset(MICInfo::SET_TYPE_OFFSET), slot + 4);

    uint32 dataOffset;
    if (mic.u.name.typeConst)
        dataOffset = MICInfo::SET_DATA_CONST_TYPE_OFFSET;
    else
        dataOffset = MICInfo::SET_DATA_TYPE_OFFSET;
    if (mic.u.name.dataWrite)
        stores.repatch(mic.load.dataLabel32AtOffset(dataOffset), slot);

    
    stubs::SetGlobalName(f, atom);
}

#endif 

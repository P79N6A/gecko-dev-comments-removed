








































#include "jsscope.h"

#include "CodeGenerator.h"
#include "Ion.h"
#include "IonCaches.h"
#include "IonLinker.h"
#include "IonSpewer.h"

#include "vm/Stack.h"

using namespace js;
using namespace js::ion;

void
CodeLocationJump::repoint(IonCode *code)
{
    JS_ASSERT(!absolute);
    JS_ASSERT(size_t(raw_) <= code->instructionsSize());
    raw_ = code->raw() + size_t(raw_);
#ifdef JS_CPU_X64
    jumpTableEntry_ = Assembler::PatchableJumpAddress(code, (size_t) jumpTableEntry_);
#endif
    markAbsolute(true);
}

void
IonCache::loadResult(MacroAssembler &masm, Address address)
{
    if (output.hasValue())
        masm.loadValue(address, output.valueReg());
    else
        masm.loadUnboxedValue(address, output.typedReg());
}

static const size_t MAX_STUBS = 16;

bool
IonCacheGetProperty::attachNative(JSContext *cx, JSObject *obj, const Shape *shape)
{
    MacroAssembler masm;

    Label exit_;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(object(), JSObject::offsetOfShape()),
                                ImmGCPtr(obj->lastProperty()),
                                &exit_);
    masm.bind(&exit_);

    if (obj->isFixedSlot(shape->slot())) {
        loadResult(masm, Address(object(), JSObject::getFixedSlotOffset(shape->slot())));
    } else {
        bool restoreSlots = false;
        Register slotsReg;

        if (output.hasValue()) {
            slotsReg = output.valueReg().scratchReg();
        } else if (output.type() == MIRType_Double) {
            slotsReg = object();
            masm.Push(slotsReg);
            restoreSlots = true;
        } else {
            slotsReg = output.typedReg().gpr();
        }

        masm.loadPtr(Address(object(), JSObject::offsetOfSlots()), slotsReg);
        loadResult(masm, Address(slotsReg, obj->dynamicSlotIndex(shape->slot()) * sizeof(Value)));

        if (restoreSlots)
            masm.Pop(slotsReg);
    }

    Label rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);

    PatchJump(lastJump(), CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated native GETPROP stub at %p", code->raw());

    return true;
}

bool
js::ion::GetPropertyCache(JSContext *cx, size_t cacheIndex, JSObject *obj, Value *vp)
{
    IonScript *ion = GetTopIonFrame(cx);
    IonCacheGetProperty &cache = ion->getCache(cacheIndex).toGetProperty();
    JSAtom *atom = cache.atom();

    
    
    
    if (cache.stubCount() < MAX_STUBS) {
        cache.incrementStubCount();

        const Shape *shape = obj->nativeLookup(cx, ATOM_TO_JSID(atom));
        if (shape && shape->hasSlot() && shape->hasDefaultGetter()) {
            if (!cache.attachNative(cx, obj, shape))
                return false;
        }
    }

    if (!obj->getGeneric(cx, obj, ATOM_TO_JSID(atom), vp))
        return false;

    JSScript *script;
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    
    JS_ASSERT(script);

    types::TypeScript::Monitor(cx, script, pc, *vp);

    return true;
}

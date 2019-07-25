






































#include "jsscope.h"
#include "jsnum.h"
#include "MonoIC.h"
#include "StubCalls.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/assembler/MacroAssembler.h"
#include "CodeGenIncludes.h"
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
    JSObject *obj = f.fp()->getScopeChain()->getGlobal();
    ic::MICInfo &mic = f.fp()->getScript()->mics[index];
    JSAtom *atom = f.fp()->getScript()->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_ASSERT(mic.kind == ic::MICInfo::GET);

    JS_LOCK_OBJ(f.cx, obj);
    const Shape *shape = obj->nativeLookup(id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->hasSlot())
    {
        JS_UNLOCK_OBJ(f.cx, obj);
        if (shape)
            PatchGetFallback(f, mic);
        stubs::GetGlobalName(f);
        return;
    }
    uint32 slot = shape->slot;
    JS_UNLOCK_OBJ(f.cx, obj);

    mic.u.name.touched = true;

    
    JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
    repatch.repatch(mic.shape, obj->shape());

    
    JS_ASSERT(slot >= JS_INITIAL_NSLOTS);
    slot -= JS_INITIAL_NSLOTS;
    slot *= sizeof(Value);
    JSC::RepatchBuffer loads(mic.load.executableAddress(), 32, false);
#if defined JS_CPU_X86
    loads.repatch(mic.load.dataLabel32AtOffset(MICInfo::GET_DATA_OFFSET), slot);
    loads.repatch(mic.load.dataLabel32AtOffset(MICInfo::GET_TYPE_OFFSET), slot + 4);
#elif defined JS_CPU_ARM
    
    
    loads.repatch(mic.load.dataLabel32AtOffset(0), slot);
#elif defined JS_PUNBOX64
    loads.repatch(mic.load.dataLabel32AtOffset(mic.patchValueOffset), slot);
#endif

    
    stubs::GetGlobalName(f);
}

static void JS_FASTCALL
SetGlobalNameSlow(VMFrame &f, uint32 index)
{
    JSAtom *atom = f.fp()->getScript()->getAtom(GET_INDEX(f.regs.pc));
    stubs::SetGlobalName(f, atom);
}

static void
PatchSetFallback(VMFrame &f, ic::MICInfo &mic)
{
    JSC::RepatchBuffer repatch(mic.stubEntry.executableAddress(), 64);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, SetGlobalNameSlow));
    repatch.relink(mic.stubCall, fptr);
}

static VoidStubAtom
GetStubForSetGlobalName(VMFrame &f)
{
    
    
    return js_CodeSpec[*f.regs.pc].format & (JOF_INC | JOF_DEC)
         ? stubs::SetGlobalNameDumb
         : stubs::SetGlobalName;
}

void JS_FASTCALL
ic::SetGlobalName(VMFrame &f, uint32 index)
{
    JSObject *obj = f.fp()->getScopeChain()->getGlobal();
    ic::MICInfo &mic = f.fp()->getScript()->mics[index];
    JSAtom *atom = f.fp()->getScript()->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_ASSERT(mic.kind == ic::MICInfo::SET);

    JS_LOCK_OBJ(f.cx, obj);
    const Shape *shape = obj->nativeLookup(id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->writable() ||
        !shape->hasSlot())
    {
        JS_UNLOCK_OBJ(f.cx, obj);
        if (shape)
            PatchSetFallback(f, mic);
        GetStubForSetGlobalName(f)(f, atom);
        return;
    }
    uint32 slot = shape->slot;
    JS_UNLOCK_OBJ(f.cx, obj);

    mic.u.name.touched = true;

    
    JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
    repatch.repatch(mic.shape, obj->shape());

    
    JS_ASSERT(slot >= JS_INITIAL_NSLOTS);
    slot -= JS_INITIAL_NSLOTS;
    slot *= sizeof(Value);

    JSC::RepatchBuffer stores(mic.load.executableAddress(), 32, false);
#if defined JS_CPU_X86
    stores.repatch(mic.load.dataLabel32AtOffset(MICInfo::SET_TYPE_OFFSET), slot + 4);

    uint32 dataOffset;
    if (mic.u.name.typeConst)
        dataOffset = MICInfo::SET_DATA_CONST_TYPE_OFFSET;
    else
        dataOffset = MICInfo::SET_DATA_TYPE_OFFSET;
    stores.repatch(mic.load.dataLabel32AtOffset(dataOffset), slot);
#elif defined JS_CPU_ARM
    
    
    stores.repatch(mic.load.dataLabel32AtOffset(0), slot);
#elif defined JS_PUNBOX64
    stores.repatch(mic.load.dataLabel32AtOffset(mic.patchValueOffset), slot);
#endif

    
    GetStubForSetGlobalName(f)(f, atom);
}

#ifdef JS_CPU_X86

ic::NativeCallCompiler::NativeCallCompiler()
    : jumps(SystemAllocPolicy())
{}

void
ic::NativeCallCompiler::finish(JSScript *script, uint8 *start, uint8 *fallthrough)
{
    
    Jump fallJump = masm.jump();
    addLink(fallJump, fallthrough);

    uint8 *result = (uint8 *)script->jit->execPool->alloc(masm.size());
    JSC::ExecutableAllocator::makeWritable(result, masm.size());
    masm.executableCopy(result);

    
    BaseAssembler::insertJump(start, result);

    
    masm.finalize(result);

    
    JSC::LinkBuffer linkmasm(result, masm.size());

    for (size_t i = 0; i < jumps.length(); i++)
        linkmasm.link(jumps[i].from, JSC::CodeLocationLabel(jumps[i].to));
}

void
ic::CallFastNative(JSContext *cx, JSScript *script, MICInfo &mic, JSFunction *fun, bool isNew)
{
    if (mic.u.generated) {
        
        return;
    }
    mic.u.generated = true;

    JS_ASSERT(fun->isFastNative());
    if (isNew)
        JS_ASSERT(fun->isFastConstructor());

    FastNative fn = (FastNative)fun->u.n.native;

    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Jump Jump;

    uint8 *start = (uint8*) mic.knownObject.executableAddress();
    uint8 *stubEntry = (uint8*) mic.stubEntry.executableAddress();
    uint8 *fallthrough = (uint8*) mic.callEnd.executableAddress();

    NativeCallCompiler ncc;

    Jump differentFunction = ncc.masm.branchPtr(Assembler::NotEqual, mic.dataReg, ImmPtr(fun));
    ncc.addLink(differentFunction, stubEntry);

    

    
    JSC::MacroAssembler::RegisterID temp = mic.dataReg;

    
    ncc.masm.storePtr(ImmPtr(cx->regs->pc),
                      FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, pc)));

    
    uint32 spOffset = sizeof(JSStackFrame) + (mic.frameDepth + mic.argc + 2) * sizeof(jsval);
    ncc.masm.addPtr(Imm32(spOffset), JSFrameReg, temp);
    ncc.masm.storePtr(temp, FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, sp)));

    
    const uint32 stackAdjustment = 16;
    ncc.masm.sub32(Imm32(stackAdjustment), JSC::X86Registers::esp);

    
    uint32 vpOffset = sizeof(JSStackFrame) + mic.frameDepth * sizeof(jsval);
    ncc.masm.addPtr(Imm32(vpOffset), JSFrameReg, temp);
    ncc.masm.storePtr(temp, Address(JSC::X86Registers::esp, 0x8));

    if (isNew) {
        
        ncc.masm.storeValue(MagicValue(JS_FAST_CONSTRUCTOR), Address(temp, sizeof(Value)));
    }

    
    ncc.masm.store32(Imm32(mic.argc), Address(JSC::X86Registers::esp, 0x4));

    
    ncc.masm.loadPtr(FrameAddress(stackAdjustment + offsetof(VMFrame, cx)), temp);
    ncc.masm.storePtr(temp, Address(JSC::X86Registers::esp, 0));

    
    ncc.masm.call(JS_FUNC_TO_DATA_PTR(void *, fn));

    
    ncc.masm.add32(Imm32(stackAdjustment), JSC::X86Registers::esp);

#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
    
    
    
    ncc.masm.sub32(Imm32(8), JSC::X86Registers::esp);
#endif

    
    Jump hasException =
        ncc.masm.branchTest32(Assembler::Zero, Registers::ReturnReg, Registers::ReturnReg);
    ncc.addLink(hasException, JS_FUNC_TO_DATA_PTR(uint8 *, JaegerThrowpoline));

#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
    ncc.masm.add32(Imm32(8), JSC::X86Registers::esp);
#endif

    
    Address rval(JSFrameReg, vpOffset);
    ncc.masm.loadPayload(rval, JSReturnReg_Data);
    ncc.masm.loadTypeTag(rval, JSReturnReg_Type);

    ncc.finish(script, start, fallthrough);
}

#endif 

void
ic::PurgeMICs(JSContext *cx, JSScript *script)
{
    
    JS_ASSERT(cx->runtime->gcRegenShapes);

    uint32 nmics = script->jit->nMICs;
    for (uint32 i = 0; i < nmics; i++) {
        ic::MICInfo &mic = script->mics[i];
        switch (mic.kind) {
          case ic::MICInfo::SET:
          case ic::MICInfo::GET:
          {
            
            JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
            repatch.repatch(mic.shape, int(JSObjectMap::INVALID_SHAPE));

            



            break;
          }
          case ic::MICInfo::CALL:
          case ic::MICInfo::EMPTYCALL:
          case ic::MICInfo::TRACER:
            
            break;
          default:
            JS_NOT_REACHED("Unknown MIC type during purge");
            break;
        }
    }
}

#endif 


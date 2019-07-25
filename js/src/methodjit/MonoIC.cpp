






































#include "jsscope.h"
#include "jsnum.h"
#include "MonoIC.h"
#include "StubCalls.h"
#include "StubCalls-inl.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "CodeGenIncludes.h"
#include "methodjit/Compiler.h"
#include "InlineFrameAssembler.h"
#include "jsobj.h"

#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::mjit;
using namespace js::mjit::ic;

typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::Address Address;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::Imm32 Imm32;
typedef JSC::MacroAssembler::ImmPtr ImmPtr;
typedef JSC::MacroAssembler::Call Call;

#if defined JS_MONOIC

static void
PatchGetFallback(VMFrame &f, ic::MICInfo *ic)
{
    JSC::RepatchBuffer repatch(ic->stubEntry.executableAddress(), 64);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, stubs::GetGlobalName));
    repatch.relink(ic->stubCall, fptr);
}

void JS_FASTCALL
ic::GetGlobalName(VMFrame &f, ic::MICInfo *ic)
{
    JSObject *obj = f.fp()->scopeChain().getGlobal();
    JSAtom *atom = f.fp()->script()->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_ASSERT(ic->kind == ic::MICInfo::GET);

    JS_LOCK_OBJ(f.cx, obj);
    const Shape *shape = obj->nativeLookup(id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->hasSlot())
    {
        JS_UNLOCK_OBJ(f.cx, obj);
        if (shape)
            PatchGetFallback(f, ic);
        stubs::GetGlobalName(f);
        return;
    }
    uint32 slot = shape->slot;
    JS_UNLOCK_OBJ(f.cx, obj);

    ic->u.name.touched = true;

    
    JSC::RepatchBuffer repatch(ic->entry.executableAddress(), 50);
    repatch.repatch(ic->shape, obj->shape());

    
    slot *= sizeof(Value);
    JSC::RepatchBuffer loads(ic->load.executableAddress(), 32, false);
#if defined JS_CPU_X86
    loads.repatch(ic->load.dataLabel32AtOffset(MICInfo::GET_DATA_OFFSET), slot);
    loads.repatch(ic->load.dataLabel32AtOffset(MICInfo::GET_TYPE_OFFSET), slot + 4);
#elif defined JS_CPU_ARM
    
    
    loads.repatch(ic->load.dataLabel32AtOffset(0), slot);
#elif defined JS_PUNBOX64
    loads.repatch(ic->load.dataLabel32AtOffset(ic->patchValueOffset), slot);
#endif

    
    stubs::GetGlobalName(f);
}

static void JS_FASTCALL
SetGlobalNameSlow(VMFrame &f, uint32 index)
{
    JSScript *script = f.fp()->script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.regs.pc));
    if (script->strictModeCode)
        stubs::SetGlobalName<true>(f, atom);
    else
        stubs::SetGlobalName<false>(f, atom);
}

static void
PatchSetFallback(VMFrame &f, ic::MICInfo *ic)
{
    JSC::RepatchBuffer repatch(ic->stubEntry.executableAddress(), 64);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, SetGlobalNameSlow));
    repatch.relink(ic->stubCall, fptr);
}

static VoidStubAtom
GetStubForSetGlobalName(VMFrame &f)
{
    JSScript *script = f.fp()->script();
    
    
    return js_CodeSpec[*f.regs.pc].format & (JOF_INC | JOF_DEC)
         ? STRICT_VARIANT(stubs::SetGlobalNameDumb)
         : STRICT_VARIANT(stubs::SetGlobalName);
}

void JS_FASTCALL
ic::SetGlobalName(VMFrame &f, ic::MICInfo *ic)
{
    JSObject *obj = f.fp()->scopeChain().getGlobal();
    JSAtom *atom = f.fp()->script()->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

    JS_ASSERT(ic->kind == ic::MICInfo::SET);

    JS_LOCK_OBJ(f.cx, obj);
    const Shape *shape = obj->nativeLookup(id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->writable() ||
        !shape->hasSlot())
    {
        JS_UNLOCK_OBJ(f.cx, obj);
        if (shape)
            PatchSetFallback(f, ic);
        GetStubForSetGlobalName(f)(f, atom);
        return;
    }
    uint32 slot = shape->slot;
    JS_UNLOCK_OBJ(f.cx, obj);

    ic->u.name.touched = true;

    
    JSC::RepatchBuffer repatch(ic->entry.executableAddress(), 50);
    repatch.repatch(ic->shape, obj->shape());

    
    slot *= sizeof(Value);

    JSC::RepatchBuffer stores(ic->load.executableAddress(), 32, false);
#if defined JS_CPU_X86
    stores.repatch(ic->load.dataLabel32AtOffset(MICInfo::SET_TYPE_OFFSET), slot + 4);

    uint32 dataOffset;
    if (ic->u.name.typeConst)
        dataOffset = MICInfo::SET_DATA_CONST_TYPE_OFFSET;
    else
        dataOffset = MICInfo::SET_DATA_TYPE_OFFSET;
    stores.repatch(ic->load.dataLabel32AtOffset(dataOffset), slot);
#elif defined JS_CPU_ARM
    
    
    stores.repatch(ic->load.dataLabel32AtOffset(0), slot);
#elif defined JS_PUNBOX64
    stores.repatch(ic->load.dataLabel32AtOffset(ic->patchValueOffset), slot);
#endif

    
    GetStubForSetGlobalName(f)(f, atom);
}

class EqualityICLinker : public LinkerHelper
{
    VMFrame &f;

  public:
    EqualityICLinker(JSContext *cx, VMFrame &f)
        : LinkerHelper(cx), f(f)
    { }

    bool init(Assembler &masm) {
        JSC::ExecutablePool *pool = LinkerHelper::init(masm);
        if (!pool)
            return false;
        JSScript *script = f.fp()->script();
        JITScript *jit = script->getJIT(f.fp()->isConstructing());
        if (!jit->execPools.append(pool)) {
            pool->release();
            js_ReportOutOfMemory(cx);
            return false;
        }
        return true;
    }
};


static const uint32 INLINE_PATH_LENGTH = 64;

class EqualityCompiler : public BaseCompiler
{
    VMFrame &f;
    EqualityICInfo &ic;

    Vector<Jump, 4, SystemAllocPolicy> jumpList;
    Jump trueJump;
    Jump falseJump;
    
  public:
    EqualityCompiler(VMFrame &f, EqualityICInfo &ic)
        : BaseCompiler(f.cx), f(f), ic(ic), jumpList(SystemAllocPolicy())
    {
    }

    void linkToStub(Jump j)
    {
        jumpList.append(j);
    }

    void linkTrue(Jump j)
    {
        trueJump = j;
    }

    void linkFalse(Jump j)
    {
        falseJump = j;
    }
    
    void generateStringPath(Assembler &masm)
    {
        ValueRemat &lvr = ic.lvr;
        ValueRemat &rvr = ic.rvr;

        if (!lvr.isConstant && !lvr.isType(JSVAL_TYPE_STRING)) {
            Jump lhsFail = masm.testString(Assembler::NotEqual, lvr.typeReg());
            linkToStub(lhsFail);
        }
        
        if (!rvr.isConstant && !rvr.isType(JSVAL_TYPE_STRING)) {
            Jump rhsFail = masm.testString(Assembler::NotEqual, rvr.typeReg());
            linkToStub(rhsFail);
        }

        RegisterID tmp = ic.tempReg;
        
        
        Imm32 atomizedFlags(JSString::FLAT | JSString::ATOMIZED);
        
        masm.load32(Address(lvr.dataReg(), offsetof(JSString, mLengthAndFlags)), tmp);
        masm.and32(Imm32(JSString::TYPE_FLAGS_MASK), tmp);
        Jump lhsNotAtomized = masm.branch32(Assembler::NotEqual, tmp, atomizedFlags);
        linkToStub(lhsNotAtomized);

        if (!rvr.isConstant) {
            masm.load32(Address(rvr.dataReg(), offsetof(JSString, mLengthAndFlags)), tmp);
            masm.and32(Imm32(JSString::TYPE_FLAGS_MASK), tmp);
            Jump rhsNotAtomized = masm.branch32(Assembler::NotEqual, tmp, atomizedFlags);
            linkToStub(rhsNotAtomized);
        }

        if (rvr.isConstant) {
            JSString *str = Valueify(rvr.u.v).toString();
            JS_ASSERT(str->isAtomized());
            Jump test = masm.branchPtr(ic.cond, lvr.dataReg(), ImmPtr(str));
            linkTrue(test);
        } else {
            Jump test = masm.branchPtr(ic.cond, lvr.dataReg(), rvr.dataReg());
            linkTrue(test);
        }

        Jump fallthrough = masm.jump();
        linkFalse(fallthrough);
    }

    void generateObjectPath(Assembler &masm)
    {
        ValueRemat &lvr = ic.lvr;
        ValueRemat &rvr = ic.rvr;
        
        if (!lvr.isConstant && !lvr.isType(JSVAL_TYPE_OBJECT)) {
            Jump lhsFail = masm.testObject(Assembler::NotEqual, lvr.typeReg());
            linkToStub(lhsFail);
        }
        
        if (!rvr.isConstant && !rvr.isType(JSVAL_TYPE_OBJECT)) {
            Jump rhsFail = masm.testObject(Assembler::NotEqual, rvr.typeReg());
            linkToStub(rhsFail);
        }

        Jump lhsHasEq = masm.branchTest32(Assembler::NonZero,
                                          Address(lvr.dataReg(),
                                                  offsetof(JSObject, flags)),
                                          Imm32(JSObject::HAS_EQUALITY));
        linkToStub(lhsHasEq);

        if (rvr.isConstant) {
            JSObject *obj = &Valueify(rvr.u.v).toObject();
            Jump test = masm.branchPtr(ic.cond, lvr.dataReg(), ImmPtr(obj));
            linkTrue(test);
        } else {
            Jump test = masm.branchPtr(ic.cond, lvr.dataReg(), rvr.dataReg());
            linkTrue(test);
        }

        Jump fallthrough = masm.jump();
        linkFalse(fallthrough);
    }

    bool linkForIC(Assembler &masm)
    {
        EqualityICLinker buffer(cx, f);
        if (!buffer.init(masm))
            return false;

        
        for (size_t i = 0; i < jumpList.length(); i++)
            buffer.link(jumpList[i], ic.stubEntry);
        jumpList.clear();

        
        buffer.link(trueJump, ic.target);
        buffer.link(falseJump, ic.fallThrough);

        CodeLocationLabel cs = buffer.finalizeCodeAddendum();

        
        JSC::RepatchBuffer jumpRepatcher(ic.jumpToStub.executableAddress(), INLINE_PATH_LENGTH);
        jumpRepatcher.relink(ic.jumpToStub, cs);

        
        JSC::RepatchBuffer stubRepatcher(ic.stubCall.executableAddress(), INLINE_PATH_LENGTH);
        JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, ic.stub));
        stubRepatcher.relink(ic.stubCall, fptr);
        
        return true;
    }

    bool update()
    {
        if (!ic.generated) {
            Assembler masm;
            Value rval = f.regs.sp[-1];
            Value lval = f.regs.sp[-2];
            
            if (rval.isObject() && lval.isObject()) {
                generateObjectPath(masm);
                ic.generated = true;
            } else if (rval.isString() && lval.isString()) {
                generateStringPath(masm);
                ic.generated = true;
            } else {
                return true;
            }

            return linkForIC(masm);
        }

        return true;
    }
};

JSBool JS_FASTCALL
ic::Equality(VMFrame &f, ic::EqualityICInfo *ic)
{
    EqualityCompiler cc(f, *ic);
    if (!cc.update())
        THROWV(JS_FALSE);

    return ic->stub(f);
}

static void * JS_FASTCALL
SlowCallFromIC(VMFrame &f, ic::CallICInfo *ic)
{
    stubs::SlowCall(f, ic->argc);
    return NULL;
}

static void * JS_FASTCALL
SlowNewFromIC(VMFrame &f, ic::CallICInfo *ic)
{
    stubs::SlowNew(f, ic->argc);
    return NULL;
}







































class CallCompiler : public BaseCompiler
{
    VMFrame &f;
    CallICInfo &ic;
    Value *vp;
    bool callingNew;

  public:
    CallCompiler(VMFrame &f, CallICInfo &ic, bool callingNew)
      : BaseCompiler(f.cx), f(f), ic(ic), vp(f.regs.sp - (ic.argc + 2)), callingNew(callingNew)
    {
    }

    JSC::ExecutablePool *poolForSize(size_t size, CallICInfo::PoolIndex index)
    {
        JSC::ExecutablePool *ep = getExecPool(size);
        if (!ep)
            return NULL;
        JS_ASSERT(!ic.pools[index]);
        ic.pools[index] = ep;
        return ep;
    }

    bool generateFullCallStub(JSScript *script, uint32 flags)
    {
        





        Assembler masm;
        InlineFrameAssembler inlFrame(masm, ic, flags);
        RegisterID t0 = inlFrame.tempRegs.takeAnyReg();

        
        inlFrame.assemble(ic.funGuard.labelAtOffset(ic.joinPointOffset).executableAddress());

        
        Address scriptAddr(ic.funPtrReg, offsetof(JSFunction, u) +
                           offsetof(JSFunction::U::Scripted, script));
        masm.loadPtr(scriptAddr, t0);

        




        size_t offset = callingNew
                        ? offsetof(JSScript, jitArityCheckCtor)
                        : offsetof(JSScript, jitArityCheckNormal);
        masm.loadPtr(Address(t0, offset), t0);
        Jump hasCode = masm.branchPtr(Assembler::Above, t0, ImmPtr(JS_UNJITTABLE_SCRIPT));

        
        masm.storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));
        masm.move(Imm32(ic.argc), Registers::ArgReg1);
        JSC::MacroAssembler::Call tryCompile =
            masm.stubCall(JS_FUNC_TO_DATA_PTR(void *, stubs::CompileFunction),
                          script->code, ic.frameDepth);
        masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);

        Jump notCompiled = masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                              Registers::ReturnReg);

        masm.jump(Registers::ReturnReg);

        hasCode.linkTo(masm.label(), &masm);

        
        masm.move(Imm32(ic.argc), JSParamReg_Argc);
        masm.jump(t0);

        JSC::ExecutablePool *ep = poolForSize(masm.size(), CallICInfo::Pool_ScriptStub);
        if (!ep)
            return false;

        JSC::LinkBuffer buffer(&masm, ep);
        buffer.link(notCompiled, ic.slowPathStart.labelAtOffset(ic.slowJoinOffset));
        buffer.link(tryCompile,
                    JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, stubs::CompileFunction)));
        JSC::CodeLocationLabel cs = buffer.finalizeCodeAddendum();

        JaegerSpew(JSpew_PICs, "generated CALL stub %p (%d bytes)\n", cs.executableAddress(),
                   masm.size());

        JSC::CodeLocationJump oolJump = ic.slowPathStart.jumpAtOffset(ic.oolJumpOffset);
        uint8 *start = (uint8 *)oolJump.executableAddress();
        JSC::RepatchBuffer repatch(start - 32, 64);
        repatch.relink(oolJump, cs);

        return true;
    }

    void patchInlinePath(JSScript *script, JSObject *obj)
    {
        
        uint8 *start = (uint8 *)ic.funGuard.executableAddress();
        JSC::RepatchBuffer repatch(start - 32, 64);

        ic.fastGuardedObject = obj;

        JITScript *jit = script->getJIT(callingNew);

        repatch.repatch(ic.funGuard, obj);
        repatch.relink(ic.funGuard.jumpAtOffset(ic.hotJumpOffset),
                       JSC::CodeLocationLabel(jit->fastEntry));

        JaegerSpew(JSpew_PICs, "patched CALL path %p (obj: %p)\n", start, ic.fastGuardedObject);
    }

    bool generateStubForClosures(JSObject *obj)
    {
        
        Assembler masm;

        Registers tempRegs;
        tempRegs.takeReg(ic.funObjReg);

        RegisterID t0 = tempRegs.takeAnyReg();

        
        Jump claspGuard = masm.branchPtr(Assembler::NotEqual,
                                         Address(ic.funObjReg, offsetof(JSObject, clasp)),
                                         ImmPtr(&js_FunctionClass));

        
        JSFunction *fun = obj->getFunctionPrivate();
        masm.loadFunctionPrivate(ic.funObjReg, t0);
        Jump funGuard = masm.branchPtr(Assembler::NotEqual, t0, ImmPtr(fun));
        Jump done = masm.jump();

        JSC::ExecutablePool *ep = poolForSize(masm.size(), CallICInfo::Pool_ClosureStub);
        if (!ep)
            return false;

        JSC::LinkBuffer buffer(&masm, ep);
        buffer.link(claspGuard, ic.slowPathStart);
        buffer.link(funGuard, ic.slowPathStart);
        buffer.link(done, ic.funGuard.labelAtOffset(ic.hotPathOffset));
        JSC::CodeLocationLabel cs = buffer.finalizeCodeAddendum();

        JaegerSpew(JSpew_PICs, "generated CALL closure stub %p (%d bytes)\n",
                   cs.executableAddress(), masm.size());

        uint8 *start = (uint8 *)ic.funJump.executableAddress();
        JSC::RepatchBuffer repatch(start - 32, 64);
        repatch.relink(ic.funJump, cs);

        ic.hasJsFunCheck = true;

        return true;
    }

    bool generateNativeStub()
    {
        Value *vp = f.regs.sp - (ic.argc + 2);

        JSObject *obj;
        if (!IsFunctionObject(*vp, &obj))
            return false;

        JSFunction *fun = obj->getFunctionPrivate();
        if ((!callingNew && !fun->isNative()) || (callingNew && !fun->isConstructor()))
            return false;

        if (callingNew)
            vp[1].setMagicWithObjectOrNullPayload(NULL);

        if (!CallJSNative(cx, fun->u.n.native, ic.argc, vp))
            THROWV(true);

        
        if (ic.fastGuardedNative || ic.hasJsFunCheck)
            return true;

        
        if (!ic.hit) {
            ic.hit = true;
            return true;
        }

        
        Assembler masm;

        
        Jump funGuard = masm.branchPtr(Assembler::NotEqual, ic.funObjReg, ImmPtr(obj));

        Registers tempRegs;
#ifndef JS_CPU_X86
        tempRegs.takeReg(Registers::ArgReg0);
        tempRegs.takeReg(Registers::ArgReg1);
        tempRegs.takeReg(Registers::ArgReg2);
#endif
        RegisterID t0 = tempRegs.takeAnyReg();

        
        masm.storePtr(ImmPtr(cx->regs->pc),
                       FrameAddress(offsetof(VMFrame, regs.pc)));

        
        uint32 spOffset = sizeof(JSStackFrame) + ic.frameDepth * sizeof(Value);
        masm.addPtr(Imm32(spOffset), JSFrameReg, t0);
        masm.storePtr(t0, FrameAddress(offsetof(VMFrame, regs.sp)));

        
        masm.storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));

        
#ifdef JS_CPU_X86
        RegisterID cxReg = tempRegs.takeAnyReg();
#else
        RegisterID cxReg = Registers::ArgReg0;
#endif
        masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), cxReg);

#ifdef JS_CPU_X86
        
        masm.subPtr(Imm32(16), Assembler::stackPointerRegister);
#endif

        
#ifdef JS_CPU_X86
        RegisterID vpReg = t0;
#else
        RegisterID vpReg = Registers::ArgReg2;
#endif
        
        uint32 vpOffset = sizeof(JSStackFrame) + (ic.frameDepth - ic.argc - 2) * sizeof(Value);
        masm.addPtr(Imm32(vpOffset), JSFrameReg, vpReg);

        
        if (callingNew) {
            Value v;
            v.setMagicWithObjectOrNullPayload(NULL);
            masm.storeValue(v, Address(vpReg, sizeof(Value)));
        }

#ifdef JS_CPU_X86
        masm.storePtr(vpReg, Address(Assembler::stackPointerRegister, 8));
#endif

        
#ifdef JS_CPU_X86
        masm.store32(Imm32(ic.argc), Address(Assembler::stackPointerRegister, 4));
#else
        masm.move(Imm32(ic.argc), Registers::ArgReg1);
#endif

        
#ifdef JS_CPU_X86
        masm.storePtr(cxReg, Address(Assembler::stackPointerRegister, 0));
#endif

#ifdef _WIN64
        
        masm.subPtr(Imm32(32), Assembler::stackPointerRegister);
#endif
        
        Assembler::Call call = masm.call();

#ifdef JS_CPU_X86
        masm.addPtr(Imm32(16), Assembler::stackPointerRegister);
#endif
#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
        
        
        
        masm.subPtr(Imm32(8), Assembler::stackPointerRegister);
#endif

        Jump hasException = masm.branchTest32(Assembler::Zero, Registers::ReturnReg,
                                              Registers::ReturnReg);
        

#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
        
        
        
        masm.addPtr(Imm32(8), Assembler::stackPointerRegister);
#elif defined(_WIN64)
        
        masm.addPtr(Imm32(32), Assembler::stackPointerRegister);
#endif

        Jump done = masm.jump();

        
        hasException.linkTo(masm.label(), &masm);
        masm.move(ImmPtr(JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline)), Registers::ReturnReg);
        masm.jump(Registers::ReturnReg);

        JSC::ExecutablePool *ep = poolForSize(masm.size(), CallICInfo::Pool_NativeStub);
        if (!ep)
            THROWV(true);

        JSC::LinkBuffer buffer(&masm, ep);
        buffer.link(done, ic.slowPathStart.labelAtOffset(ic.slowJoinOffset));
        buffer.link(call, JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, fun->u.n.native)));
        buffer.link(funGuard, ic.slowPathStart);
        
        JSC::CodeLocationLabel cs = buffer.finalizeCodeAddendum();

        JaegerSpew(JSpew_PICs, "generated native CALL stub %p (%d bytes)\n",
                   cs.executableAddress(), masm.size());

        uint8 *start = (uint8 *)ic.funJump.executableAddress();
        JSC::RepatchBuffer repatch(start - 32, 64);
        repatch.relink(ic.funJump, cs);

        ic.fastGuardedNative = obj;

        return true;
    }

    void *update()
    {
        stubs::UncachedCallResult ucr;
        if (callingNew)
            stubs::UncachedNewHelper(f, ic.argc, &ucr);
        else
            stubs::UncachedCallHelper(f, ic.argc, &ucr);

        
        
        if (!ucr.codeAddr) {
            JSC::CodeLocationCall oolCall = ic.slowPathStart.callAtOffset(ic.oolCallOffset);
            uint8 *start = (uint8 *)oolCall.executableAddress();
            JSC::RepatchBuffer repatch(start - 32, 64);
            JSC::FunctionPtr fptr = callingNew
                                    ? JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, SlowNewFromIC))
                                    : JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, SlowCallFromIC));
            repatch.relink(oolCall, fptr);
            return NULL;
        }
            
        JSFunction *fun = ucr.fun;
        JS_ASSERT(fun);
        JSScript *script = fun->script();
        JS_ASSERT(script);
        JSObject *callee = ucr.callee;
        JS_ASSERT(callee);

        uint32 flags = callingNew ? JSFRAME_CONSTRUCTING : 0;

        if (!ic.hit) {
            ic.hit = true;
            return ucr.codeAddr;
        }

        if (ic.argc != fun->nargs) {
            if (!generateFullCallStub(script, flags))
                THROWV(NULL);
        } else {
            if (!ic.fastGuardedObject) {
                patchInlinePath(script, callee);
            } else if (!ic.hasJsFunCheck &&
                       !ic.fastGuardedNative &&
                       ic.fastGuardedObject->getFunctionPrivate() == fun) {
                



                if (!generateStubForClosures(callee))
                    THROWV(NULL);
            } else {
                if (!generateFullCallStub(script, flags))
                    THROWV(NULL);
            }
        }

        return ucr.codeAddr;
    }
};

void * JS_FASTCALL
ic::Call(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, false);
    return cc.update();
}

void * JS_FASTCALL
ic::New(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, true);
    return cc.update();
}

void JS_FASTCALL
ic::NativeCall(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, false);
    if (!cc.generateNativeStub())
        stubs::SlowCall(f, ic->argc);
}

void JS_FASTCALL
ic::NativeNew(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, true);
    if (!cc.generateNativeStub())
        stubs::SlowNew(f, ic->argc);
}

void
JITScript::purgeMICs()
{
    for (uint32 i = 0; i < nMICs; i++) {
        ic::MICInfo &mic = mics[i];
        switch (mic.kind) {
          case ic::MICInfo::SET:
          case ic::MICInfo::GET:
          {
            
            JSC::RepatchBuffer repatch(mic.entry.executableAddress(), 50);
            repatch.repatch(mic.shape, int(JSObjectMap::INVALID_SHAPE));

            



            break;
          }
          case ic::MICInfo::TRACER:
            
            break;
          default:
            JS_NOT_REACHED("Unknown MIC type during purge");
            break;
        }
    }
}

void
ic::PurgeMICs(JSContext *cx, JSScript *script)
{
    
    JS_ASSERT(cx->runtime->gcRegenShapes);

    if (script->jitNormal)
        script->jitNormal->purgeMICs();
    if (script->jitCtor)
        script->jitCtor->purgeMICs();
}

void
JITScript::sweepCallICs()
{
    for (uint32 i = 0; i < nCallICs; i++) {
        ic::CallICInfo &ic = callICs[i];

        




        bool fastFunDead = ic.fastGuardedObject && IsAboutToBeFinalized(ic.fastGuardedObject);
        bool nativeDead = ic.fastGuardedNative && IsAboutToBeFinalized(ic.fastGuardedNative);

        if (!fastFunDead && !nativeDead)
            continue;

        uint8 *start = (uint8 *)ic.funGuard.executableAddress();
        JSC::RepatchBuffer repatch(start - 32, 64);

        if (fastFunDead) {
            repatch.repatch(ic.funGuard, NULL);
            ic.releasePool(CallICInfo::Pool_ClosureStub);
            ic.hasJsFunCheck = false;
            ic.fastGuardedObject = NULL;
        }

        if (nativeDead) {
            ic.releasePool(CallICInfo::Pool_NativeStub);
            ic.fastGuardedNative = NULL;
        }

        repatch.relink(ic.funJump, ic.slowPathStart);

        ic.hit = false;
    }
}

void
ic::SweepCallICs(JSScript *script)
{
    if (script->jitNormal)
        script->jitNormal->sweepCallICs();
    if (script->jitCtor)
        script->jitCtor->sweepCallICs();
}

#endif 


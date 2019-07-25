






































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

    const Shape *shape = obj->nativeLookup(id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->hasSlot())
    {
        if (shape)
            PatchGetFallback(f, ic);
        stubs::GetGlobalName(f);
        return;
    }
    uint32 slot = shape->slot;

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

    const Shape *shape = obj->nativeLookup(id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->writable() ||
        !shape->hasSlot())
    {
        if (shape)
            PatchSetFallback(f, ic);
        GetStubForSetGlobalName(f)(f, atom);
        return;
    }
    uint32 slot = shape->slot;

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
        const ValueRemat &lvr = ic.lvr;
        const ValueRemat &rvr = ic.rvr;

        if (!lvr.isConstant() && !lvr.isType(JSVAL_TYPE_STRING)) {
            Jump lhsFail = masm.testString(Assembler::NotEqual, lvr.typeReg());
            linkToStub(lhsFail);
        }
        
        if (!rvr.isConstant() && !rvr.isType(JSVAL_TYPE_STRING)) {
            Jump rhsFail = masm.testString(Assembler::NotEqual, rvr.typeReg());
            linkToStub(rhsFail);
        }

        RegisterID tmp = ic.tempReg;
        
        
        Imm32 atomizedFlags(JSString::FLAT | JSString::ATOMIZED);
        
        masm.load32(Address(lvr.dataReg(), offsetof(JSString, mLengthAndFlags)), tmp);
        masm.and32(Imm32(JSString::TYPE_FLAGS_MASK), tmp);
        Jump lhsNotAtomized = masm.branch32(Assembler::NotEqual, tmp, atomizedFlags);
        linkToStub(lhsNotAtomized);

        if (!rvr.isConstant()) {
            masm.load32(Address(rvr.dataReg(), offsetof(JSString, mLengthAndFlags)), tmp);
            masm.and32(Imm32(JSString::TYPE_FLAGS_MASK), tmp);
            Jump rhsNotAtomized = masm.branch32(Assembler::NotEqual, tmp, atomizedFlags);
            linkToStub(rhsNotAtomized);
        }

        if (rvr.isConstant()) {
            JSString *str = rvr.value().toString();
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
        
        if (!lvr.isConstant() && !lvr.isType(JSVAL_TYPE_OBJECT)) {
            Jump lhsFail = masm.testObject(Assembler::NotEqual, lvr.typeReg());
            linkToStub(lhsFail);
        }
        
        if (!rvr.isConstant() && !rvr.isType(JSVAL_TYPE_OBJECT)) {
            Jump rhsFail = masm.testObject(Assembler::NotEqual, rvr.typeReg());
            linkToStub(rhsFail);
        }

        Jump lhsHasEq = masm.branchTest32(Assembler::NonZero,
                                          Address(lvr.dataReg(),
                                                  offsetof(JSObject, flags)),
                                          Imm32(JSObject::HAS_EQUALITY));
        linkToStub(lhsHasEq);

        if (rvr.isConstant()) {
            JSObject *obj = &rvr.value().toObject();
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
    stubs::SlowCall(f, ic->frameSize.getArgc(f));
    return NULL;
}

static void * JS_FASTCALL
SlowNewFromIC(VMFrame &f, ic::CallICInfo *ic)
{
    stubs::SlowNew(f, ic->frameSize.staticArgc());
    return NULL;
}







































class CallCompiler : public BaseCompiler
{
    VMFrame &f;
    CallICInfo &ic;
    bool callingNew;

  public:
    CallCompiler(VMFrame &f, CallICInfo &ic, bool callingNew)
      : BaseCompiler(f.cx), f(f), ic(ic), callingNew(callingNew)
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
        void *compilePtr = JS_FUNC_TO_DATA_PTR(void *, stubs::CompileFunction);
        if (ic.frameSize.isStatic()) {
            masm.move(Imm32(ic.frameSize.staticArgc()), Registers::ArgReg1);
            masm.fallibleVMCall(compilePtr, script->code, ic.frameSize.staticFrameDepth());
        } else {
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), Registers::ArgReg1);
            masm.fallibleVMCall(compilePtr, script->code, -1);
        }
        masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);

        Jump notCompiled = masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                              Registers::ReturnReg);

        masm.jump(Registers::ReturnReg);

        hasCode.linkTo(masm.label(), &masm);

        
        if (ic.frameSize.isStatic())
            masm.move(Imm32(ic.frameSize.staticArgc()), JSParamReg_Argc);
        else
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), JSParamReg_Argc);
        masm.jump(t0);

        JSC::ExecutablePool *ep = poolForSize(masm.size(), CallICInfo::Pool_ScriptStub);
        if (!ep)
            return false;

        JSC::LinkBuffer buffer(&masm, ep);
        buffer.link(notCompiled, ic.slowPathStart.labelAtOffset(ic.slowJoinOffset));
        masm.finalize(buffer);
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
        JS_ASSERT(ic.frameSize.isStatic());

        
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
        JS_ASSERT(ic.frameSize.isStatic());

        
        Assembler masm;

        Registers tempRegs;
        tempRegs.takeReg(ic.funObjReg);

        RegisterID t0 = tempRegs.takeAnyReg();

        
        Jump claspGuard = masm.testObjClass(Assembler::NotEqual, ic.funObjReg, &js_FunctionClass);

        
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
        
        uintN initialFrameDepth = f.regs.sp - f.regs.fp->slots();

        



        Value *vp;
        if (ic.frameSize.isStatic()) {
            JS_ASSERT(f.regs.sp - f.regs.fp->slots() == (int)ic.frameSize.staticFrameDepth());
            vp = f.regs.sp - (2 + ic.frameSize.staticArgc());
        } else {
            JS_ASSERT(*f.regs.pc == JSOP_FUNAPPLY && GET_ARGC(f.regs.pc) == 2);
            if (!ic::SplatApplyArgs(f))       
                THROWV(true);
            vp = f.regs.sp - (2 + f.u.call.dynamicArgc);
        }

        JSObject *obj;
        if (!IsFunctionObject(*vp, &obj))
            return false;

        JSFunction *fun = obj->getFunctionPrivate();
        if ((!callingNew && !fun->isNative()) || (callingNew && !fun->isConstructor()))
            return false;

        if (callingNew)
            vp[1].setMagicWithObjectOrNullPayload(NULL);

        if (!CallJSNative(cx, fun->u.n.native, ic.frameSize.getArgc(f), vp))
            THROWV(true);

        
        if (ic.fastGuardedNative || ic.hasJsFunCheck)
            return true;

        
        if (!ic.hit) {
            ic.hit = true;
            return true;
        }

        
        Assembler masm;

        
        Jump funGuard = masm.branchPtr(Assembler::NotEqual, ic.funObjReg, ImmPtr(obj));

        
        if (ic.frameSize.isDynamic()) {
            masm.fallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, ic::SplatApplyArgs),
                                f.regs.pc, initialFrameDepth);
        }

        Registers tempRegs;
#ifndef JS_CPU_X86
        tempRegs.takeReg(Registers::ArgReg0);
        tempRegs.takeReg(Registers::ArgReg1);
        tempRegs.takeReg(Registers::ArgReg2);
#endif
        RegisterID t0 = tempRegs.takeAnyReg();

        
        masm.storePtr(ImmPtr(cx->regs->pc),
                       FrameAddress(offsetof(VMFrame, regs.pc)));

        
        if (ic.frameSize.isStatic()) {
            uint32 spOffset = sizeof(JSStackFrame) + initialFrameDepth * sizeof(Value);
            masm.addPtr(Imm32(spOffset), JSFrameReg, t0);
            masm.storePtr(t0, FrameAddress(offsetof(VMFrame, regs.sp)));
        }

        
        masm.storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));

        
#ifdef JS_CPU_X86
        RegisterID cxReg = tempRegs.takeAnyReg();
#else
        RegisterID cxReg = Registers::ArgReg0;
#endif
        masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), cxReg);

        
#ifdef JS_CPU_X86
        RegisterID vpReg = t0;
#else
        RegisterID vpReg = Registers::ArgReg2;
#endif
        MaybeRegisterID argcReg;
        if (ic.frameSize.isStatic()) {
            uint32 vpOffset = sizeof(JSStackFrame) + (vp - f.regs.fp->slots()) * sizeof(Value);
            masm.addPtr(Imm32(vpOffset), JSFrameReg, vpReg);
        } else {
            argcReg = tempRegs.takeAnyReg();
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), argcReg.reg());
            masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), vpReg);

            
            RegisterID vpOff = tempRegs.takeAnyReg();
            masm.move(argcReg.reg(), vpOff);
            masm.add32(Imm32(2), vpOff);  
            JS_STATIC_ASSERT(sizeof(Value) == 8);
            masm.lshift32(Imm32(3), vpOff);
            masm.subPtr(vpOff, vpReg);

            tempRegs.putReg(vpOff);
        }

        
        if (callingNew) {
            Value v;
            v.setMagicWithObjectOrNullPayload(NULL);
            masm.storeValue(v, Address(vpReg, sizeof(Value)));
        }

        masm.setupABICall(Registers::NormalCall, 3);
        masm.storeArg(2, vpReg);
        if (ic.frameSize.isStatic())
            masm.storeArg(1, Imm32(ic.frameSize.staticArgc()));
        else
            masm.storeArg(1, argcReg.reg());
        masm.storeArg(0, cxReg);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, fun->u.n.native));

        Jump hasException = masm.branchTest32(Assembler::Zero, Registers::ReturnReg,
                                              Registers::ReturnReg);
        

        Jump done = masm.jump();

        
        hasException.linkTo(masm.label(), &masm);
        masm.throwInJIT();

        JSC::ExecutablePool *ep = poolForSize(masm.size(), CallICInfo::Pool_NativeStub);
        if (!ep)
            THROWV(true);

        JSC::LinkBuffer buffer(&masm, ep);
        buffer.link(done, ic.slowPathStart.labelAtOffset(ic.slowJoinOffset));
        buffer.link(funGuard, ic.slowPathStart);
        masm.finalize(buffer);
        
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
            stubs::UncachedNewHelper(f, ic.frameSize.staticArgc(), &ucr);
        else
            stubs::UncachedCallHelper(f, ic.frameSize.getArgc(f), &ucr);

        
        
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

        if (!ic.frameSize.isStatic() || ic.frameSize.staticArgc() != fun->nargs) {
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
        stubs::SlowCall(f, ic->frameSize.getArgc(f));
}

void JS_FASTCALL
ic::NativeNew(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, true);
    if (!cc.generateNativeStub())
        stubs::SlowNew(f, ic->frameSize.staticArgc());
}

static inline bool
BumpStack(VMFrame &f, uintN inc)
{
    static const unsigned MANY_ARGS = 1024;
    static const unsigned MIN_SPACE = 500;

    
    if (inc < MANY_ARGS) {
        if (f.regs.sp + inc < f.stackLimit)
            return true;
        StackSpace &stack = f.cx->stack();
        if (!stack.bumpCommitAndLimit(f.entryFp, f.regs.sp, inc, &f.stackLimit)) {
            js_ReportOverRecursed(f.cx);
            return false;
        }
        return true;
    }

    












    uintN incWithSpace = inc + MIN_SPACE;
    Value *bumpedWithSpace = f.regs.sp + incWithSpace;
    if (bumpedWithSpace < f.stackLimit)
        return true;

    StackSpace &stack = f.cx->stack();
    if (stack.bumpCommitAndLimit(f.entryFp, f.regs.sp, incWithSpace, &f.stackLimit))
        return true;

    if (!stack.ensureSpace(f.cx, f.regs.sp, incWithSpace))
        return false;
    f.stackLimit = bumpedWithSpace;
    return true;
}







JSBool JS_FASTCALL
ic::SplatApplyArgs(VMFrame &f)
{
    JSContext *cx = f.cx;
    JS_ASSERT(GET_ARGC(f.regs.pc) == 2);

    










    if (f.u.call.lazyArgsObj) {
        Value *vp = f.regs.sp - 3;
        JS_ASSERT(JS_CALLEE(cx, vp).toObject().getFunctionPrivate()->u.n.native == js_fun_apply);

        JSStackFrame *fp = f.regs.fp;
        if (!fp->hasOverriddenArgs() &&
            (!fp->hasArgsObj() ||
             (fp->hasArgsObj() && !fp->argsObj().isArgsLengthOverridden() &&
              !js_PrototypeHasIndexedProperties(cx, &fp->argsObj())))) {

            uintN n = fp->numActualArgs();
            if (!BumpStack(f, n))
                THROWV(false);
            f.regs.sp += n;

            Value *argv = JS_ARGV(cx, vp + 1 );
            if (fp->hasArgsObj())
                fp->forEachCanonicalActualArg(CopyNonHoleArgsTo(&fp->argsObj(), argv));
            else
                fp->forEachCanonicalActualArg(CopyTo(argv));

            f.u.call.dynamicArgc = n;
            return true;
        }

        



        f.regs.sp++;
        if (!js_GetArgsValue(cx, fp, &vp[3]))
            THROWV(false);
    }

    Value *vp = f.regs.sp - 4;
    JS_ASSERT(JS_CALLEE(cx, vp).toObject().getFunctionPrivate()->u.n.native == js_fun_apply);

    




    
    if (vp[3].isNullOrUndefined()) {
        f.regs.sp--;
        f.u.call.dynamicArgc = 0;
        return true;
    }

    
    if (!vp[3].isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS, js_apply_str);
        THROWV(false);
    }

    
    JSObject *aobj = &vp[3].toObject();
    jsuint length;
    if (!js_GetLengthProperty(cx, aobj, &length))
        THROWV(false);

    JS_ASSERT(!JS_ON_TRACE(cx));

    
    uintN n = uintN(JS_MIN(length, JS_ARGS_LENGTH_MAX));

    intN delta = n - 1;
    if (delta > 0 && !BumpStack(f, delta))
        THROWV(false);
    f.regs.sp += delta;

    
    if (!GetElements(cx, aobj, n, f.regs.sp - n))
        THROWV(false);

    f.u.call.dynamicArgc = n;
    return true;
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


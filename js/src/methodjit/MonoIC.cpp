






































#include "jsscope.h"
#include "jsnum.h"
#include "MonoIC.h"
#include "StubCalls.h"
#include "StubCalls-inl.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/CodeGenIncludes.h"
#include "methodjit/Compiler.h"
#include "methodjit/ICRepatcher.h"
#include "methodjit/PolyIC.h"
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
typedef JSC::MacroAssembler::Label Label;
typedef JSC::MacroAssembler::DataLabel32 DataLabel32;
typedef JSC::MacroAssembler::DataLabelPtr DataLabelPtr;

#if defined JS_MONOIC

static void
PatchGetFallback(VMFrame &f, ic::GetGlobalNameIC *ic)
{
    Repatcher repatch(f.jit());
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, stubs::GetGlobalName));
    repatch.relink(ic->slowPathCall, fptr);
}

void JS_FASTCALL
ic::GetGlobalName(VMFrame &f, ic::GetGlobalNameIC *ic)
{
    JSObject *obj = f.fp()->scopeChain().getGlobal();
    JSAtom *atom = f.script()->getAtom(GET_INDEX(f.pc()));
    jsid id = ATOM_TO_JSID(atom);

    const Shape *shape = obj->nativeLookup(f.cx, id);
    if (!shape ||
        !shape->hasDefaultGetterOrIsMethod() ||
        !shape->hasSlot())
    {
        if (shape)
            PatchGetFallback(f, ic);
        stubs::GetGlobalName(f);
        return;
    }
    uint32 slot = shape->slot();

    
    Repatcher repatcher(f.jit());
    repatcher.repatch(ic->fastPathStart.dataLabelPtrAtOffset(ic->shapeOffset), obj->lastProperty());

    
    uint32 index = obj->dynamicSlotIndex(slot);
    JSC::CodeLocationLabel label = ic->fastPathStart.labelAtOffset(ic->loadStoreOffset);
    repatcher.patchAddressOffsetForValueLoad(label, index * sizeof(Value));

    
    stubs::GetGlobalName(f);
}

template <JSBool strict>
static void JS_FASTCALL
DisabledSetGlobal(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSScript *script = f.script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.pc()));
    stubs::SetGlobalName<strict>(f, atom);
}

template void JS_FASTCALL DisabledSetGlobal<true>(VMFrame &f, ic::SetGlobalNameIC *ic);
template void JS_FASTCALL DisabledSetGlobal<false>(VMFrame &f, ic::SetGlobalNameIC *ic);

template <JSBool strict>
static void JS_FASTCALL
DisabledSetGlobalNoCache(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSScript *script = f.script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.pc()));
    stubs::SetGlobalNameNoCache<strict>(f, atom);
}

template void JS_FASTCALL DisabledSetGlobalNoCache<true>(VMFrame &f, ic::SetGlobalNameIC *ic);
template void JS_FASTCALL DisabledSetGlobalNoCache<false>(VMFrame &f, ic::SetGlobalNameIC *ic);

static void
PatchSetFallback(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSScript *script = f.script();
    Repatcher repatch(f.jit());
    VoidStubSetGlobal stub = ic->usePropertyCache
                             ? STRICT_VARIANT(DisabledSetGlobal)
                             : STRICT_VARIANT(DisabledSetGlobalNoCache);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, stub));
    repatch.relink(ic->slowPathCall, fptr);
}

void
SetGlobalNameIC::patchExtraShapeGuard(Repatcher &repatcher, const Shape *shape)
{
    JS_ASSERT(hasExtraStub);

    JSC::CodeLocationLabel label(JSC::MacroAssemblerCodePtr(extraStub.start()));
    repatcher.repatch(label.dataLabelPtrAtOffset(extraShapeGuard), shape);
}

void
SetGlobalNameIC::patchInlineShapeGuard(Repatcher &repatcher, const Shape *shape)
{
    JSC::CodeLocationDataLabelPtr label = fastPathStart.dataLabelPtrAtOffset(shapeOffset);
    repatcher.repatch(label, shape);
}

static LookupStatus
UpdateSetGlobalName(VMFrame &f, ic::SetGlobalNameIC *ic, JSObject *obj, const Shape *shape)
{
    
    if (!shape)
        return Lookup_Uncacheable;

    if (shape->isMethod() ||
        !shape->hasDefaultSetter() ||
        !shape->writable() ||
        !shape->hasSlot() ||
        obj->watched())
    {
        
        PatchSetFallback(f, ic);
        return Lookup_Uncacheable;
    }

    
    Repatcher repatcher(f.jit());
    ic->patchInlineShapeGuard(repatcher, obj->lastProperty());

    uint32 index = obj->dynamicSlotIndex(shape->slot());
    JSC::CodeLocationLabel label = ic->fastPathStart.labelAtOffset(ic->loadStoreOffset);
    repatcher.patchAddressOffsetForValueStore(label, index * sizeof(Value),
                                              ic->vr.isTypeKnown());

    return Lookup_Cacheable;
}

void JS_FASTCALL
ic::SetGlobalName(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSObject *obj = f.fp()->scopeChain().getGlobal();
    JSScript *script = f.script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.pc()));
    const Shape *shape = obj->nativeLookup(f.cx, ATOM_TO_JSID(atom));

    LookupStatus status = UpdateSetGlobalName(f, ic, obj, shape);
    if (status == Lookup_Error)
        THROW();

    if (ic->usePropertyCache)
        STRICT_VARIANT(stubs::SetGlobalName)(f, atom);
    else
        STRICT_VARIANT(stubs::SetGlobalNameNoCache)(f, atom);
}

class EqualityICLinker : public LinkerHelper
{
    VMFrame &f;

  public:
    EqualityICLinker(Assembler &masm, VMFrame &f)
        : LinkerHelper(masm, JSC::METHOD_CODE), f(f)
    { }

    bool init(JSContext *cx) {
        JSC::ExecutablePool *pool = LinkerHelper::init(cx);
        if (!pool)
            return false;
        JS_ASSERT(!f.regs.inlined());
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

        JS_ASSERT_IF(lvr.isConstant(), lvr.isType(JSVAL_TYPE_STRING));
        JS_ASSERT_IF(rvr.isConstant(), rvr.isType(JSVAL_TYPE_STRING));

        if (!lvr.isType(JSVAL_TYPE_STRING)) {
            Jump lhsFail = masm.testString(Assembler::NotEqual, lvr.typeReg());
            linkToStub(lhsFail);
        }
        
        if (!rvr.isType(JSVAL_TYPE_STRING)) {
            Jump rhsFail = masm.testString(Assembler::NotEqual, rvr.typeReg());
            linkToStub(rhsFail);
        }

        RegisterID tmp = ic.tempReg;
        
        
        JS_STATIC_ASSERT(JSString::ATOM_FLAGS == 0);
        Imm32 atomMask(JSString::ATOM_MASK);
        
        masm.load32(Address(lvr.dataReg(), JSString::offsetOfLengthAndFlags()), tmp);
        Jump lhsNotAtomized = masm.branchTest32(Assembler::NonZero, tmp, atomMask);
        linkToStub(lhsNotAtomized);

        if (!rvr.isConstant()) {
            masm.load32(Address(rvr.dataReg(), JSString::offsetOfLengthAndFlags()), tmp);
            Jump rhsNotAtomized = masm.branchTest32(Assembler::NonZero, tmp, atomMask);
            linkToStub(rhsNotAtomized);
        }

        if (rvr.isConstant()) {
            JSString *str = rvr.value().toString();
            JS_ASSERT(str->isAtom());
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
        EqualityICLinker buffer(masm, f);
        if (!buffer.init(cx))
            return false;

        Repatcher repatcher(f.jit());

        
        JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, ic.stub));
        repatcher.relink(ic.stubCall, fptr);

        
        if (!buffer.verifyRange(f.jit()))
            return true;

        
        for (size_t i = 0; i < jumpList.length(); i++)
            buffer.link(jumpList[i], ic.stubEntry);
        jumpList.clear();

        
        buffer.link(trueJump, ic.target);
        buffer.link(falseJump, ic.fallThrough);

        CodeLocationLabel cs = buffer.finalize();

        
        repatcher.relink(ic.jumpToStub, cs);

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

bool
NativeStubLinker::init(JSContext *cx)
{
    JSC::ExecutablePool *pool = LinkerHelper::init(cx);
    if (!pool)
        return false;

    NativeCallStub stub;
    stub.pc = pc;
    stub.pool = pool;
    stub.jump = locationOf(done);
    if (!jit->nativeCallStubs.append(stub)) {
        pool->release();
        return false;
    }

    return true;
}







bool
mjit::NativeStubEpilogue(VMFrame &f, Assembler &masm, NativeStubLinker::FinalJump *result,
                         int32 initialFrameDepth, int32 vpOffset,
                         MaybeRegisterID typeReg, MaybeRegisterID dataReg)
{
    
    masm.loadPtr(FrameAddress(VMFrame::offsetOfFp), JSFrameReg);

    Jump hasException = masm.branchTest32(Assembler::Zero, Registers::ReturnReg,
                                          Registers::ReturnReg);

    Address resultAddress(JSFrameReg, vpOffset);

    Vector<Jump> mismatches(f.cx);
    if (f.cx->typeInferenceEnabled() && !typeReg.isSet()) {
        




        types::TypeSet *types = f.script()->analysis()->bytecodeTypes(f.pc());
        if (!masm.generateTypeCheck(f.cx, resultAddress, types, &mismatches))
            THROWV(false);
    }

    



    masm.storePtr(ImmPtr(NULL), FrameAddress(offsetof(VMFrame, stubRejoin)));

    if (typeReg.isSet())
        masm.loadValueAsComponents(resultAddress, typeReg.reg(), dataReg.reg());

    



    Label finished = masm.label();
#ifdef JS_CPU_X64
    JSC::MacroAssembler::DataLabelPtr done = masm.moveWithPatch(ImmPtr(NULL), Registers::ValueReg);
    masm.jump(Registers::ValueReg);
#else
    Jump done = masm.jump();
#endif

    
    if (!mismatches.empty()) {
        for (unsigned i = 0; i < mismatches.length(); i++)
            mismatches[i].linkTo(masm.label(), &masm);
        masm.addPtr(Imm32(vpOffset), JSFrameReg, Registers::ArgReg1);
        masm.fallibleVMCall(true, JS_FUNC_TO_DATA_PTR(void *, stubs::TypeBarrierReturn),
                            f.regs.pc, NULL, initialFrameDepth);
        masm.storePtr(ImmPtr(NULL), FrameAddress(offsetof(VMFrame, stubRejoin)));
        masm.jump().linkTo(finished, &masm);
    }

    
    hasException.linkTo(masm.label(), &masm);
    masm.storePtr(ImmPtr(NULL), FrameAddress(offsetof(VMFrame, stubRejoin)));
    masm.throwInJIT();

    *result = done;
    return true;
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

    JSC::ExecutablePool *poolForSize(LinkerHelper &linker, CallICInfo::PoolIndex index)
    {
        JSC::ExecutablePool *ep = linker.init(f.cx);
        if (!ep)
            return NULL;
        JS_ASSERT(!ic.pools[index]);
        ic.pools[index] = ep;
        return ep;
    }

    void disable(JITScript *jit)
    {
        JSC::CodeLocationCall oolCall = ic.slowPathStart.callAtOffset(ic.oolCallOffset);
        Repatcher repatch(jit);
        JSC::FunctionPtr fptr = callingNew
                                ? JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, SlowNewFromIC))
                                : JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, SlowCallFromIC));
        repatch.relink(oolCall, fptr);
    }

    bool generateFullCallStub(JITScript *from, JSScript *script, uint32 flags)
    {
        





        Assembler masm;
        InlineFrameAssembler inlFrame(masm, ic, flags);
        RegisterID t0 = inlFrame.tempRegs.takeAnyReg().reg();

        
        void *ncode = ic.funGuard.labelAtOffset(ic.joinPointOffset).executableAddress();
        inlFrame.assemble(ncode, f.pc());

        
        Address scriptAddr(ic.funPtrReg, offsetof(JSFunction, u) +
                           offsetof(JSFunction::U::Scripted, script));
        masm.loadPtr(scriptAddr, t0);

        




        size_t offset = callingNew
                        ? offsetof(JSScript, jitArityCheckCtor)
                        : offsetof(JSScript, jitArityCheckNormal);
        masm.loadPtr(Address(t0, offset), t0);
        Jump hasCode = masm.branchPtr(Assembler::Above, t0, ImmPtr(JS_UNJITTABLE_SCRIPT));

        




        masm.storePtr(ImmPtr((void *) ic.frameSize.rejoinState(f.pc(), false)),
                      FrameAddress(offsetof(VMFrame, stubRejoin)));

        masm.bumpStubCounter(f.script(), f.pc(), Registers::tempCallReg());

        
        void *compilePtr = JS_FUNC_TO_DATA_PTR(void *, stubs::CompileFunction);
        DataLabelPtr inlined;
        if (ic.frameSize.isStatic()) {
            masm.move(Imm32(ic.frameSize.staticArgc()), Registers::ArgReg1);
            masm.fallibleVMCall(cx->typeInferenceEnabled(),
                                compilePtr, f.regs.pc, &inlined, ic.frameSize.staticLocalSlots());
        } else {
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), Registers::ArgReg1);
            masm.fallibleVMCall(cx->typeInferenceEnabled(),
                                compilePtr, f.regs.pc, &inlined, -1);
        }

        Jump notCompiled = masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                              Registers::ReturnReg);
        masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), JSFrameReg);

        
        ncode = (uint8 *) f.jit()->code.m_code.executableAddress() + ic.call->codeOffset;
        masm.storePtr(ImmPtr(ncode), Address(JSFrameReg, StackFrame::offsetOfNcode()));

        masm.jump(Registers::ReturnReg);

        hasCode.linkTo(masm.label(), &masm);

        
        if (ic.frameSize.isStatic())
            masm.move(Imm32(ic.frameSize.staticArgc()), JSParamReg_Argc);
        else
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), JSParamReg_Argc);
        masm.jump(t0);

        LinkerHelper linker(masm, JSC::METHOD_CODE);
        JSC::ExecutablePool *ep = poolForSize(linker, CallICInfo::Pool_ScriptStub);
        if (!ep)
            return false;

        if (!linker.verifyRange(from)) {
            disable(from);
            return true;
        }

        linker.link(notCompiled, ic.slowPathStart.labelAtOffset(ic.slowJoinOffset));
        JSC::CodeLocationLabel cs = linker.finalize();

        JaegerSpew(JSpew_PICs, "generated CALL stub %p (%lu bytes)\n", cs.executableAddress(),
                   (unsigned long) masm.size());

        if (f.regs.inlined()) {
            JSC::LinkBuffer code((uint8 *) cs.executableAddress(), masm.size(), JSC::METHOD_CODE);
            code.patch(inlined, f.regs.inlined());
        }

        Repatcher repatch(from);
        JSC::CodeLocationJump oolJump = ic.slowPathStart.jumpAtOffset(ic.oolJumpOffset);
        repatch.relink(oolJump, cs);

        return true;
    }

    bool patchInlinePath(JITScript *from, JSScript *script, JSObject *obj)
    {
        JS_ASSERT(ic.frameSize.isStatic());
        JITScript *jit = script->getJIT(callingNew);

        
        Repatcher repatch(from);

        



        void *entry = ic.typeMonitored ? jit->argsCheckEntry : jit->fastEntry;

        if (!repatch.canRelink(ic.funGuard.jumpAtOffset(ic.hotJumpOffset),
                               JSC::CodeLocationLabel(entry))) {
            return false;
        }

        ic.fastGuardedObject = obj;
        JS_APPEND_LINK(&ic.links, &jit->callers);

        repatch.repatch(ic.funGuard, obj);
        repatch.relink(ic.funGuard.jumpAtOffset(ic.hotJumpOffset),
                       JSC::CodeLocationLabel(entry));

        JaegerSpew(JSpew_PICs, "patched CALL path %p (obj: %p)\n",
                   ic.funGuard.executableAddress(),
                   static_cast<void*>(ic.fastGuardedObject));

        return true;
    }

    bool generateStubForClosures(JITScript *from, JSObject *obj)
    {
        JS_ASSERT(ic.frameSize.isStatic());

        
        Assembler masm;

        Registers tempRegs(Registers::AvailRegs);
        tempRegs.takeReg(ic.funObjReg);

        RegisterID t0 = tempRegs.takeAnyReg().reg();

        
        Jump claspGuard = masm.testObjClass(Assembler::NotEqual, ic.funObjReg, t0, &FunctionClass);

        
        JSFunction *fun = obj->getFunctionPrivate();
        masm.loadObjPrivate(ic.funObjReg, t0);
        Jump funGuard = masm.branchPtr(Assembler::NotEqual, t0, ImmPtr(fun));
        Jump done = masm.jump();

        LinkerHelper linker(masm, JSC::METHOD_CODE);
        JSC::ExecutablePool *ep = poolForSize(linker, CallICInfo::Pool_ClosureStub);
        if (!ep)
            return false;

        ic.hasJsFunCheck = true;

        if (!linker.verifyRange(from)) {
            disable(from);
            return true;
        }

        linker.link(claspGuard, ic.slowPathStart);
        linker.link(funGuard, ic.slowPathStart);
        linker.link(done, ic.funGuard.labelAtOffset(ic.hotPathOffset));
        JSC::CodeLocationLabel cs = linker.finalize();

        JaegerSpew(JSpew_PICs, "generated CALL closure stub %p (%lu bytes)\n",
                   cs.executableAddress(), (unsigned long) masm.size());

        Repatcher repatch(from);
        repatch.relink(ic.funJump, cs);

        return true;
    }

    bool generateNativeStub()
    {
        JITScript *jit = f.jit();

        
        uintN initialFrameDepth = f.regs.sp - f.fp()->slots();

        



        CallArgs args;
        if (ic.frameSize.isStatic()) {
            JS_ASSERT(f.regs.sp - f.fp()->slots() == (int)ic.frameSize.staticLocalSlots());
            args = CallArgsFromSp(ic.frameSize.staticArgc(), f.regs.sp);
        } else {
            JS_ASSERT(!f.regs.inlined());
            JS_ASSERT(*f.regs.pc == JSOP_FUNAPPLY && GET_ARGC(f.regs.pc) == 2);
            if (!ic::SplatApplyArgs(f))       
                THROWV(true);
            args = CallArgsFromSp(f.u.call.dynamicArgc, f.regs.sp);
        }

        JSObject *obj;
        if (!IsFunctionObject(args.calleev(), &obj))
            return false;

        JSFunction *fun = obj->getFunctionPrivate();
        if ((!callingNew && !fun->isNative()) || (callingNew && !fun->isConstructor()))
            return false;

        if (callingNew)
            args.thisv().setMagicWithObjectOrNullPayload(NULL);

        RecompilationMonitor monitor(cx);

        if (!CallJSNative(cx, fun->u.n.native, args))
            THROWV(true);

        types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());

        
        if (monitor.recompiled())
            return true;

        
        if (ic.fastGuardedNative || ic.hasJsFunCheck)
            return true;

        
        if (f.regs.inlined())
            return true;

        
        if (!ic.hit) {
            ic.hit = true;
            return true;
        }

        
        Assembler masm;

        
        Jump funGuard = masm.branchPtr(Assembler::NotEqual, ic.funObjReg, ImmPtr(obj));

        





        masm.storePtr(ImmPtr((void *) ic.frameSize.rejoinState(f.pc(), true)),
                      FrameAddress(offsetof(VMFrame, stubRejoin)));

        
        if (ic.frameSize.isDynamic()) {
            masm.bumpStubCounter(f.script(), f.pc(), Registers::tempCallReg());
            masm.fallibleVMCall(cx->typeInferenceEnabled(),
                                JS_FUNC_TO_DATA_PTR(void *, ic::SplatApplyArgs),
                                f.regs.pc, NULL, initialFrameDepth);
        }

        Registers tempRegs = Registers::tempCallRegMask();
        RegisterID t0 = tempRegs.takeAnyReg().reg();
        masm.bumpStubCounter(f.script(), f.pc(), t0);

        int32 storeFrameDepth = ic.frameSize.isStatic() ? initialFrameDepth : -1;
        masm.setupFallibleABICall(cx->typeInferenceEnabled(), f.regs.pc, storeFrameDepth);

        
#ifdef JS_CPU_X86
        RegisterID cxReg = tempRegs.takeAnyReg().reg();
#else
        RegisterID cxReg = Registers::ArgReg0;
#endif
        masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), cxReg);

        




#ifdef JS_CPU_X86
        RegisterID vpReg = t0;
#else
        RegisterID vpReg = Registers::ArgReg2;
#endif
        uint32 vpOffset = (uint32) ((char *) args.base() - (char *) f.fp());
        masm.addPtr(Imm32(vpOffset), JSFrameReg, vpReg);

        
        MaybeRegisterID argcReg;
        if (!ic.frameSize.isStatic()) {
            argcReg = tempRegs.takeAnyReg().reg();
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), argcReg.reg());
        }

        
        if (callingNew) {
            Value v;
            v.setMagicWithObjectOrNullPayload(NULL);
            masm.storeValue(v, Address(vpReg, sizeof(Value)));
        }

        masm.restoreStackBase();
        masm.setupABICall(Registers::NormalCall, 3);
        masm.storeArg(2, vpReg);
        if (ic.frameSize.isStatic())
            masm.storeArg(1, ImmPtr((void *) ic.frameSize.staticArgc()));
        else
            masm.storeArg(1, argcReg.reg());
        masm.storeArg(0, cxReg);

        js::Native native = fun->u.n.native;

        





        if (native == js_regexp_exec && !CallResultEscapes(f.pc()))
            native = js_regexp_test;

        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, native), false);

        NativeStubLinker::FinalJump done;
        if (!NativeStubEpilogue(f, masm, &done, initialFrameDepth, vpOffset, MaybeRegisterID(), MaybeRegisterID()))
            return false;
        NativeStubLinker linker(masm, f.jit(), f.regs.pc, done);
        if (!linker.init(f.cx))
            THROWV(true);

        if (!linker.verifyRange(jit)) {
            disable(jit);
            return true;
        }

        linker.patchJump(ic.slowPathStart.labelAtOffset(ic.slowJoinOffset));

        ic.fastGuardedNative = obj;

        linker.link(funGuard, ic.slowPathStart);
        JSC::CodeLocationLabel start = linker.finalize();

        JaegerSpew(JSpew_PICs, "generated native CALL stub %p (%lu bytes)\n",
                   start.executableAddress(), (unsigned long) masm.size());

        Repatcher repatch(jit);
        repatch.relink(ic.funJump, start);

        return true;
    }

    void *update()
    {
        StackFrame *fp = f.fp();
        JITScript *jit = fp->jit();
        RecompilationMonitor monitor(cx);

        bool lowered = ic.frameSize.lowered(f.pc());
        JS_ASSERT_IF(lowered, !callingNew);

        stubs::UncachedCallResult ucr;
        if (callingNew)
            stubs::UncachedNewHelper(f, ic.frameSize.staticArgc(), &ucr);
        else
            stubs::UncachedCallHelper(f, ic.frameSize.getArgc(f), lowered, &ucr);

        
        
        
        if (monitor.recompiled())
            return ucr.codeAddr;

        
        
        if (!ucr.codeAddr) {
            if (ucr.unjittable)
                disable(jit);
            return NULL;
        }
            
        JSFunction *fun = ucr.fun;
        JS_ASSERT(fun);
        JSScript *script = fun->script();
        JS_ASSERT(script);
        JSObject *callee = ucr.callee;
        JS_ASSERT(callee);

        uint32 flags = callingNew ? StackFrame::CONSTRUCTING : 0;

        if (!ic.hit) {
            ic.hit = true;
            return ucr.codeAddr;
        }

        if (!ic.frameSize.isStatic() || ic.frameSize.staticArgc() != fun->nargs) {
            if (!generateFullCallStub(jit, script, flags))
                THROWV(NULL);
        } else {
            if (!ic.fastGuardedObject && patchInlinePath(jit, script, callee)) {
                
            } else if (ic.fastGuardedObject &&
                       !ic.hasJsFunCheck &&
                       !ic.fastGuardedNative &&
                       ic.fastGuardedObject->getFunctionPrivate() == fun) {
                



                if (!generateStubForClosures(jit, callee))
                    THROWV(NULL);
            } else {
                if (!generateFullCallStub(jit, script, flags))
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

void * JS_FASTCALL
ic::NativeCall(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, false);
    if (!cc.generateNativeStub())
        stubs::SlowCall(f, ic->frameSize.getArgc(f));
    return NULL;
}

void * JS_FASTCALL
ic::NativeNew(VMFrame &f, CallICInfo *ic)
{
    CallCompiler cc(f, *ic, true);
    if (!cc.generateNativeStub())
        stubs::SlowNew(f, ic->frameSize.staticArgc());
    return NULL;
}

static JS_ALWAYS_INLINE bool
BumpStack(VMFrame &f, uintN inc)
{
    if (f.regs.sp + inc < f.stackLimit)
        return true;
    return f.cx->stack.space().tryBumpLimit(f.cx, f.regs.sp, inc, &f.stackLimit);
}







JSBool JS_FASTCALL
ic::SplatApplyArgs(VMFrame &f)
{
    JSContext *cx = f.cx;
    JS_ASSERT(!f.regs.inlined());
    JS_ASSERT(GET_ARGC(f.regs.pc) == 2);

    










    if (f.u.call.lazyArgsObj) {
        Value *vp = f.regs.sp - 3;
        JS_ASSERT(JS_CALLEE(cx, vp).toObject().getFunctionPrivate()->u.n.native == js_fun_apply);

        StackFrame *fp = f.regs.fp();
        if (!fp->hasOverriddenArgs()) {
            uintN n;
            if (!fp->hasArgsObj()) {
                
                n = fp->numActualArgs();
                if (!BumpStack(f, n))
                    THROWV(false);
                Value *argv = JS_ARGV(cx, vp + 1 );
                f.regs.sp += n;
                fp->forEachCanonicalActualArg(CopyTo(argv));
            } else {
                
                JSObject *aobj = &fp->argsObj();

                
                uintN length;
                if (!js_GetLengthProperty(cx, aobj, &length))
                    THROWV(false);

                
                if (length > StackSpace::ARGS_LENGTH_MAX) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TOO_MANY_FUN_APPLY_ARGS);
                    THROWV(false);
                }

                n = length;
                if (!BumpStack(f, n))
                    THROWV(false);

                
                Value *argv = JS_ARGV(cx, &vp[1]);  
                f.regs.sp += n;  
                if (!GetElements(cx, aobj, n, argv))
                    THROWV(false);
            }

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

    
    if (length > StackSpace::ARGS_LENGTH_MAX) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_TOO_MANY_FUN_APPLY_ARGS);
        THROWV(false);
    }

    intN delta = length - 1;
    if (delta > 0 && !BumpStack(f, delta))
        THROWV(false);
    f.regs.sp += delta;

    
    if (!GetElements(cx, aobj, length, f.regs.sp - length))
        THROWV(false);

    f.u.call.dynamicArgc = length;
    return true;
}

void
ic::GenerateArgumentCheckStub(VMFrame &f)
{
    JS_ASSERT(f.cx->typeInferenceEnabled());

    JITScript *jit = f.jit();
    StackFrame *fp = f.fp();
    JSFunction *fun = fp->fun();
    JSScript *script = fun->script();

    if (jit->argsCheckPool)
        jit->resetArgsCheck();

    Assembler masm;
    Vector<Jump> mismatches(f.cx);

    if (!f.fp()->isConstructing()) {
        types::TypeSet *types = types::TypeScript::ThisTypes(script);
        Address address(JSFrameReg, StackFrame::offsetOfThis(fun));
        if (!masm.generateTypeCheck(f.cx, address, types, &mismatches))
            return;
    }

    for (unsigned i = 0; i < fun->nargs; i++) {
        types::TypeSet *types = types::TypeScript::ArgTypes(script, i);
        Address address(JSFrameReg, StackFrame::offsetOfFormalArg(fun, i));
        if (!masm.generateTypeCheck(f.cx, address, types, &mismatches))
            return;
    }

    Jump done = masm.jump();

    LinkerHelper linker(masm, JSC::METHOD_CODE);
    JSC::ExecutablePool *ep = linker.init(f.cx);
    if (!ep)
        return;
    jit->argsCheckPool = ep;

    if (!linker.verifyRange(jit)) {
        jit->resetArgsCheck();
        return;
    }

    for (unsigned i = 0; i < mismatches.length(); i++)
        linker.link(mismatches[i], jit->argsCheckStub);
    linker.link(done, jit->argsCheckFallthrough);

    JSC::CodeLocationLabel cs = linker.finalize();

    JaegerSpew(JSpew_PICs, "generated ARGS CHECK stub %p (%lu bytes)\n",
               cs.executableAddress(), (unsigned long)masm.size());

    Repatcher repatch(jit);
    repatch.relink(jit->argsCheckJump, cs);
}

void
JITScript::resetArgsCheck()
{
    argsCheckPool->release();
    argsCheckPool = NULL;

    Repatcher repatch(this);
    repatch.relink(argsCheckJump, argsCheckStub);
}

void
JITScript::purgeMICs()
{
    if (!nGetGlobalNames || !nSetGlobalNames)
        return;

    Repatcher repatch(this);

    ic::GetGlobalNameIC *getGlobalNames_ = getGlobalNames();
    for (uint32 i = 0; i < nGetGlobalNames; i++) {
        ic::GetGlobalNameIC &ic = getGlobalNames_[i];
        JSC::CodeLocationDataLabelPtr label = ic.fastPathStart.dataLabelPtrAtOffset(ic.shapeOffset);
        repatch.repatch(label, NULL);
    }

    ic::SetGlobalNameIC *setGlobalNames_ = setGlobalNames();
    for (uint32 i = 0; i < nSetGlobalNames; i++) {
        ic::SetGlobalNameIC &ic = setGlobalNames_[i];
        ic.patchInlineShapeGuard(repatch, NULL);

        if (ic.hasExtraStub) {
            Repatcher repatcher(ic.extraStub);
            ic.patchExtraShapeGuard(repatcher, NULL);
        }
    }
}

void
ic::PurgeMICs(JSContext *cx, JSScript *script)
{
    if (script->jitNormal)
        script->jitNormal->purgeMICs();
    if (script->jitCtor)
        script->jitCtor->purgeMICs();
}

void
JITScript::nukeScriptDependentICs()
{
    if (!nCallICs)
        return;

    Repatcher repatcher(this);

    ic::CallICInfo *callICs_ = callICs();
    for (uint32 i = 0; i < nCallICs; i++) {
        ic::CallICInfo &ic = callICs_[i];
        if (!ic.fastGuardedObject)
            continue;
        repatcher.repatch(ic.funGuard, NULL);
        repatcher.relink(ic.funJump, ic.slowPathStart);
        ic.releasePool(CallICInfo::Pool_ClosureStub);
        ic.fastGuardedObject = NULL;
        ic.hasJsFunCheck = false;
    }
}

void
JITScript::sweepCallICs(JSContext *cx, bool purgeAll)
{
    Repatcher repatcher(this);

    





    ic::CallICInfo *callICs_ = callICs();
    for (uint32 i = 0; i < nCallICs; i++) {
        ic::CallICInfo &ic = callICs_[i];

        




        bool fastFunDead = ic.fastGuardedObject &&
            (purgeAll || IsAboutToBeFinalized(cx, ic.fastGuardedObject));
        bool hasNative = ic.fastGuardedNative != NULL;

        









        if (purgeAll || hasNative || (fastFunDead && ic.hasJsFunCheck)) {
            repatcher.relink(ic.funJump, ic.slowPathStart);
            ic.hit = false;
        }

        if (fastFunDead) {
            repatcher.repatch(ic.funGuard, NULL);
            ic.purgeGuardedObject();
        }

        if (hasNative)
            ic.fastGuardedNative = NULL;

        if (purgeAll) {
            ic.releasePool(CallICInfo::Pool_ScriptStub);
            JSC::CodeLocationJump oolJump = ic.slowPathStart.jumpAtOffset(ic.oolJumpOffset);
            JSC::CodeLocationLabel icCall = ic.slowPathStart.labelAtOffset(ic.icCallOffset);
            repatcher.relink(oolJump, icCall);
        }
    }

    
    if (argsCheckPool)
        resetArgsCheck();

    if (purgeAll) {
        
        uint32 released = 0;

        ic::EqualityICInfo *equalityICs_ = equalityICs();
        for (uint32 i = 0; i < nEqualityICs; i++) {
            ic::EqualityICInfo &ic = equalityICs_[i];
            if (!ic.generated)
                continue;

            JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, ic::Equality));
            repatcher.relink(ic.stubCall, fptr);
            repatcher.relink(ic.jumpToStub, ic.stubEntry);

            ic.generated = false;
            released++;
        }

        ic::SetGlobalNameIC *setGlobalNames_ = setGlobalNames();
        for (uint32 i = 0; i < nSetGlobalNames; i ++) {
            ic::SetGlobalNameIC &ic = setGlobalNames_[i];
            if (!ic.hasExtraStub)
                continue;
            repatcher.relink(ic.fastPathStart.jumpAtOffset(ic.inlineShapeJump), ic.slowPathStart);
            ic.hasExtraStub = false;
            released++;
        }

        JS_ASSERT(released == execPools.length());
        for (uint32 i = 0; i < released; i++)
            execPools[i]->release();
        execPools.clear();
    }
}

void
ic::SweepCallICs(JSContext *cx, JSScript *script, bool purgeAll)
{
    if (script->jitNormal)
        script->jitNormal->sweepCallICs(cx, purgeAll);
    if (script->jitCtor)
        script->jitCtor->sweepCallICs(cx, purgeAll);
}

#endif 


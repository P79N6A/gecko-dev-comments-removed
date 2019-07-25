






































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
#include "methodjit/MethodJIT-inl.h"
#include "methodjit/PolyIC.h"
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
    JSAtom *atom = f.fp()->script()->getAtom(GET_INDEX(f.regs.pc));
    jsid id = ATOM_TO_JSID(atom);

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

    
    Repatcher repatcher(f.jit());
    repatcher.repatch(ic->fastPathStart.dataLabel32AtOffset(ic->shapeOffset), obj->shape());

    
    JSC::CodeLocationLabel label = ic->fastPathStart.labelAtOffset(ic->loadStoreOffset);
    repatcher.patchAddressOffsetForValueLoad(label, slot * sizeof(Value));

    
    stubs::GetGlobalName(f);
}

template <JSBool strict>
static void JS_FASTCALL
DisabledSetGlobal(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSScript *script = f.fp()->script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.regs.pc));
    stubs::SetGlobalName<strict>(f, atom);
}

template void JS_FASTCALL DisabledSetGlobal<true>(VMFrame &f, ic::SetGlobalNameIC *ic);
template void JS_FASTCALL DisabledSetGlobal<false>(VMFrame &f, ic::SetGlobalNameIC *ic);

template <JSBool strict>
static void JS_FASTCALL
DisabledSetGlobalNoCache(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSScript *script = f.fp()->script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.regs.pc));
    stubs::SetGlobalNameNoCache<strict>(f, atom);
}

template void JS_FASTCALL DisabledSetGlobalNoCache<true>(VMFrame &f, ic::SetGlobalNameIC *ic);
template void JS_FASTCALL DisabledSetGlobalNoCache<false>(VMFrame &f, ic::SetGlobalNameIC *ic);

static void
PatchSetFallback(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSScript *script = f.fp()->script();

    Repatcher repatch(f.jit());
    VoidStubSetGlobal stub = ic->usePropertyCache
                             ? STRICT_VARIANT(DisabledSetGlobal)
                             : STRICT_VARIANT(DisabledSetGlobalNoCache);
    JSC::FunctionPtr fptr(JS_FUNC_TO_DATA_PTR(void *, stub));
    repatch.relink(ic->slowPathCall, fptr);
}

void
SetGlobalNameIC::patchExtraShapeGuard(Repatcher &repatcher, int32 shape)
{
    JS_ASSERT(hasExtraStub);

    JSC::CodeLocationLabel label(JSC::MacroAssemblerCodePtr(extraStub.start()));
    repatcher.repatch(label.dataLabel32AtOffset(extraShapeGuard), shape);
}

void
SetGlobalNameIC::patchInlineShapeGuard(Repatcher &repatcher, int32 shape)
{
    JSC::CodeLocationDataLabel32 label = fastPathStart.dataLabel32AtOffset(shapeOffset);
    repatcher.repatch(label, shape);
}

static LookupStatus
UpdateSetGlobalNameStub(VMFrame &f, ic::SetGlobalNameIC *ic, JSObject *obj, const Shape *shape)
{
    Repatcher repatcher(ic->extraStub);

    ic->patchExtraShapeGuard(repatcher, obj->shape());

    JSC::CodeLocationLabel label(JSC::MacroAssemblerCodePtr(ic->extraStub.start()));
    label = label.labelAtOffset(ic->extraStoreOffset);
    repatcher.patchAddressOffsetForValueStore(label, shape->slot * sizeof(Value),
                                              ic->vr.isTypeKnown());

    return Lookup_Cacheable;
}

static LookupStatus
AttachSetGlobalNameStub(VMFrame &f, ic::SetGlobalNameIC *ic, JSObject *obj, const Shape *shape)
{
    Assembler masm;

    Label start = masm.label();

    DataLabel32 shapeLabel;
    Jump guard = masm.branch32WithPatch(Assembler::NotEqual, ic->shapeReg, Imm32(obj->shape()),
                                        shapeLabel);

    
    if (ic->objConst)
        masm.move(ImmPtr(obj), ic->objReg);

    JS_ASSERT(obj->branded());

    



    masm.loadPtr(Address(ic->objReg, offsetof(JSObject, slots)), ic->shapeReg);

    
    Address slot(ic->shapeReg, sizeof(Value) * shape->slot);
    Jump isNotObject = masm.testObject(Assembler::NotEqual, slot);

    
    masm.loadPayload(slot, ic->shapeReg);
    Jump isFun = masm.testFunction(Assembler::Equal, ic->shapeReg);

    
    if (ic->objConst)
        masm.move(ImmPtr(obj), ic->objReg);
    masm.loadPtr(Address(ic->objReg, offsetof(JSObject, slots)), ic->shapeReg);

    
    isNotObject.linkTo(masm.label(), &masm);
    DataLabel32 store = masm.storeValueWithAddressOffsetPatch(ic->vr, slot);

    Jump done = masm.jump();

    JITScript *jit = f.jit();
    LinkerHelper linker(masm);
    JSC::ExecutablePool *ep = linker.init(f.cx);
    if (!ep)
        return Lookup_Error;
    if (!jit->execPools.append(ep)) {
        ep->release();
        js_ReportOutOfMemory(f.cx);
        return Lookup_Error;
    }

    if (!linker.verifyRange(jit))
        return Lookup_Uncacheable;

    linker.link(done, ic->fastPathStart.labelAtOffset(ic->fastRejoinOffset));
    linker.link(guard, ic->slowPathStart);
    linker.link(isFun, ic->slowPathStart);

    JSC::CodeLocationLabel cs = linker.finalize();
    JaegerSpew(JSpew_PICs, "generated setgname stub at %p\n", cs.executableAddress());

    Repatcher repatcher(f.jit());
    repatcher.relink(ic->fastPathStart.jumpAtOffset(ic->inlineShapeJump), cs);

    int offset = linker.locationOf(shapeLabel) - linker.locationOf(start);
    ic->extraShapeGuard = offset;
    JS_ASSERT(ic->extraShapeGuard == offset);

    ic->extraStub = JSC::JITCode(cs.executableAddress(), linker.size());
    offset = linker.locationOf(store) - linker.locationOf(start);
    ic->extraStoreOffset = offset;
    JS_ASSERT(ic->extraStoreOffset == offset);

    ic->hasExtraStub = true;

    return Lookup_Cacheable;
}

static LookupStatus
UpdateSetGlobalName(VMFrame &f, ic::SetGlobalNameIC *ic, JSObject *obj, const Shape *shape)
{
    
    if (!shape)
        return Lookup_Uncacheable;

    if (shape->isMethod() ||
        !shape->hasDefaultSetter() ||
        !shape->writable() ||
        !shape->hasSlot())
    {
        
        PatchSetFallback(f, ic);
        return Lookup_Uncacheable;
    }

    
    if (obj->branded()) {
        





        const Value &v = obj->getSlot(shape->slot);
        if (v.isObject() && v.toObject().isFunction()) {
            



            if (!ChangesMethodValue(v, f.regs.sp[-1]))
                PatchSetFallback(f, ic);
            return Lookup_Uncacheable;
        }

        if (ic->hasExtraStub)
            return UpdateSetGlobalNameStub(f, ic, obj, shape);

        return AttachSetGlobalNameStub(f, ic, obj, shape);
    }

    
    Repatcher repatcher(f.jit());
    ic->patchInlineShapeGuard(repatcher, obj->shape());

    JSC::CodeLocationLabel label = ic->fastPathStart.labelAtOffset(ic->loadStoreOffset);
    repatcher.patchAddressOffsetForValueStore(label, shape->slot * sizeof(Value),
                                              ic->vr.isTypeKnown());

    return Lookup_Cacheable;
}

void JS_FASTCALL
ic::SetGlobalName(VMFrame &f, ic::SetGlobalNameIC *ic)
{
    JSObject *obj = f.fp()->scopeChain().getGlobal();
    JSScript *script = f.fp()->script();
    JSAtom *atom = script->getAtom(GET_INDEX(f.regs.pc));
    const Shape *shape = obj->nativeLookup(ATOM_TO_JSID(atom));

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
        : LinkerHelper(masm), f(f)
    { }

    bool init(JSContext *cx) {
        JSC::ExecutablePool *pool = LinkerHelper::init(cx);
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

static const unsigned MANY_ARGS = 1024;
static const unsigned MIN_SPACE = 500;

static bool
BumpStackFull(VMFrame &f, uintN inc)
{
    
    if (inc < MANY_ARGS) {
        if (f.regs.sp + inc < f.stackLimit)
            return true;
        StackSpace &stack = f.cx->stack();
        if (!stack.bumpCommitAndLimit(f.entryfp, f.regs.sp, inc, &f.stackLimit)) {
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
    if (stack.bumpCommitAndLimit(f.entryfp, f.regs.sp, incWithSpace, &f.stackLimit))
        return true;

    if (!stack.ensureSpace(f.cx, f.regs.sp, incWithSpace))
        return false;
    f.stackLimit = bumpedWithSpace;
    return true;
}

static JS_ALWAYS_INLINE bool
BumpStack(VMFrame &f, uintN inc)
{
    
    if (inc < MANY_ARGS && f.regs.sp + inc < f.stackLimit)
        return true;
    return BumpStackFull(f, inc);
}







template <bool LazyArgsObj>
JSBool JS_FASTCALL
ic::SplatApplyArgs(VMFrame &f)
{
    JSContext *cx = f.cx;
    JS_ASSERT(GET_ARGC(f.regs.pc) == 2);

    










    if (LazyArgsObj) {
        Value *vp = f.regs.sp - 3;
        JS_ASSERT(JS_CALLEE(cx, vp).toObject().getFunctionPrivate()->u.n.native == js_fun_apply);

        JSStackFrame *fp = f.regs.fp;
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

                
                JS_ASSERT(length <= JS_ARGS_LENGTH_MAX);
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
template JSBool JS_FASTCALL ic::SplatApplyArgs<true>(VMFrame &f);
template JSBool JS_FASTCALL ic::SplatApplyArgs<false>(VMFrame &f);

template <bool IsNew>
static bool
CallHelper(VMFrame &f, ic::CallIC *ic, CallDescription &call)
{
    Value *vp = f.regs.sp - (2 + call.argc);

    
    
    if (vp->isObject()) {
        call.callee = &vp->toObject();
        call.fun = call.callee->isFunction() ? call.callee->getFunctionPrivate() : NULL;
    } else {
        call.callee = call.fun = NULL;
    }

    if (!call.fun || call.fun->isNative()) {
        if (IsNew)
            return InvokeConstructor(f.cx, InvokeArgsAlreadyOnTheStack(vp, call.argc));
        if (!call.fun)
            return Invoke(f.cx, InvokeArgsAlreadyOnTheStack(vp, call.argc), 0);
        return CallJSNative(f.cx, call.fun->u.n.native, call.argc, vp);
    }

    
    uint32 flags = IsNew ? JSFRAME_CONSTRUCTING : 0;
    JSScript *newscript = call.fun->script();
    StackSpace &stack = f.cx->stack();
    JSStackFrame *newfp = stack.getInlineFrameWithinLimit(f.cx, f.regs.sp, call.argc,
                                                          call.fun, newscript, &flags,
                                                          f.entryfp, &f.stackLimit);
    if (!newfp)
        return false;

    
    newfp->initCallFrame(f.cx, *call.callee, call.fun, call.argc, flags);
    SetValueRangeToUndefined(newfp->slots(), newscript->nfixed);

    
    stack.pushInlineFrame(f.cx, newscript, newfp, &f.regs);
    JS_ASSERT(newfp == f.regs.fp);

    
    if (call.fun->isHeavyweight() && !js::CreateFunCallObject(f.cx, newfp))
        return false;

    
    call.unjittable = false;
    if (newscript->getJITStatus(IsNew) == JITScript_None) {
        CompileStatus status = CanMethodJIT(f.cx, newscript, newfp, CompileRequest_Interpreter);
        if (status == Compile_Error) {
            
            InlineReturn(f);
            return false;
        }
        if (status == Compile_Abort)
            call.unjittable = true;
    }

    
    if (JITScript *jit = newscript->getJIT(IsNew)) {
        newfp->setNativeReturnAddress(ic->returnAddress());
        call.code = jit->invokeEntry;
        return true;
    }

    
    bool ok = !!Interpret(f.cx, f.fp());
    InlineReturn(f);
    call.code = NULL;

    return ok;
}

template <bool IsNew, bool UpdateIC>
static bool
CallICTail(VMFrame &f, ic::CallIC *ic, CallDescription &call)
{
    JITScript *jit = f.jit();
    (void)jit;

    if (!CallHelper<IsNew>(f, ic, call))
        return false;
    if (UpdateIC) {
        if (!ic->update(f.cx, jit, call))
            return false;
    }
    return true;
}

void * JS_FASTCALL
ic::FailedFunApplyLazyArgs(VMFrame &f, ic::CallIC *ic)
{
    
    f.regs.sp++;
    if (!js_GetArgsValue(f.cx, f.fp(), &f.regs.sp[-1]))
        THROWV(NULL);
    return ic::FailedFunApply(f, ic);
}

void * JS_FASTCALL
ic::FailedFunApply(VMFrame &f, ic::CallIC *ic)
{
    CallDescription call(2);
    if (!CallICTail<false, false>(f, ic, call))
        THROWV(NULL);
    return call.code;
}

void * JS_FASTCALL
ic::FailedFunCall(VMFrame &f, ic::CallIC *ic)
{
    CallDescription call(ic->frameSize.staticArgc() + 1);
    if (!CallICTail<false, false>(f, ic, call))
        THROWV(NULL);
    return call.code;
}

template <bool DynamicArgc, bool UpdateIC>
void * JS_FASTCALL
ic::Call(VMFrame &f, ic::CallIC *ic)
{
    CallDescription call(DynamicArgc
                         ? f.u.call.dynamicArgc
                         : ic->frameSize.staticArgc());
    if (!CallICTail<false, UpdateIC>(f, ic, call))
        THROWV(NULL);
    return call.code;
}
template void * JS_FASTCALL ic::Call<true, true>(VMFrame &f, ic::CallIC *ic);
template void * JS_FASTCALL ic::Call<true, false>(VMFrame &f, ic::CallIC *ic);
template void * JS_FASTCALL ic::Call<false, true>(VMFrame &f, ic::CallIC *ic);
template void * JS_FASTCALL ic::Call<false, false>(VMFrame &f, ic::CallIC *ic);

template <bool UpdateIC>
void * JS_FASTCALL
ic::New(VMFrame &f, ic::CallIC *ic)
{
    CallDescription call(ic->frameSize.staticArgc());
    if (!CallICTail<true, UpdateIC>(f, ic, call))
        THROWV(NULL);
    return call.code;
}
template void * JS_FASTCALL ic::New<true>(VMFrame &f, ic::CallIC *ic);
template void * JS_FASTCALL ic::New<false>(VMFrame &f, ic::CallIC *ic);

JSOp
ic::CallIC::op() const
{
    return JSOp(*pc);
}

class CallAssembler
{
    JSContext *cx;
    const ic::CallIC &ic;
    JITScript *jit;
    CallDescription &call;
    Registers tempRegs;

    
    MaybeJump claspGuard;

    
    MaybeJump nativeDone;
    MaybeJump takeSlowPath;
    MaybeJump interpDone;

  public:
    Assembler masm;

  public:
    CallAssembler(JSContext *cx, ic::CallIC &ic, JITScript *jit, CallDescription &call)
      : cx(cx),
        ic(ic),
        jit(jit),
        call(call)
    {
        tempRegs.takeReg(ic.calleeData);
    }

    void emitStaticFrame(uint32 frameDepth)
    {
        masm.emitStaticFrame(ic.op() == JSOP_NEW, frameDepth, ic.returnAddress());
    }

    void emitDynamicFrame()
    {
        RegisterID newfp = tempRegs.takeAnyReg();
        masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), newfp);
    
        Address flagsAddr(newfp, JSStackFrame::offsetOfFlags());
        uint32 extraFlags = (ic.op() == JSOP_NEW) ? JSFRAME_CONSTRUCTING : 0;
        masm.store32(Imm32(JSFRAME_FUNCTION | extraFlags), flagsAddr);
    
        Address prevAddr(newfp, JSStackFrame::offsetOfPrev());
        masm.storePtr(JSFrameReg, prevAddr);
    
        Address ncodeAddr(newfp, JSStackFrame::offsetOfncode());
        masm.storePtr(ImmPtr(ic.returnAddress()), ncodeAddr);
    
        masm.move(newfp, JSFrameReg);
        tempRegs.putReg(newfp);
    }

    Jump emitNativeCall(Native native)
    {
        
        
        if (native == js_regexp_exec && !CallResultEscapes(cx->regs->pc))
            native = js_regexp_test;

        Registers tempRegs; 
#ifndef JS_CPU_X86
        tempRegs.takeReg(Registers::ArgReg0);
        tempRegs.takeReg(Registers::ArgReg1);
        tempRegs.takeReg(Registers::ArgReg2);
#endif

        
        masm.storePtr(ImmPtr(ic.pc), FrameAddress(offsetof(VMFrame, regs.pc)));

        
        
        RegisterID t0 = tempRegs.takeAnyReg();
        if (ic.frameSize.isStatic()) {
            uint32 spOffset = sizeof(JSStackFrame) + ic.frameSize.staticLocalSlots() * sizeof(Value);
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
        RegisterID argcReg = tempRegs.takeAnyReg();
        RegisterID vpReg = t0;
#else
        RegisterID argcReg = Registers::ArgReg1;
        RegisterID vpReg = Registers::ArgReg2;
#endif
        if (ic.frameSize.isStatic()) {
            uint32 vpOffset = ic.frameSize.staticLocalSlots() - (ic.frameSize.staticArgc() + 2);
            masm.addPtr(Imm32(sizeof(JSStackFrame) + vpOffset * sizeof(Value)), JSFrameReg, vpReg);
        } else {
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), argcReg);
            masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), vpReg);

            
            RegisterID vpOff = tempRegs.takeAnyReg();
            masm.move(argcReg, vpOff);
            masm.add32(Imm32(2), vpOff); 
            JS_STATIC_ASSERT(sizeof(Value) == 8);
            masm.lshift32(Imm32(3), vpOff);
            masm.subPtr(vpOff, vpReg);
            tempRegs.putReg(vpOff);
        }

        
        if (ic.op() == JSOP_NEW) {
            Value v;
            v.setMagicWithObjectOrNullPayload(NULL);
            masm.storeValue(v, Address(vpReg, sizeof(Value)));
        }

        masm.setupABICall(Registers::NormalCall, 3);
        masm.storeArg(2, vpReg);
        if (ic.frameSize.isStatic())
            masm.storeArg(1, Imm32(ic.frameSize.staticArgc()));
        else
            masm.storeArg(1, argcReg);
        masm.storeArg(0, cxReg);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, native), false);

        Jump hasException = masm.branchTest32(Assembler::Zero, Registers::ReturnReg,
                                              Registers::ReturnReg);

        Address rval(JSFrameReg, ic.rvalOffset);
        masm.loadValueAsComponents(rval, JSReturnReg_Type, JSReturnReg_Data);
        Jump done = masm.jump();

        hasException.linkTo(masm.label(), &masm);
        masm.throwInJIT();

        return done;
    }

    void emitLoadArgc(RegisterID reg)
    {
        if (ic.frameSize.isStatic())
            masm.move(Imm32(ic.frameSize.staticArgc()), reg);
        else
            masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), reg);
    }

    void emitGeneric()
    {
        RegisterID tmp = tempRegs.takeAnyReg();

        
        masm.loadObjPrivate(ic.calleeData, ic.calleeData);
        masm.load16(Address(ic.calleeData, offsetof(JSFunction, flags)), tmp);
        masm.and32(Imm32(JSFUN_KINDMASK), tmp);

        takeSlowPath = masm.branch32(Assembler::Below, tmp, Imm32(JSFUN_INTERPRETED));
        if (ic.frameSize.isStatic())
            emitStaticFrame(ic.frameSize.staticLocalSlots());
        else
            emitDynamicFrame();

        Address scriptAddr(ic.calleeData, offsetof(JSFunction, u.i.script));
        masm.loadPtr(scriptAddr, tmp);

        
        size_t offset = (ic.op() == JSOP_NEW)
                        ? offsetof(JSScript, jitArityCheckCtor)
                        : offsetof(JSScript, jitArityCheckNormal);
        masm.loadPtr(Address(tmp, offset), Registers::ReturnReg);
        Jump hasCode = masm.branchPtr(Assembler::Above, Registers::ReturnReg,
                                      ImmPtr(JS_UNJITTABLE_SCRIPT));

        
        masm.storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));
        emitLoadArgc(Registers::ArgReg1);

        void *ptr = JS_FUNC_TO_DATA_PTR(void *, stubs::CompileFunction);
        if (ic.frameSize.isStatic())
            masm.fallibleVMCall(ptr, NULL, ic.frameSize.staticLocalSlots());
        else
            masm.fallibleVMCall(ptr, NULL, -1);

        
        masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);

        
        
        
        
        interpDone = masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                        Registers::ReturnReg);

        masm.jump(Registers::ReturnReg);

        hasCode.linkTo(masm.label(), &masm);
        emitLoadArgc(JSParamReg_Argc);
        masm.jump(Registers::ReturnReg);
    }

    bool link(Repatcher &repatcher)
    {
        JS_ASSERT(!ic.pool);
        JS_ASSERT(!ic.guardedNative);

        LinkerHelper linker(masm);
        JSC::ExecutablePool *ep = linker.init(cx);
        if (!ep)
            return false;
        if (!linker.verifyRange(jit)) {
            ep->release();
            return true;
        }

        ic.pool = ep;
        linker.link(claspGuard.get(), ic.slowPathStart);
        linker.link(takeSlowPath.get(), ic.slowPathStart);
        linker.link(interpDone.get(), ic.completedRejoinLabel());

        if (nativeDone.isSet()) {
            
            
            linker.link(nativeDone.get(), ic.completedRejoinLabel());
            ic.guardedNative = call.callee;
        }

        JSC::CodeLocationLabel cs = linker.finalize();
        JaegerSpew(JSpew_PICs, "generated call IC at %p\n", cs.executableAddress());

        
        repatcher.relink(ic.inlineJump(), cs);
        return true;
    }

  public:
    bool assemble(Repatcher &repatcher)
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
 
        
        MaybeJump calleeGuard;
        if (call.fun->isNative()) {
            calleeGuard = masm.branchPtr(Assembler::NotEqual, ic.calleeData, ImmPtr(call.callee));
            nativeDone = emitNativeCall(call.fun->u.n.native);
        }
    
        
        
        
        
        if (calleeGuard.isSet())
            calleeGuard.get().linkTo(masm.label(), &masm);
        claspGuard = masm.testFunction(Assembler::NotEqual, ic.calleeData);

        emitGeneric();

        return link(repatcher);
    }
};

typedef void * (JS_FASTCALL *CallICFun)(VMFrame &, ic::CallIC *);
template <bool UpdateIC>
CallICFun GetCallICFun(JSOp op, const FrameSize &frameSize)
{
    if (op == JSOP_NEW)
        return ic::New<UpdateIC>;
    if (frameSize.isStatic())
        return ic::Call<false, UpdateIC>;
    return ic::Call<true, UpdateIC>;
}

void
CallIC::disable(Repatcher &repatcher)
{
    CallICFun fun = GetCallICFun<false>(op(), frameSize);
    repatcher.relink(slowCall(), JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, fun)));
}

void
CallIC::reenable(JITScript *jit, Repatcher &repatcher)
{
    
    if (jit->script->debugMode)
        return;
    CallICFun fun = GetCallICFun<true>(op(), frameSize);
    repatcher.relink(slowCall(), JSC::FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, fun)));
}

void
CallIC::purgeInlineGuard(Repatcher &repatcher)
{
    JS_ASSERT(hasExtendedInlinePath);
    repatcher.repatch(calleePtr(), NULL);
    inlineCallee = NULL;
}

void
CallIC::purgeStub(Repatcher &repatcher)
{
    repatcher.relink(inlineJump(), slowPathStart);
    pool->release();
    pool = NULL;
    guardedNative = NULL;
}

void
CallIC::purge(JITScript *jit, Repatcher &repatcher)
{
    reenable(jit, repatcher);
    if (inlineCallee)
        purgeInlineGuard(repatcher);
    if (pool)
        purgeStub(repatcher);
}

bool
CallIC::shouldDisable(JSContext *cx, JITScript *jit, CallDescription &call)
{
    
    if (!call.fun)
        return true;

    
    if (call.fun->isInterpreted() && call.unjittable)
        return true;

    
    JS_ASSERT(!jit->script->debugMode);
    return false;
}

bool
CallIC::update(JSContext *cx, JITScript *jit, CallDescription &call)
{
    if (shouldDisable(cx, jit, call)) {
        Repatcher repatcher(jit);
        disable(repatcher);
        return true;
    }

    
    
    if (call.fun->isInterpreted() && !call.code) {
        
        JS_ASSERT(!call.unjittable);
        return true;
    }

    
    Repatcher repatcher(jit);
    disable(repatcher);

    
    
    

    
    if (call.fun->isInterpreted() &&
        hasExtendedInlinePath &&
        frameSize.staticArgc() == call.fun->nargs &&
        !inlineCallee)
    {
        JS_ASSERT(cx->fp()->jit() == call.fun->script()->getJIT(op() == JSOP_NEW));

        JSC::CodeLocationLabel target(cx->fp()->jit()->fastEntry);
        repatcher.relink(inlineCall(), target);
        repatcher.repatch(calleePtr(), call.callee);
        inlineCallee = call.callee;
    }

    
    if (!pool) {
        CallAssembler ca(cx, *this, jit, call);
        return ca.assemble(repatcher);
    }
    return true;
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
        JSC::CodeLocationDataLabel32 label = ic.fastPathStart.dataLabel32AtOffset(ic.shapeOffset);
        repatch.repatch(label, int(INVALID_SHAPE));
    }

    ic::SetGlobalNameIC *setGlobalNames_ = setGlobalNames();
    for (uint32 i = 0; i < nSetGlobalNames; i++) {
        ic::SetGlobalNameIC &ic = setGlobalNames_[i];
        ic.patchInlineShapeGuard(repatch, int32(INVALID_SHAPE));

        if (ic.hasExtraStub) {
            Repatcher repatcher(ic.extraStub);
            ic.patchExtraShapeGuard(repatcher, int32(INVALID_SHAPE));
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
JITScript::sweepICs(JSContext *cx)
{
    Repatcher repatcher(this);

    ic::CallIC *callICs_ = callICs();
    for (uint32 i = 0; i < nCallICs; i++) {
        ic::CallIC &ic = callICs_[i];
        bool killedInline = ic.inlineCallee && IsAboutToBeFinalized(cx, ic.inlineCallee);
        bool killedNative = ic.guardedNative && IsAboutToBeFinalized(cx, ic.guardedNative);
        if (killedInline)
            ic.purgeInlineGuard(repatcher);
        if (killedNative)
            ic.purgeStub(repatcher);
        if (killedInline || killedNative)
            ic.reenable(this, repatcher);
    }
}

void
JITScript::purgeICs(JSContext *cx)
{
    Repatcher repatcher(this);

    





    uint32 released = 0;

    ic::CallIC *callICs_ = callICs();
    for (uint32 i = 0; i < nCallICs; i++)
        callICs_[i].purge(this, repatcher);

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

void
ic::SweepICs(JSContext *cx, JSScript *script)
{
    if (script->jitNormal)
        script->jitNormal->sweepICs(cx);
    if (script->jitCtor)
        script->jitCtor->sweepICs(cx);
}

void
ic::PurgeICs(JSContext *cx, JSScript *script)
{
    if (script->jitNormal)
        script->jitNormal->purgeICs(cx);
    if (script->jitCtor)
        script->jitCtor->purgeICs(cx);
}

#endif 


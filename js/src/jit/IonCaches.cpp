





#include "jit/IonCaches.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/TemplateLib.h"

#include "jsproxy.h"
#include "jstypes.h"

#include "builtin/TypeRepresentation.h"
#include "jit/Ion.h"
#include "jit/IonLinker.h"
#include "jit/IonSpewer.h"
#include "jit/Lowering.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/ParallelFunctions.h"
#include "jit/VMFunctions.h"
#include "vm/Shape.h"

#include "jit/IonFrames-inl.h"
#include "vm/Interpreter-inl.h"
#include "vm/Shape-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;
using mozilla::tl::FloorLog2;

void
CodeLocationJump::repoint(IonCode *code, MacroAssembler *masm)
{
    JS_ASSERT(state_ == Relative);
    size_t new_off = (size_t)raw_;
#ifdef JS_SMALL_BRANCH
    size_t jumpTableEntryOffset = reinterpret_cast<size_t>(jumpTableEntry_);
#endif
    if (masm != nullptr) {
#ifdef JS_CPU_X64
        JS_ASSERT((uint64_t)raw_ <= UINT32_MAX);
#endif
        new_off = masm->actualOffset((uintptr_t)raw_);
#ifdef JS_SMALL_BRANCH
        jumpTableEntryOffset = masm->actualIndex(jumpTableEntryOffset);
#endif
    }
    raw_ = code->raw() + new_off;
#ifdef JS_SMALL_BRANCH
    jumpTableEntry_ = Assembler::PatchableJumpAddress(code, (size_t) jumpTableEntryOffset);
#endif
    setAbsolute();
}

void
CodeLocationLabel::repoint(IonCode *code, MacroAssembler *masm)
{
     JS_ASSERT(state_ == Relative);
     size_t new_off = (size_t)raw_;
     if (masm != nullptr) {
#ifdef JS_CPU_X64
        JS_ASSERT((uint64_t)raw_ <= UINT32_MAX);
#endif
        new_off = masm->actualOffset((uintptr_t)raw_);
     }
     JS_ASSERT(new_off < code->instructionsSize());

     raw_ = code->raw() + new_off;
     setAbsolute();
}

void
CodeOffsetLabel::fixup(MacroAssembler *masm)
{
     offset_ = masm->actualOffset(offset_);
}

void
CodeOffsetJump::fixup(MacroAssembler *masm)
{
     offset_ = masm->actualOffset(offset_);
#ifdef JS_SMALL_BRANCH
     jumpTableIndex_ = masm->actualIndex(jumpTableIndex_);
#endif
}

const char *
IonCache::CacheName(IonCache::Kind kind)
{
    static const char * const names[] =
    {
#define NAME(x) #x,
        IONCACHE_KIND_LIST(NAME)
#undef NAME
    };
    return names[kind];
}

IonCache::LinkStatus
IonCache::linkCode(JSContext *cx, MacroAssembler &masm, IonScript *ion, IonCode **code)
{
    Linker linker(masm);
    *code = linker.newCode<CanGC>(cx, JSC::ION_CODE);
    if (!*code)
        return LINK_ERROR;

    if (ion->invalidated())
        return CACHE_FLUSHED;

    return LINK_GOOD;
}

const size_t IonCache::MAX_STUBS = 16;










































class IonCache::StubAttacher
{
  protected:
    bool hasNextStubOffset_ : 1;
    bool hasStubCodePatchOffset_ : 1;

    CodeLocationLabel rejoinLabel_;
    CodeOffsetJump nextStubOffset_;
    CodeOffsetJump rejoinOffset_;
    CodeOffsetLabel stubCodePatchOffset_;

  public:
    StubAttacher(CodeLocationLabel rejoinLabel)
      : hasNextStubOffset_(false),
        hasStubCodePatchOffset_(false),
        rejoinLabel_(rejoinLabel),
        nextStubOffset_(),
        rejoinOffset_(),
        stubCodePatchOffset_()
    { }

    
    
    
    
    
    static const ImmPtr STUB_ADDR;

    template <class T1, class T2>
    void branchNextStub(MacroAssembler &masm, Assembler::Condition cond, T1 op1, T2 op2) {
        JS_ASSERT(!hasNextStubOffset_);
        RepatchLabel nextStub;
        nextStubOffset_ = masm.branchPtrWithPatch(cond, op1, op2, &nextStub);
        hasNextStubOffset_ = true;
        masm.bind(&nextStub);
    }

    template <class T1, class T2>
    void branchNextStubOrLabel(MacroAssembler &masm, Assembler::Condition cond, T1 op1, T2 op2,
                               Label *label)
    {
        if (label != nullptr)
            masm.branchPtr(cond, op1, op2, label);
        else
            branchNextStub(masm, cond, op1, op2);
    }

    void jumpRejoin(MacroAssembler &masm) {
        RepatchLabel rejoin;
        rejoinOffset_ = masm.jumpWithPatch(&rejoin);
        masm.bind(&rejoin);
    }

    void jumpNextStub(MacroAssembler &masm) {
        JS_ASSERT(!hasNextStubOffset_);
        RepatchLabel nextStub;
        nextStubOffset_ = masm.jumpWithPatch(&nextStub);
        hasNextStubOffset_ = true;
        masm.bind(&nextStub);
    }

    void pushStubCodePointer(MacroAssembler &masm) {
        
        
        
        
        
        
        
        
        
        
        JS_ASSERT(!hasStubCodePatchOffset_);
        stubCodePatchOffset_ = masm.PushWithPatch(STUB_ADDR);
        hasStubCodePatchOffset_ = true;
    }

    void patchRejoinJump(MacroAssembler &masm, IonCode *code) {
        rejoinOffset_.fixup(&masm);
        CodeLocationJump rejoinJump(code, rejoinOffset_);
        PatchJump(rejoinJump, rejoinLabel_);
    }

    void patchStubCodePointer(MacroAssembler &masm, IonCode *code) {
        if (hasStubCodePatchOffset_) {
            stubCodePatchOffset_.fixup(&masm);
            Assembler::patchDataWithValueCheck(CodeLocationLabel(code, stubCodePatchOffset_),
                                               ImmPtr(code), STUB_ADDR);
        }
    }

    virtual void patchNextStubJump(MacroAssembler &masm, IonCode *code) = 0;
};

const ImmPtr IonCache::StubAttacher::STUB_ADDR = ImmPtr((void*)0xdeadc0de);

class RepatchIonCache::RepatchStubAppender : public IonCache::StubAttacher
{
    RepatchIonCache &cache_;

  public:
    RepatchStubAppender(RepatchIonCache &cache)
      : StubAttacher(cache.rejoinLabel()),
        cache_(cache)
    {
    }

    void patchNextStubJump(MacroAssembler &masm, IonCode *code) {
        
        
        PatchJump(cache_.lastJump_, CodeLocationLabel(code));

        
        
        if (hasNextStubOffset_) {
            nextStubOffset_.fixup(&masm);
            CodeLocationJump nextStubJump(code, nextStubOffset_);
            PatchJump(nextStubJump, cache_.fallbackLabel_);

            
            
            
            cache_.lastJump_ = nextStubJump;
        }
    }
};

void
RepatchIonCache::reset()
{
    IonCache::reset();
    PatchJump(initialJump_, fallbackLabel_);
    lastJump_ = initialJump_;
}

void
RepatchIonCache::emitInitialJump(MacroAssembler &masm, AddCacheState &addState)
{
    initialJump_ = masm.jumpWithPatch(&addState.repatchEntry);
    lastJump_ = initialJump_;
}

void
RepatchIonCache::bindInitialJump(MacroAssembler &masm, AddCacheState &addState)
{
    masm.bind(&addState.repatchEntry);
}

void
RepatchIonCache::updateBaseAddress(IonCode *code, MacroAssembler &masm)
{
    IonCache::updateBaseAddress(code, masm);
    initialJump_.repoint(code, &masm);
    lastJump_.repoint(code, &masm);
}

class DispatchIonCache::DispatchStubPrepender : public IonCache::StubAttacher
{
    DispatchIonCache &cache_;

  public:
    DispatchStubPrepender(DispatchIonCache &cache)
      : StubAttacher(cache.rejoinLabel_),
        cache_(cache)
    {
    }

    void patchNextStubJump(MacroAssembler &masm, IonCode *code) {
        JS_ASSERT(hasNextStubOffset_);

        
        
        nextStubOffset_.fixup(&masm);
        CodeLocationJump nextStubJump(code, nextStubOffset_);
        PatchJump(nextStubJump, CodeLocationLabel(cache_.firstStub_));

        
        
        
        cache_.firstStub_ = code->raw();
    }
};

void
DispatchIonCache::reset()
{
    IonCache::reset();
    firstStub_ = fallbackLabel_.raw();
}
void
DispatchIonCache::emitInitialJump(MacroAssembler &masm, AddCacheState &addState)
{
    Register scratch = addState.dispatchScratch;
    dispatchLabel_ = masm.movWithPatch(ImmPtr((void*)-1), scratch);
    masm.loadPtr(Address(scratch, 0), scratch);
    masm.jump(scratch);
    rejoinLabel_ = masm.labelForPatch();
}

void
DispatchIonCache::bindInitialJump(MacroAssembler &masm, AddCacheState &addState)
{
    
}

void
DispatchIonCache::updateBaseAddress(IonCode *code, MacroAssembler &masm)
{
    
    JS_ASSERT(uintptr_t(&firstStub_) % sizeof(uintptr_t) == 0);

    IonCache::updateBaseAddress(code, masm);
    dispatchLabel_.fixup(&masm);
    Assembler::patchDataWithValueCheck(CodeLocationLabel(code, dispatchLabel_),
                                       ImmPtr(&firstStub_),
                                       ImmPtr((void*)-1));
    firstStub_ = fallbackLabel_.raw();
    rejoinLabel_.repoint(code, &masm);
}

void
IonCache::attachStub(MacroAssembler &masm, StubAttacher &attacher, Handle<IonCode *> code)
{
    JS_ASSERT(canAttachStub());
    incrementStubCount();

    
    attacher.patchRejoinJump(masm, code);

    
    attacher.patchNextStubJump(masm, code);

    
    
    
    attacher.patchStubCodePointer(masm, code);
}

bool
IonCache::linkAndAttachStub(JSContext *cx, MacroAssembler &masm, StubAttacher &attacher,
                            IonScript *ion, const char *attachKind)
{
    Rooted<IonCode *> code(cx);
    LinkStatus status = linkCode(cx, masm, ion, code.address());
    if (status != LINK_GOOD)
        return status != LINK_ERROR;

    attachStub(masm, attacher, code);

    if (pc_) {
        IonSpew(IonSpew_InlineCaches, "Cache %p(%s:%d/%d) generated %s %s stub at %p",
                this, script_->filename(), script_->lineno(), script_->pcToOffset(pc_),
                attachKind, CacheName(kind()), code->raw());
    } else {
        IonSpew(IonSpew_InlineCaches, "Cache %p generated %s %s stub at %p",
                this, attachKind, CacheName(kind()), code->raw());
    }

#ifdef JS_ION_PERF
    writePerfSpewerIonCodeProfile(code, "IonCache");
#endif

    return true;
}

void
IonCache::updateBaseAddress(IonCode *code, MacroAssembler &masm)
{
    fallbackLabel_.repoint(code, &masm);
}

void
IonCache::initializeAddCacheState(LInstruction *ins, AddCacheState *addState)
{
}

static bool
IsCacheableDOMProxy(JSObject *obj)
{
    if (!obj->is<ProxyObject>())
        return false;

    BaseProxyHandler *handler = obj->as<ProxyObject>().handler();

    if (handler->family() != GetDOMProxyHandlerFamily())
        return false;

    if (obj->numFixedSlots() <= GetDOMProxyExpandoSlot())
        return false;

    return true;
}

static void
GeneratePrototypeGuards(JSContext *cx, IonScript *ion, MacroAssembler &masm, JSObject *obj,
                        JSObject *holder, Register objectReg, Register scratchReg,
                        Label *failures)
{
    




    JS_ASSERT(obj != holder);

    if (obj->hasUncacheableProto()) {
        
        
        masm.loadPtr(Address(objectReg, JSObject::offsetOfType()), scratchReg);
        Address proto(scratchReg, offsetof(types::TypeObject, proto));
        masm.branchNurseryPtr(Assembler::NotEqual, proto,
                              ImmMaybeNurseryPtr(obj->getProto()), failures);
    }

    JSObject *pobj = IsCacheableDOMProxy(obj)
                     ? obj->getTaggedProto().toObjectOrNull()
                     : obj->getProto();
    if (!pobj)
        return;
    while (pobj != holder) {
        if (pobj->hasUncacheableProto()) {
            JS_ASSERT(!pobj->hasSingletonType());
            masm.moveNurseryPtr(ImmMaybeNurseryPtr(pobj), scratchReg);
            Address objType(scratchReg, JSObject::offsetOfType());
            masm.branchPtr(Assembler::NotEqual, objType, ImmGCPtr(pobj->type()), failures);
        }
        pobj = pobj->getProto();
    }
}

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder)
{
    while (obj != holder) {
        




        JSObject *proto = obj->getProto();
        if (!proto || !proto->isNative())
            return false;
        obj = proto;
    }
    return true;
}

static bool
IsCacheableGetPropReadSlot(JSObject *obj, JSObject *holder, Shape *shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (!shape->hasSlot() || !shape->hasDefaultGetter())
        return false;

    return true;
}

static bool
IsCacheableNoProperty(JSObject *obj, JSObject *holder, Shape *shape, jsbytecode *pc,
                      const TypedOrValueRegister &output)
{
    if (shape)
        return false;

    JS_ASSERT(!holder);

    
    
    if (obj->getClass()->getProperty && obj->getClass()->getProperty != JS_PropertyStub)
        return false;

    
    
    
    JSObject *obj2 = obj;
    while (obj2) {
        if (!obj2->isNative())
            return false;
        obj2 = obj2->getProto();
    }

    
    
    
    
    
    if (!pc)
        return false;

#if JS_HAS_NO_SUCH_METHOD
    
    
    
    if (JSOp(*pc) == JSOP_CALLPROP ||
        JSOp(*pc) == JSOP_CALLELEM)
    {
        return false;
    }
#endif

    
    
    if (!output.hasValue())
        return false;

    return true;
}

static bool
IsOptimizableArgumentsObjectForLength(JSObject *obj)
{
    if (!obj->is<ArgumentsObject>())
        return false;

    if (obj->as<ArgumentsObject>().hasOverriddenLength())
        return false;

    return true;
}

static bool
IsOptimizableArgumentsObjectForGetElem(JSObject *obj, Value idval)
{
    if (!IsOptimizableArgumentsObjectForLength(obj))
        return false;

    ArgumentsObject &argsObj = obj->as<ArgumentsObject>();

    if (argsObj.isAnyElementDeleted())
        return false;

    if (!idval.isInt32())
        return false;

    int32_t idint = idval.toInt32();
    if (idint < 0 || static_cast<uint32_t>(idint) >= argsObj.initialLength())
        return false;

    return true;
}

static bool
IsCacheableGetPropCallNative(JSObject *obj, JSObject *holder, Shape *shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (!shape->hasGetterValue() || !shape->getterValue().isObject())
        return false;

    return shape->getterValue().toObject().is<JSFunction>() &&
           shape->getterValue().toObject().as<JSFunction>().isNative();
}

static bool
IsCacheableGetPropCallPropertyOp(JSObject *obj, JSObject *holder, Shape *shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (shape->hasSlot() || shape->hasGetterValue() || shape->hasDefaultGetter())
        return false;

    return true;
}

static inline void
EmitLoadSlot(MacroAssembler &masm, JSObject *holder, Shape *shape, Register holderReg,
             TypedOrValueRegister output, Register scratchReg)
{
    JS_ASSERT(holder);
    if (holder->isFixedSlot(shape->slot())) {
        Address addr(holderReg, JSObject::getFixedSlotOffset(shape->slot()));
        masm.loadTypedOrValue(addr, output);
    } else {
        masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), scratchReg);

        Address addr(scratchReg, holder->dynamicSlotIndex(shape->slot()) * sizeof(Value));
        masm.loadTypedOrValue(addr, output);
    }
}

static void
GenerateDOMProxyChecks(JSContext *cx, MacroAssembler &masm, JSObject *obj,
                       PropertyName *name, Register object, Label *stubFailure,
                       bool skipExpandoCheck = false)
{
    JS_ASSERT(IsCacheableDOMProxy(obj));

    
    
    
    
    Address handlerAddr(object, ProxyObject::offsetOfHandler());
    Address expandoSlotAddr(object, JSObject::getFixedSlotOffset(GetDOMProxyExpandoSlot()));

    
    masm.branchPrivatePtr(Assembler::NotEqual, handlerAddr,
                          ImmPtr(obj->as<ProxyObject>().handler()), stubFailure);

    if (skipExpandoCheck)
        return;

    
    
    RegisterSet domProxyRegSet(RegisterSet::All());
    domProxyRegSet.take(AnyRegister(object));
    ValueOperand tempVal = domProxyRegSet.takeValueOperand();
    masm.pushValue(tempVal);

    Label failDOMProxyCheck;
    Label domProxyOk;

    Value expandoVal = obj->getFixedSlot(GetDOMProxyExpandoSlot());
    masm.loadValue(expandoSlotAddr, tempVal);

    if (!expandoVal.isObject() && !expandoVal.isUndefined()) {
        masm.branchTestValue(Assembler::NotEqual, tempVal, expandoVal, &failDOMProxyCheck);

        ExpandoAndGeneration *expandoAndGeneration = (ExpandoAndGeneration*)expandoVal.toPrivate();
        masm.movePtr(ImmPtr(expandoAndGeneration), tempVal.scratchReg());

        masm.branch32(Assembler::NotEqual,
                      Address(tempVal.scratchReg(),
                              ExpandoAndGeneration::offsetOfGeneration()),
                      Imm32(expandoAndGeneration->generation),
                      &failDOMProxyCheck);

        expandoVal = expandoAndGeneration->expando;
        masm.loadValue(Address(tempVal.scratchReg(),
                               ExpandoAndGeneration::offsetOfExpando()),
                       tempVal);
    }

    
    
    masm.branchTestUndefined(Assembler::Equal, tempVal, &domProxyOk);

    if (expandoVal.isObject()) {
        JS_ASSERT(!expandoVal.toObject().nativeContains(cx, name));

        
        
        masm.branchTestObject(Assembler::NotEqual, tempVal, &failDOMProxyCheck);
        masm.extractObject(tempVal, tempVal.scratchReg());
        masm.branchPtr(Assembler::Equal,
                       Address(tempVal.scratchReg(), JSObject::offsetOfShape()),
                       ImmGCPtr(expandoVal.toObject().lastProperty()),
                       &domProxyOk);
    }

    
    masm.bind(&failDOMProxyCheck);
    masm.popValue(tempVal);
    masm.jump(stubFailure);

    
    masm.bind(&domProxyOk);
    masm.popValue(tempVal);
}

static void
GenerateReadSlot(JSContext *cx, IonScript *ion, MacroAssembler &masm,
                 IonCache::StubAttacher &attacher, JSObject *obj, JSObject *holder,
                 Shape *shape, Register object, TypedOrValueRegister output,
                 Label *failures = nullptr)
{
    JS_ASSERT(obj->isNative());
    
    
    
    bool multipleFailureJumps = (obj != holder) || (failures != nullptr && failures->used());

    
    
    Label failures_;
    if (multipleFailureJumps && !failures)
        failures = &failures_;

    
    attacher.branchNextStubOrLabel(masm, Assembler::NotEqual,
                                   Address(object, JSObject::offsetOfShape()),
                                   ImmGCPtr(obj->lastProperty()),
                                   failures);

    
    
    
    bool restoreScratch = false;
    Register scratchReg = Register::FromCode(0); 

    if (obj != holder || !holder->isFixedSlot(shape->slot())) {
        if (output.hasValue()) {
            scratchReg = output.valueReg().scratchReg();
        } else if (output.type() == MIRType_Double) {
            scratchReg = object;
            masm.push(scratchReg);
            restoreScratch = true;
        } else {
            scratchReg = output.typedReg().gpr();
        }
    }

    
    if (!multipleFailureJumps) {
        EmitLoadSlot(masm, holder, shape, object, output, scratchReg);
        if (restoreScratch)
            masm.pop(scratchReg);
        attacher.jumpRejoin(masm);
        return;
    }

    
    Label prototypeFailures;
    Register holderReg;
    if (obj != holder) {
        
        GeneratePrototypeGuards(cx, ion, masm, obj, holder, object, scratchReg,
                                &prototypeFailures);

        if (holder) {
            
            holderReg = scratchReg;
            masm.moveNurseryPtr(ImmMaybeNurseryPtr(holder), holderReg);
            masm.branchPtr(Assembler::NotEqual,
                           Address(holderReg, JSObject::offsetOfShape()),
                           ImmGCPtr(holder->lastProperty()),
                           &prototypeFailures);
        } else {
            
            
            JSObject *proto = obj->getTaggedProto().toObjectOrNull();
            Register lastReg = object;
            JS_ASSERT(scratchReg != object);
            while (proto) {
                Address addrType(lastReg, JSObject::offsetOfType());
                masm.loadPtr(addrType, scratchReg);
                Address addrProto(scratchReg, offsetof(types::TypeObject, proto));
                masm.loadPtr(addrProto, scratchReg);
                Address addrShape(scratchReg, JSObject::offsetOfShape());

                
                masm.branchPtr(Assembler::NotEqual,
                               Address(scratchReg, JSObject::offsetOfShape()),
                               ImmGCPtr(proto->lastProperty()),
                               &prototypeFailures);

                proto = proto->getProto();
                lastReg = scratchReg;
            }

            holderReg = InvalidReg;
        }
    } else {
        holderReg = object;
    }

    
    if (holder)
        EmitLoadSlot(masm, holder, shape, holderReg, output, scratchReg);
    else
        masm.moveValue(UndefinedValue(), output.valueReg());

    
    if (restoreScratch)
        masm.pop(scratchReg);

    attacher.jumpRejoin(masm);

    masm.bind(&prototypeFailures);
    if (restoreScratch)
        masm.pop(scratchReg);
    masm.bind(failures);

    attacher.jumpNextStub(masm);

}

static bool
EmitGetterCall(JSContext *cx, MacroAssembler &masm,
               IonCache::StubAttacher &attacher, JSObject *obj,
               JSObject *holder, HandleShape shape,
               RegisterSet liveRegs, Register object,
               Register scratchReg, TypedOrValueRegister output,
               void *returnAddr)
{
    JS_ASSERT(output.hasValue());
    
    MacroAssembler::AfterICSaveLive aic = masm.icSaveLive(liveRegs);

    
    
    RegisterSet regSet(RegisterSet::All());
    regSet.take(AnyRegister(object));

    
    
    
    
    scratchReg               = regSet.takeGeneral();
    Register argJSContextReg = regSet.takeGeneral();
    Register argUintNReg     = regSet.takeGeneral();
    Register argVpReg        = regSet.takeGeneral();

    
    bool callNative = IsCacheableGetPropCallNative(obj, holder, shape);
    JS_ASSERT_IF(!callNative, IsCacheableGetPropCallPropertyOp(obj, holder, shape));

    
    DebugOnly<uint32_t> initialStack = masm.framePushed();

    if (callNative) {
        JS_ASSERT(shape->hasGetterValue() && shape->getterValue().isObject() &&
                  shape->getterValue().toObject().is<JSFunction>());
        JSFunction *target = &shape->getterValue().toObject().as<JSFunction>();

        JS_ASSERT(target);
        JS_ASSERT(target->isNative());

        
        
        
        

        
        
        masm.Push(TypedOrValueRegister(MIRType_Object, AnyRegister(object)));
        
        masm.Push(ObjectValue(*target));

        
        masm.loadJSContext(argJSContextReg);
        masm.move32(Imm32(0), argUintNReg);
        masm.movePtr(StackPointer, argVpReg);

        
        masm.Push(argUintNReg);
        attacher.pushStubCodePointer(masm);

        if (!masm.icBuildOOLFakeExitFrame(returnAddr, aic))
            return false;
        masm.enterFakeExitFrame(ION_FRAME_OOL_NATIVE);

        
        masm.setupUnalignedABICall(3, scratchReg);
        masm.passABIArg(argJSContextReg);
        masm.passABIArg(argUintNReg);
        masm.passABIArg(argVpReg);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));

        
        masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

        
        masm.loadValue(
            Address(StackPointer, IonOOLNativeExitFrameLayout::offsetOfResult()),
            JSReturnOperand);
    } else {
        Register argObjReg       = argUintNReg;
        Register argIdReg        = regSet.takeGeneral();

        PropertyOp target = shape->getterOp();
        JS_ASSERT(target);

        
        attacher.pushStubCodePointer(masm);

        

        
        masm.Push(UndefinedValue());
        masm.movePtr(StackPointer, argVpReg);

        
        RootedId propId(cx);
        if (!shape->getUserId(cx, &propId))
            return false;
        masm.Push(propId, scratchReg);
        masm.movePtr(StackPointer, argIdReg);

        masm.Push(object);
        masm.movePtr(StackPointer, argObjReg);

        masm.loadJSContext(argJSContextReg);

        if (!masm.icBuildOOLFakeExitFrame(returnAddr, aic))
            return false;
        masm.enterFakeExitFrame(ION_FRAME_OOL_PROPERTY_OP);

        
        masm.setupUnalignedABICall(4, scratchReg);
        masm.passABIArg(argJSContextReg);
        masm.passABIArg(argObjReg);
        masm.passABIArg(argIdReg);
        masm.passABIArg(argVpReg);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target));

        
        masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

        
        masm.loadValue(
            Address(StackPointer, IonOOLPropertyOpExitFrameLayout::offsetOfResult()),
            JSReturnOperand);
    }

    
    
    masm.storeCallResultValue(output);

    
    

    
    if (callNative)
        masm.adjustStack(IonOOLNativeExitFrameLayout::Size(0));
    else
        masm.adjustStack(IonOOLPropertyOpExitFrameLayout::Size());
    JS_ASSERT(masm.framePushed() == initialStack);

    
    masm.PopRegsInMask(liveRegs);

    return true;
}

static bool
GenerateCallGetter(JSContext *cx, IonScript *ion, MacroAssembler &masm,
                   IonCache::StubAttacher &attacher, JSObject *obj, PropertyName *name,
                   JSObject *holder, HandleShape shape, RegisterSet &liveRegs, Register object,
                   TypedOrValueRegister output, void *returnAddr, Label *failures = nullptr)
{
    JS_ASSERT(obj->isNative());
    JS_ASSERT(output.hasValue());

    
    Label stubFailure;
    failures = failures ? failures : &stubFailure;

    
    masm.branchPtr(Assembler::NotEqual, Address(object, JSObject::offsetOfShape()),
                   ImmGCPtr(obj->lastProperty()), failures);

    Register scratchReg = output.valueReg().scratchReg();

    
    if (obj != holder)
        GeneratePrototypeGuards(cx, ion, masm, obj, holder, object, scratchReg, failures);

    
    Register holderReg = scratchReg;
    masm.moveNurseryPtr(ImmMaybeNurseryPtr(holder), holderReg);
    masm.branchPtr(Assembler::NotEqual,
                   Address(holderReg, JSObject::offsetOfShape()),
                   ImmGCPtr(holder->lastProperty()),
                   failures);

    
    if (!EmitGetterCall(cx, masm, attacher, obj, holder, shape, liveRegs, object,
                        scratchReg, output, returnAddr))
        return false;

    
    attacher.jumpRejoin(masm);

    
    masm.bind(failures);
    attacher.jumpNextStub(masm);

    return true;
}

static bool
GenerateArrayLength(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                    JSObject *obj, Register object, TypedOrValueRegister output)
{
    JS_ASSERT(obj->is<ArrayObject>());

    Label failures;

    
    RootedShape shape(cx, obj->lastProperty());
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object, shape, &failures);

    
    Register outReg;
    if (output.hasValue()) {
        outReg = output.valueReg().scratchReg();
    } else {
        JS_ASSERT(output.type() == MIRType_Int32);
        outReg = output.typedReg().gpr();
    }

    masm.loadPtr(Address(object, JSObject::offsetOfElements()), outReg);
    masm.load32(Address(outReg, ObjectElements::offsetOfLength()), outReg);

    
    JS_ASSERT(object != outReg);
    masm.branchTest32(Assembler::Signed, outReg, outReg, &failures);

    if (output.hasValue())
        masm.tagValue(JSVAL_TYPE_INT32, outReg, output.valueReg());

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    return true;
}

static void
GenerateTypedArrayLength(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                         JSObject *obj, Register object, TypedOrValueRegister output)
{
    JS_ASSERT(obj->is<TypedArrayObject>());

    Label failures;

    Register tmpReg;
    if (output.hasValue()) {
        tmpReg = output.valueReg().scratchReg();
    } else {
        JS_ASSERT(output.type() == MIRType_Int32);
        tmpReg = output.typedReg().gpr();
    }
    JS_ASSERT(object != tmpReg);

    
    masm.loadObjClass(object, tmpReg);
    masm.branchPtr(Assembler::Below, tmpReg, ImmPtr(&TypedArrayObject::classes[0]),
                   &failures);
    masm.branchPtr(Assembler::AboveOrEqual, tmpReg,
                   ImmPtr(&TypedArrayObject::classes[ScalarTypeRepresentation::TYPE_MAX]),
                   &failures);

    
    masm.loadTypedOrValue(Address(object, TypedArrayObject::lengthOffset()), output);

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failures);
    attacher.jumpNextStub(masm);
}

static bool
IsCacheableArrayLength(JSContext *cx, HandleObject obj, HandlePropertyName name,
                       TypedOrValueRegister output)
{
    if (!obj->is<ArrayObject>())
        return false;

    if (output.type() != MIRType_Value && output.type() != MIRType_Int32) {
        
        
        return false;
    }

    return true;
}

template <class GetPropCache>
static GetPropertyIC::NativeGetPropCacheability
CanAttachNativeGetProp(typename GetPropCache::Context cx, const GetPropCache &cache,
                       HandleObject obj, HandlePropertyName name,
                       MutableHandleObject holder, MutableHandleShape shape,
                       bool skipArrayLen = false)
{
    if (!obj || !obj->isNative())
        return GetPropertyIC::CanAttachNone;

    
    
    
    
    if (!LookupPropertyPure(obj, NameToId(name), holder.address(), shape.address()))
        return GetPropertyIC::CanAttachNone;

    RootedScript script(cx);
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);
    if (IsCacheableGetPropReadSlot(obj, holder, shape) ||
        IsCacheableNoProperty(obj, holder, shape, pc, cache.output()))
    {
        return GetPropertyIC::CanAttachReadSlot;
    }

    
    
    
    if (!skipArrayLen && cx->names().length == name && cache.allowArrayLength(cx, obj) &&
        IsCacheableArrayLength(cx, obj, name, cache.output()))
    {
        
        
        
        
        return GetPropertyIC::CanAttachArrayLength;
    }

    
    
    
    if (cache.allowGetters() &&
        (IsCacheableGetPropCallNative(obj, holder, shape) ||
         IsCacheableGetPropCallPropertyOp(obj, holder, shape)))
    {
        
        
        return GetPropertyIC::CanAttachCallGetter;
    }

    return GetPropertyIC::CanAttachNone;
}

bool
GetPropertyIC::allowArrayLength(Context cx, HandleObject obj) const
{
    if (!idempotent())
        return true;

    uint32_t locationIndex, numLocations;
    getLocationInfo(&locationIndex, &numLocations);

    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    CacheLocation *locs = ion->getCacheLocs(locationIndex);
    for (size_t i = 0; i < numLocations; i++) {
        CacheLocation &curLoc = locs[i];
        types::StackTypeSet *bcTypes =
            types::TypeScript::BytecodeTypes(curLoc.script, curLoc.pc);

        if (!bcTypes->hasType(types::Type::Int32Type()))
            return false;
    }

    return true;
}

bool
GetPropertyIC::tryAttachNative(JSContext *cx, IonScript *ion, HandleObject obj,
                               HandlePropertyName name, void *returnAddr, bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);

    RootedShape shape(cx);
    RootedObject holder(cx);

    NativeGetPropCacheability type =
        CanAttachNativeGetProp(cx, *this, obj, name, &holder, &shape);
    if (type == CanAttachNone)
        return true;

    *emitted = true;

    MacroAssembler masm(cx);
    SkipRoot skip(cx, &masm);

    RepatchStubAppender attacher(*this);
    const char *attachKind;

    switch (type) {
      case CanAttachReadSlot:
        GenerateReadSlot(cx, ion, masm, attacher, obj, holder,
                         shape, object(), output());
        attachKind = idempotent() ? "idempotent reading"
                                    : "non idempotent reading";
        break;
      case CanAttachCallGetter:
        masm.setFramePushed(ion->frameSize());
        if (!GenerateCallGetter(cx, ion, masm, attacher, obj, name, holder, shape,
                                liveRegs_, object(), output(), returnAddr))
        {
            return false;
        }
        attachKind = "getter call";
        break;
      case CanAttachArrayLength:
        if (!GenerateArrayLength(cx, masm, attacher, obj, object(), output()))
            return false;

        attachKind = "array length";
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Bad NativeGetPropCacheability");
    }
    return linkAndAttachStub(cx, masm, attacher, ion, attachKind);
}

bool
GetPropertyIC::tryAttachTypedArrayLength(JSContext *cx, IonScript *ion, HandleObject obj,
                                         HandlePropertyName name, bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);

    if (!obj->is<TypedArrayObject>())
        return true;

    if (cx->names().length != name)
        return true;

    if (hasTypedArrayLengthStub())
        return true;

    if (output().type() != MIRType_Value && output().type() != MIRType_Int32) {
        
        
        return true;
    }

    if (idempotent())
        return true;

    *emitted = true;

    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    GenerateTypedArrayLength(cx, masm, attacher, obj, object(), output());

    JS_ASSERT(!hasTypedArrayLengthStub_);
    hasTypedArrayLengthStub_ = true;
    return linkAndAttachStub(cx, masm, attacher, ion, "typed array length");
}


static bool
EmitCallProxyGet(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                 PropertyName *name, RegisterSet liveRegs, Register object,
                 TypedOrValueRegister output, jsbytecode *pc, void *returnAddr)
{
    JS_ASSERT(output.hasValue());
    
    MacroAssembler::AfterICSaveLive aic = masm.icSaveLive(liveRegs);

    
    
    RegisterSet regSet(RegisterSet::All());
    regSet.take(AnyRegister(object));

    
    
    Register argJSContextReg = regSet.takeGeneral();
    Register argProxyReg     = regSet.takeGeneral();
    Register argIdReg        = regSet.takeGeneral();
    Register argVpReg        = regSet.takeGeneral();

    Register scratch         = regSet.takeGeneral();

    DebugOnly<uint32_t> initialStack = masm.framePushed();
    void *getFunction = JSOp(*pc) == JSOP_CALLPROP                      ?
                            JS_FUNC_TO_DATA_PTR(void *, Proxy::callProp) :
                            JS_FUNC_TO_DATA_PTR(void *, Proxy::get);

    
    attacher.pushStubCodePointer(masm);

    
    masm.Push(UndefinedValue());
    masm.movePtr(StackPointer, argVpReg);

    RootedId propId(cx, AtomToId(name));
    masm.Push(propId, scratch);
    masm.movePtr(StackPointer, argIdReg);

    
    
    masm.Push(object);
    masm.Push(object);
    masm.movePtr(StackPointer, argProxyReg);

    masm.loadJSContext(argJSContextReg);

    if (!masm.icBuildOOLFakeExitFrame(returnAddr, aic))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_OOL_PROXY);

    
    masm.setupUnalignedABICall(5, scratch);
    masm.passABIArg(argJSContextReg);
    masm.passABIArg(argProxyReg);
    masm.passABIArg(argProxyReg);
    masm.passABIArg(argIdReg);
    masm.passABIArg(argVpReg);
    masm.callWithABI(getFunction);

    
    masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

    
    masm.loadValue(
        Address(StackPointer, IonOOLProxyExitFrameLayout::offsetOfResult()),
        JSReturnOperand);

    masm.storeCallResultValue(output);

    
    

    
    masm.adjustStack(IonOOLProxyExitFrameLayout::Size());
    JS_ASSERT(masm.framePushed() == initialStack);

    
    masm.PopRegsInMask(liveRegs);

    return true;
}

bool
GetPropertyIC::tryAttachDOMProxyShadowed(JSContext *cx, IonScript *ion,
                                         HandleObject obj, void *returnAddr,
                                         bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);
    JS_ASSERT(IsCacheableDOMProxy(obj));

    if (idempotent() || !output().hasValue())
        return true;

    *emitted = true;

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    masm.setFramePushed(ion->frameSize());

    
    attacher.branchNextStubOrLabel(masm, Assembler::NotEqual,
                                   Address(object(), JSObject::offsetOfShape()),
                                   ImmGCPtr(obj->lastProperty()),
                                   &failures);

    
    GenerateDOMProxyChecks(cx, masm, obj, name(), object(), &failures,
                           true);

    if (!EmitCallProxyGet(cx, masm, attacher, name(), liveRegs_, object(), output(),
                          pc(), returnAddr))
    {
        return false;
    }

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "list base shadowed get");
}

bool
GetPropertyIC::tryAttachDOMProxyUnshadowed(JSContext *cx, IonScript *ion, HandleObject obj,
                                           HandlePropertyName name, bool resetNeeded,
                                           void *returnAddr, bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);
    JS_ASSERT(IsCacheableDOMProxy(obj));

    RootedObject checkObj(cx, obj->getTaggedProto().toObjectOrNull());
    RootedObject holder(cx);
    RootedShape shape(cx);

    NativeGetPropCacheability canCache =
        CanAttachNativeGetProp(cx, *this, checkObj, name, &holder, &shape,
                               true);
    JS_ASSERT(canCache != CanAttachArrayLength);

    if (canCache == CanAttachNone)
        return true;

    
    if (!holder && (idempotent() || !output().hasValue()))
        return true;

    *emitted = true;

    if (resetNeeded) {
        
        
        
        
        
        
        reset();
    }

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    masm.setFramePushed(ion->frameSize());

    
    attacher.branchNextStubOrLabel(masm, Assembler::NotEqual,
                                   Address(object(), JSObject::offsetOfShape()),
                                   ImmGCPtr(obj->lastProperty()),
                                   &failures);

    
    GenerateDOMProxyChecks(cx, masm, obj, name, object(), &failures);

    if (holder) {
        
        
        Register scratchReg = output().valueReg().scratchReg();
        GeneratePrototypeGuards(cx, ion, masm, obj, holder, object(), scratchReg, &failures);

        
        Register holderReg = scratchReg;

        
        masm.moveNurseryPtr(ImmMaybeNurseryPtr(holder), holderReg);
        masm.branchPtr(Assembler::NotEqual,
                    Address(holderReg, JSObject::offsetOfShape()),
                    ImmGCPtr(holder->lastProperty()),
                    &failures);

        if (canCache == CanAttachReadSlot) {
            EmitLoadSlot(masm, holder, shape, holderReg, output(), scratchReg);
        } else {
            
            
            
            JS_ASSERT(canCache == CanAttachCallGetter);
            JS_ASSERT(!idempotent());
            if (!EmitGetterCall(cx, masm, attacher, checkObj, holder, shape, liveRegs_,
                                object(), scratchReg, output(), returnAddr))
            {
                return false;
            }
        }
    } else {
        
        
        JS_ASSERT(!idempotent());
        if (!EmitCallProxyGet(cx, masm, attacher, name, liveRegs_, object(), output(),
                              pc(), returnAddr))
        {
            return false;
        }
    }

    attacher.jumpRejoin(masm);
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "unshadowed proxy get");
}

bool
GetPropertyIC::tryAttachProxy(JSContext *cx, IonScript *ion, HandleObject obj,
                              HandlePropertyName name, void *returnAddr,
                              bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);

    if (!obj->is<ProxyObject>())
        return true;

    
    if (IsCacheableDOMProxy(obj)) {
        RootedId id(cx, NameToId(name));
        DOMProxyShadowsResult shadows = GetDOMProxyShadowsCheck()(cx, obj, id);
        if (shadows == ShadowCheckFailed)
            return false;
        if (shadows == Shadows)
            return tryAttachDOMProxyShadowed(cx, ion, obj, returnAddr, emitted);

        return tryAttachDOMProxyUnshadowed(cx, ion, obj, name, shadows == DoesntShadowUnique,
                                           returnAddr, emitted);
    }

    return tryAttachGenericProxy(cx, ion, obj, name, returnAddr, emitted);
}

static void
GenerateProxyClassGuards(MacroAssembler &masm, Register object, Register scratchReg,
                         Label *failures)
{
    Label success;
    
    
    
    masm.branchTestObjClass(Assembler::Equal, object, scratchReg,
                            CallableProxyClassPtr, &success);
    masm.branchTestObjClass(Assembler::Equal, object, scratchReg,
                            UncallableProxyClassPtr, &success);
    masm.branchTestObjClass(Assembler::NotEqual, object, scratchReg,
                            OuterWindowProxyClassPtr, failures);
    masm.bind(&success);
}

bool
GetPropertyIC::tryAttachGenericProxy(JSContext *cx, IonScript *ion, HandleObject obj,
                                     HandlePropertyName name, void *returnAddr,
                                     bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);
    JS_ASSERT(obj->is<ProxyObject>());

    if (hasGenericProxyStub())
        return true;

    if (idempotent() || !output().hasValue())
        return true;

    *emitted = true;

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    Register scratchReg = output().valueReg().scratchReg();

    masm.setFramePushed(ion->frameSize());

    GenerateProxyClassGuards(masm, object(), scratchReg, &failures);

    
    
    masm.branchTestProxyHandlerFamily(Assembler::Equal, object(), scratchReg,
                                      GetDOMProxyHandlerFamily(), &failures);

    if (!EmitCallProxyGet(cx, masm, attacher, name, liveRegs_, object(), output(),
                          pc(), returnAddr))
    {
        return false;
    }

    attacher.jumpRejoin(masm);

    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    JS_ASSERT(!hasGenericProxyStub_);
    hasGenericProxyStub_ = true;

    return linkAndAttachStub(cx, masm, attacher, ion, "Generic Proxy get");
}

bool
GetPropertyIC::tryAttachArgumentsLength(JSContext *cx, IonScript *ion, HandleObject obj,
                                        HandlePropertyName name, bool *emitted)
{
    JS_ASSERT(canAttachStub());
    JS_ASSERT(!*emitted);

    if (name != cx->names().length)
        return true;
    if (!IsOptimizableArgumentsObjectForLength(obj))
        return true;

    MIRType outputType = output().type();
    if (!(outputType == MIRType_Value || outputType == MIRType_Int32))
        return true;

    if (hasArgumentsLengthStub(obj->is<StrictArgumentsObject>()))
        return true;

    *emitted = true;

    JS_ASSERT(!idempotent());

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    Register tmpReg;
    if (output().hasValue()) {
        tmpReg = output().valueReg().scratchReg();
    } else {
        JS_ASSERT(output().type() == MIRType_Int32);
        tmpReg = output().typedReg().gpr();
    }
    JS_ASSERT(object() != tmpReg);

    const Class *clasp = obj->is<StrictArgumentsObject>() ? &StrictArgumentsObject::class_
                                                          : &NormalArgumentsObject::class_;

    masm.branchTestObjClass(Assembler::NotEqual, object(), tmpReg, clasp, &failures);

    
    masm.unboxInt32(Address(object(), ArgumentsObject::getInitialLengthSlotOffset()), tmpReg);
    masm.branchTest32(Assembler::NonZero, tmpReg, Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT),
                      &failures);

    masm.rshiftPtr(Imm32(ArgumentsObject::PACKED_BITS_COUNT), tmpReg);

    
    if (output().hasValue())
        masm.tagValue(JSVAL_TYPE_INT32, tmpReg, output().valueReg());

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    if (obj->is<StrictArgumentsObject>()) {
        JS_ASSERT(!hasStrictArgumentsLengthStub_);
        hasStrictArgumentsLengthStub_ = true;
        return linkAndAttachStub(cx, masm, attacher, ion, "ArgsObj length (strict)");
    }

    JS_ASSERT(!hasNormalArgumentsLengthStub_);
    hasNormalArgumentsLengthStub_ = true;
    return linkAndAttachStub(cx, masm, attacher, ion, "ArgsObj length (normal)");
}

bool
GetPropertyIC::tryAttachStub(JSContext *cx, IonScript *ion, HandleObject obj,
                             HandlePropertyName name, void *returnAddr, bool *emitted)
{
    JS_ASSERT(!*emitted);

    if (!canAttachStub())
        return true;

    if (!*emitted && !tryAttachArgumentsLength(cx, ion, obj, name, emitted))
        return false;

    if (!*emitted && !tryAttachProxy(cx, ion, obj, name, returnAddr, emitted))
        return false;

    if (!*emitted && !tryAttachNative(cx, ion, obj, name, returnAddr, emitted))
        return false;

    if (!*emitted && !tryAttachTypedArrayLength(cx, ion, obj, name, emitted))
        return false;

    return true;
}

 bool
GetPropertyIC::update(JSContext *cx, size_t cacheIndex,
                      HandleObject obj, MutableHandleValue vp)
{
    void *returnAddr;
    RootedScript topScript(cx, GetTopIonJSScript(cx, &returnAddr));
    IonScript *ion = topScript->ionScript();

    GetPropertyIC &cache = ion->getCache(cacheIndex).toGetProperty();
    RootedPropertyName name(cx, cache.name());

    AutoFlushCache afc ("GetPropertyCache", cx->runtime()->jitRuntime());

    
    AutoDetectInvalidation adi(cx, vp.address(), ion);

    
    if (cache.idempotent())
        adi.disable();

    
    
    
    bool emitted = false;
    if (!cache.tryAttachStub(cx, ion, obj, name, returnAddr, &emitted))
        return false;

    if (cache.idempotent() && !emitted) {
        
        
        
        
        
        
        IonSpew(IonSpew_InlineCaches, "Invalidating from idempotent cache %s:%d",
                topScript->filename(), topScript->lineno());

        topScript->setInvalidatedIdempotentCache();

        
        if (!topScript->hasIonScript())
            return true;

        return Invalidate(cx, topScript);
    }

    RootedId id(cx, NameToId(name));
    if (!JSObject::getGeneric(cx, obj, obj, id, vp))
        return false;

    if (!cache.idempotent()) {
        RootedScript script(cx);
        jsbytecode *pc;
        cache.getScriptedLocation(&script, &pc);

        
        

#if JS_HAS_NO_SUCH_METHOD
        
        if (JSOp(*pc) == JSOP_CALLPROP && JS_UNLIKELY(vp.isUndefined())) {
            if (!OnUnknownMethod(cx, obj, IdToValue(id), vp))
                return false;
        }
#endif

        
        types::TypeScript::Monitor(cx, script, pc, vp);
    }

    return true;
}

void
GetPropertyIC::reset()
{
    RepatchIonCache::reset();
    hasTypedArrayLengthStub_ = false;
    hasStrictArgumentsLengthStub_ = false;
    hasNormalArgumentsLengthStub_ = false;
    hasGenericProxyStub_ = false;
}

bool
ParallelIonCache::initStubbedShapes(JSContext *cx)
{
    JS_ASSERT(isAllocated());
    if (!stubbedShapes_) {
        stubbedShapes_ = cx->new_<ShapeSet>(cx);
        return stubbedShapes_ && stubbedShapes_->init();
    }
    return true;
}

bool
ParallelIonCache::hasOrAddStubbedShape(LockedJSContext &cx, Shape *shape, bool *alreadyStubbed)
{
    
    
    if (!initStubbedShapes(cx))
        return false;
    ShapeSet::AddPtr p = stubbedShapes_->lookupForAdd(shape);
    if ((*alreadyStubbed = !!p))
        return true;
    return stubbedShapes_->add(p, shape);
}

void
ParallelIonCache::reset()
{
    DispatchIonCache::reset();
    if (stubbedShapes_)
        stubbedShapes_->clear();
}

void
ParallelIonCache::destroy()
{
    DispatchIonCache::destroy();
    js_delete(stubbedShapes_);
}

void
GetPropertyParIC::reset()
{
    ParallelIonCache::reset();
    hasTypedArrayLengthStub_ = false;
}

bool
GetPropertyParIC::attachReadSlot(LockedJSContext &cx, IonScript *ion, JSObject *obj,
                                 JSObject *holder, Shape *shape)
{
    
    DispatchStubPrepender attacher(*this);
    MacroAssembler masm(cx);
    GenerateReadSlot(cx, ion, masm, attacher, obj, holder, shape, object(), output());

    return linkAndAttachStub(cx, masm, attacher, ion, "parallel reading");
}

bool
GetPropertyParIC::attachArrayLength(LockedJSContext &cx, IonScript *ion, JSObject *obj)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    if (!GenerateArrayLength(cx, masm, attacher, obj, object(), output()))
        return false;

    return linkAndAttachStub(cx, masm, attacher, ion, "parallel array length");
}

bool
GetPropertyParIC::attachTypedArrayLength(LockedJSContext &cx, IonScript *ion, JSObject *obj)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    GenerateTypedArrayLength(cx, masm, attacher, obj, object(), output());

    JS_ASSERT(!hasTypedArrayLengthStub_);
    hasTypedArrayLengthStub_ = true;
    return linkAndAttachStub(cx, masm, attacher, ion, "parallel typed array length");
}

bool
GetPropertyParIC::update(ForkJoinSlice *slice, size_t cacheIndex,
                         HandleObject obj, MutableHandleValue vp)
{
    AutoFlushCache afc("GetPropertyParCache", slice->runtime()->jitRuntime());

    IonScript *ion = GetTopIonJSScript(slice)->parallelIonScript();
    GetPropertyParIC &cache = ion->getCache(cacheIndex).toGetPropertyPar();

    
    
    if (!GetPropertyPure(slice, obj, NameToId(cache.name()), vp.address()))
        return false;

    
    if (!cache.canAttachStub())
        return true;

    {
        
        
        
        LockedJSContext cx(slice);

        if (cache.canAttachStub()) {
            bool alreadyStubbed;
            if (!cache.hasOrAddStubbedShape(cx, obj->lastProperty(), &alreadyStubbed))
                return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
            if (alreadyStubbed)
                return true;

            
            bool attachedStub = false;

            {
                RootedShape shape(cx);
                RootedObject holder(cx);
                RootedPropertyName name(cx, cache.name());

                GetPropertyIC::NativeGetPropCacheability canCache =
                    CanAttachNativeGetProp(cx, cache, obj, name, &holder, &shape);

                if (canCache == GetPropertyIC::CanAttachReadSlot) {
                    if (!cache.attachReadSlot(cx, ion, obj, holder, shape))
                        return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                    attachedStub = true;
                }

                if (!attachedStub && canCache == GetPropertyIC::CanAttachArrayLength) {
                    if (!cache.attachArrayLength(cx, ion, obj))
                        return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                    attachedStub = true;
                }
            }

            if (!attachedStub && !cache.hasTypedArrayLengthStub() &&
                obj->is<TypedArrayObject>() && slice->names().length == cache.name() &&
                (cache.output().type() == MIRType_Value || cache.output().type() == MIRType_Int32))
            {
                if (!cache.attachTypedArrayLength(cx, ion, obj))
                    return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                attachedStub = true;
            }
        }
    }

    return true;
}

void
IonCache::disable()
{
    reset();
    this->disabled_ = 1;
}

void
IonCache::reset()
{
    this->stubCount_ = 0;
}

void
IonCache::destroy()
{
}

static void
GenerateSetSlot(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                JSObject *obj, Shape *shape, Register object, ConstantOrRegister value,
                bool needsTypeBarrier, bool checkTypeset)
{
    JS_ASSERT(obj->isNative());

    Label failures, barrierFailure;
    masm.branchPtr(Assembler::NotEqual,
                   Address(object, JSObject::offsetOfShape()),
                   ImmGCPtr(obj->lastProperty()), &failures);

    
    
    if (needsTypeBarrier) {
        
        

        
        types::TypeObject *type = obj->type();
        masm.branchPtr(Assembler::NotEqual,
                       Address(object, JSObject::offsetOfType()),
                       ImmGCPtr(type), &failures);

        if (checkTypeset) {
            TypedOrValueRegister valReg = value.reg();
            types::HeapTypeSet *propTypes = type->maybeGetProperty(shape->propid());
            JS_ASSERT(propTypes);
            JS_ASSERT(!propTypes->unknown());

            Register scratchReg = object;
            masm.push(scratchReg);

            masm.guardTypeSet(valReg, propTypes, scratchReg, &barrierFailure);
            masm.pop(object);
        }
    }

    if (obj->isFixedSlot(shape->slot())) {
        Address addr(object, JSObject::getFixedSlotOffset(shape->slot()));

        if (cx->zone()->needsBarrier())
            masm.callPreBarrier(addr, MIRType_Value);

        masm.storeConstantOrRegister(value, addr);
    } else {
        Register slotsReg = object;
        masm.loadPtr(Address(object, JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(shape->slot()) * sizeof(Value));

        if (cx->zone()->needsBarrier())
            masm.callPreBarrier(addr, MIRType_Value);

        masm.storeConstantOrRegister(value, addr);
    }

    attacher.jumpRejoin(masm);

    if (barrierFailure.used()) {
        masm.bind(&barrierFailure);
        masm.pop(object);
    }

    masm.bind(&failures);
    attacher.jumpNextStub(masm);
}

bool
SetPropertyIC::attachSetSlot(JSContext *cx, IonScript *ion, HandleObject obj,
                             HandleShape shape, bool checkTypeset)
{
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    GenerateSetSlot(cx, masm, attacher, obj, shape, object(), value(), needsTypeBarrier(),
                    checkTypeset);
    return linkAndAttachStub(cx, masm, attacher, ion, "setting");
}

static bool
IsCacheableSetPropCallNative(HandleObject obj, HandleObject holder, HandleShape shape)
{
    JS_ASSERT(obj->isNative());

    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    return shape->hasSetterValue() && shape->setterObject() &&
           shape->setterObject()->is<JSFunction>() &&
           shape->setterObject()->as<JSFunction>().isNative();
}

static bool
IsCacheableSetPropCallPropertyOp(HandleObject obj, HandleObject holder, HandleShape shape)
{
    JS_ASSERT(obj->isNative());

    if (!shape)
        return false;

    if (!IsCacheableProtoChain(obj, holder))
        return false;

    if (shape->hasSlot())
        return false;

    if (shape->hasDefaultSetter())
        return false;

    if (shape->hasSetterValue())
        return false;

    
    
    
    
    if (!shape->writable())
        return false;

    return true;
}

static bool
EmitCallProxySet(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                 HandleId propId, RegisterSet liveRegs, Register object,
                 ConstantOrRegister value, void *returnAddr, bool strict)
{
    
    MacroAssembler::AfterICSaveLive aic = masm.icSaveLive(liveRegs);

    
    
    RegisterSet regSet(RegisterSet::All());
    regSet.take(AnyRegister(object));

    
    
    Register argJSContextReg = regSet.takeGeneral();
    Register argProxyReg     = regSet.takeGeneral();
    Register argIdReg        = regSet.takeGeneral();
    Register argVpReg        = regSet.takeGeneral();
    Register argStrictReg    = regSet.takeGeneral();

    Register scratch         = regSet.takeGeneral();

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    
    attacher.pushStubCodePointer(masm);

    
    masm.Push(value);
    masm.movePtr(StackPointer, argVpReg);

    masm.Push(propId, scratch);
    masm.movePtr(StackPointer, argIdReg);

    
    
    masm.Push(object);
    masm.Push(object);
    masm.movePtr(StackPointer, argProxyReg);

    masm.loadJSContext(argJSContextReg);
    masm.move32(Imm32(strict? 1 : 0), argStrictReg);

    if (!masm.icBuildOOLFakeExitFrame(returnAddr, aic))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_OOL_PROXY);

    
    masm.setupUnalignedABICall(6, scratch);
    masm.passABIArg(argJSContextReg);
    masm.passABIArg(argProxyReg);
    masm.passABIArg(argProxyReg);
    masm.passABIArg(argIdReg);
    masm.passABIArg(argStrictReg);
    masm.passABIArg(argVpReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Proxy::set));

    
    masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

    
    

    
    masm.adjustStack(IonOOLProxyExitFrameLayout::Size());
    JS_ASSERT(masm.framePushed() == initialStack);

    
    masm.PopRegsInMask(liveRegs);

    return true;
}

bool
SetPropertyIC::attachGenericProxy(JSContext *cx, IonScript *ion, void *returnAddr)
{
    JS_ASSERT(!hasGenericProxyStub());

    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    masm.setFramePushed(ion->frameSize());

    Label failures;
    {
        Label proxyFailures;
        Label proxySuccess;

        RegisterSet regSet(RegisterSet::All());
        regSet.take(AnyRegister(object()));
        if (!value().constant())
            regSet.takeUnchecked(value().reg());

        Register scratch = regSet.takeGeneral();
        masm.push(scratch);

        GenerateProxyClassGuards(masm, object(), scratch, &proxyFailures);

        
        
        masm.branchTestProxyHandlerFamily(Assembler::NotEqual, object(), scratch,
                                          GetDOMProxyHandlerFamily(), &proxySuccess);

        masm.bind(&proxyFailures);
        masm.pop(scratch);
        
        masm.jump(&failures);

        masm.bind(&proxySuccess);
        masm.pop(scratch);
    }

    RootedId propId(cx, AtomToId(name()));
    if (!EmitCallProxySet(cx, masm, attacher, propId, liveRegs_, object(), value(),
                          returnAddr, strict()))
    {
        return false;
    }

    attacher.jumpRejoin(masm);

    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    JS_ASSERT(!hasGenericProxyStub_);
    hasGenericProxyStub_ = true;

    return linkAndAttachStub(cx, masm, attacher, ion, "generic proxy set");
}

bool
SetPropertyIC::attachDOMProxyShadowed(JSContext *cx, IonScript *ion, HandleObject obj,
                                        void *returnAddr)
{
    JS_ASSERT(IsCacheableDOMProxy(obj));

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    masm.setFramePushed(ion->frameSize());

    
    masm.branchPtr(Assembler::NotEqual,
                   Address(object(), JSObject::offsetOfShape()),
                   ImmGCPtr(obj->lastProperty()), &failures);

    
    GenerateDOMProxyChecks(cx, masm, obj, name(), object(), &failures,
                           true);

    RootedId propId(cx, AtomToId(name()));
    if (!EmitCallProxySet(cx, masm, attacher, propId, liveRegs_, object(),
                          value(), returnAddr, strict()))
    {
        return false;
    }

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "DOM proxy shadowed set");
}

static bool
GenerateCallSetter(JSContext *cx, IonScript *ion, MacroAssembler &masm,
                   IonCache::StubAttacher &attacher, HandleObject obj,
                   HandleObject holder, HandleShape shape, bool strict, Register object,
                   ConstantOrRegister value, Label *failure, RegisterSet liveRegs,
                   void *returnAddr)
{
    
    
    {
        RegisterSet regSet(RegisterSet::All());
        regSet.take(AnyRegister(object));
        if (!value.constant())
            regSet.takeUnchecked(value.reg());
        Register scratchReg = regSet.takeGeneral();
        masm.push(scratchReg);

        Label protoFailure;
        Label protoSuccess;

        
        if (obj != holder)
            GeneratePrototypeGuards(cx, ion, masm, obj, holder, object, scratchReg, &protoFailure);

        masm.moveNurseryPtr(ImmMaybeNurseryPtr(holder), scratchReg);
        masm.branchPtr(Assembler::NotEqual,
                       Address(scratchReg, JSObject::offsetOfShape()),
                       ImmGCPtr(holder->lastProperty()),
                       &protoFailure);

        masm.jump(&protoSuccess);

        masm.bind(&protoFailure);
        masm.pop(scratchReg);
        masm.jump(failure);

        masm.bind(&protoSuccess);
        masm.pop(scratchReg);
    }

    

    
    MacroAssembler::AfterICSaveLive aic = masm.icSaveLive(liveRegs);

    
    
    RegisterSet regSet(RegisterSet::All());
    regSet.take(AnyRegister(object));

    
    
    
    
    
    
    
    Register scratchReg     = regSet.takeGeneral();
    Register argJSContextReg = regSet.takeGeneral();
    Register argVpReg        = regSet.takeGeneral();

    bool callNative = IsCacheableSetPropCallNative(obj, holder, shape);
    JS_ASSERT_IF(!callNative, IsCacheableSetPropCallPropertyOp(obj, holder, shape));

    
    DebugOnly<uint32_t> initialStack = masm.framePushed();

    if (callNative) {
        JS_ASSERT(shape->hasSetterValue() && shape->setterObject() &&
                  shape->setterObject()->is<JSFunction>());
        JSFunction *target = &shape->setterObject()->as<JSFunction>();

        JS_ASSERT(target->isNative());

        Register argUintNReg = regSet.takeGeneral();

        
        
        
        
        

        
        masm.Push(value);
        masm.Push(TypedOrValueRegister(MIRType_Object, AnyRegister(object)));
        masm.Push(ObjectValue(*target));
        masm.movePtr(StackPointer, argVpReg);

        
        masm.loadJSContext(argJSContextReg);
        masm.move32(Imm32(1), argUintNReg);

        
        masm.Push(argUintNReg);
        attacher.pushStubCodePointer(masm);

        if (!masm.icBuildOOLFakeExitFrame(returnAddr, aic))
            return false;
        masm.enterFakeExitFrame(ION_FRAME_OOL_NATIVE);

        
        masm.setupUnalignedABICall(3, scratchReg);
        masm.passABIArg(argJSContextReg);
        masm.passABIArg(argUintNReg);
        masm.passABIArg(argVpReg);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));
    } else {
        Register argObjReg       = regSet.takeGeneral();
        Register argIdReg        = regSet.takeGeneral();
        Register argStrictReg    = regSet.takeGeneral();

        attacher.pushStubCodePointer(masm);

        StrictPropertyOp target = shape->setterOp();
        JS_ASSERT(target);
        
        

        
        if (value.constant())
            masm.Push(value.value());
        else
            masm.Push(value.reg());
        masm.movePtr(StackPointer, argVpReg);

        masm.move32(Imm32(strict ? 1 : 0), argStrictReg);

        
        RootedId propId(cx);
        if (!shape->getUserId(cx, &propId))
            return false;
        masm.Push(propId, argIdReg);
        masm.movePtr(StackPointer, argIdReg);

        masm.Push(object);
        masm.movePtr(StackPointer, argObjReg);

        masm.loadJSContext(argJSContextReg);

        if (!masm.icBuildOOLFakeExitFrame(returnAddr, aic))
            return false;
        masm.enterFakeExitFrame(ION_FRAME_OOL_PROPERTY_OP);

        
        masm.setupUnalignedABICall(5, scratchReg);
        masm.passABIArg(argJSContextReg);
        masm.passABIArg(argObjReg);
        masm.passABIArg(argIdReg);
        masm.passABIArg(argStrictReg);
        masm.passABIArg(argVpReg);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target));
    }

    
    masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

    
    

    
    if (callNative)
        masm.adjustStack(IonOOLNativeExitFrameLayout::Size(1));
    else
        masm.adjustStack(IonOOLPropertyOpExitFrameLayout::Size());
    JS_ASSERT(masm.framePushed() == initialStack);

    
    masm.PopRegsInMask(liveRegs);

    return true;
}

static bool
IsCacheableDOMProxyUnshadowedSetterCall(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                        MutableHandleObject holder, MutableHandleShape shape,
                                        bool *isSetter)
{
    JS_ASSERT(IsCacheableDOMProxy(obj));

    *isSetter = false;

    RootedObject checkObj(cx, obj->getTaggedProto().toObjectOrNull());
    if (!checkObj)
        return true;

    if (!JSObject::lookupProperty(cx, obj, name, holder, shape))
        return false;

    if (!holder)
        return true;

    if (!IsCacheableSetPropCallNative(checkObj, holder, shape) &&
        !IsCacheableSetPropCallPropertyOp(checkObj, holder, shape))
    {
        return true;
    }

    *isSetter = true;
    return true;
}

bool
SetPropertyIC::attachDOMProxyUnshadowed(JSContext *cx, IonScript *ion, HandleObject obj,
                                        void *returnAddr)
{
    JS_ASSERT(IsCacheableDOMProxy(obj));

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    masm.setFramePushed(ion->frameSize());

    
    masm.branchPtr(Assembler::NotEqual,
                   Address(object(), JSObject::offsetOfShape()),
                   ImmGCPtr(obj->lastProperty()), &failures);

    
    GenerateDOMProxyChecks(cx, masm, obj, name(), object(), &failures);

    RootedPropertyName propName(cx, name());
    RootedObject holder(cx);
    RootedShape shape(cx);
    bool isSetter;
    if (!IsCacheableDOMProxyUnshadowedSetterCall(cx, obj, propName, &holder,
                                                 &shape, &isSetter))
    {
        return false;
    }

    if (isSetter) {
        if (!GenerateCallSetter(cx, ion, masm, attacher, obj, holder, shape, strict(),
                                object(), value(), &failures, liveRegs_, returnAddr))
        {
            return false;
        }
    } else {
        
        
        RootedId propId(cx, AtomToId(name()));
        if (!EmitCallProxySet(cx, masm, attacher, propId, liveRegs_, object(),
                            value(), returnAddr, strict()))
        {
            return false;
        }
    }

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "DOM proxy unshadowed set");
}

bool
SetPropertyIC::attachCallSetter(JSContext *cx, IonScript *ion,
                                HandleObject obj, HandleObject holder, HandleShape shape,
                                void *returnAddr)
{
    JS_ASSERT(obj->isNative());

    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    
    
    masm.setFramePushed(ion->frameSize());

    Label failure;
    masm.branchPtr(Assembler::NotEqual,
                   Address(object(), JSObject::offsetOfShape()),
                   ImmGCPtr(obj->lastProperty()),
                   &failure);

    if (!GenerateCallSetter(cx, ion, masm, attacher, obj, holder, shape, strict(),
                            object(), value(), &failure, liveRegs_, returnAddr))
    {
        return false;
    }

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failure);
    attacher.jumpNextStub(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "setter call");
}

static void
GenerateAddSlot(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                JSObject *obj, Shape *oldShape, Register object, ConstantOrRegister value,
                bool checkTypeset)
{
    JS_ASSERT(obj->isNative());

    Label failures;

    
    masm.branchPtr(Assembler::NotEqual, Address(object, JSObject::offsetOfType()),
                   ImmGCPtr(obj->type()), &failures);

    
    masm.branchTestObjShape(Assembler::NotEqual, object, oldShape, &failures);

    Label failuresPopObject;
    masm.push(object);    

    
    
    if (checkTypeset) {
        TypedOrValueRegister valReg = value.reg();
        types::TypeObject *type = obj->type();
        types::HeapTypeSet *propTypes = type->maybeGetProperty(obj->lastProperty()->propid());
        JS_ASSERT(propTypes);
        JS_ASSERT(!propTypes->unknown());

        Register scratchReg = object;
        masm.guardTypeSet(valReg, propTypes, scratchReg, &failuresPopObject);
        masm.loadPtr(Address(StackPointer, 0), object);
    }

    JSObject *proto = obj->getProto();
    Register protoReg = object;
    while (proto) {
        Shape *protoShape = proto->lastProperty();

        
        masm.loadPtr(Address(protoReg, JSObject::offsetOfType()), protoReg);
        masm.loadPtr(Address(protoReg, offsetof(types::TypeObject, proto)), protoReg);

        
        masm.branchTestObjShape(Assembler::NotEqual, protoReg, protoShape, &failuresPopObject);

        proto = proto->getProto();
    }

    masm.pop(object);     

    
    Shape *newShape = obj->lastProperty();
    Address shapeAddr(object, JSObject::offsetOfShape());
    if (cx->zone()->needsBarrier())
        masm.callPreBarrier(shapeAddr, MIRType_Shape);
    masm.storePtr(ImmGCPtr(newShape), shapeAddr);

    
    
    if (obj->isFixedSlot(newShape->slot())) {
        Address addr(object, JSObject::getFixedSlotOffset(newShape->slot()));
        masm.storeConstantOrRegister(value, addr);
    } else {
        Register slotsReg = object;

        masm.loadPtr(Address(object, JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(newShape->slot()) * sizeof(Value));
        masm.storeConstantOrRegister(value, addr);
    }

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failuresPopObject);
    masm.pop(object);
    masm.bind(&failures);

    attacher.jumpNextStub(masm);
}

bool
SetPropertyIC::attachAddSlot(JSContext *cx, IonScript *ion, JSObject *obj, HandleShape oldShape,
                             bool checkTypeset)
{
    JS_ASSERT_IF(!needsTypeBarrier(), !checkTypeset);

    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    GenerateAddSlot(cx, masm, attacher, obj, oldShape, object(), value(), checkTypeset);
    return linkAndAttachStub(cx, masm, attacher, ion, "adding");
}

static bool
CanInlineSetPropTypeCheck(JSObject *obj, jsid id, ConstantOrRegister val, bool *checkTypeset)
{
    bool shouldCheck = false;
    types::TypeObject *type = obj->type();
    if (!type->unknownProperties()) {
        types::HeapTypeSet *propTypes = type->maybeGetProperty(id);
        if (!propTypes)
            return false;
        if (!propTypes->unknown()) {
            shouldCheck = true;
            if (val.constant()) {
                
                if (!propTypes->hasType(types::GetValueType(val.value())))
                    return false;
                shouldCheck = false;
            } else {
                TypedOrValueRegister reg = val.reg();
                
                
                
                
                if (reg.hasTyped() && reg.type() != MIRType_Object) {
                    JSValueType valType = ValueTypeFromMIRType(reg.type());
                    if (!propTypes->hasType(types::Type::PrimitiveType(valType)))
                        return false;
                    shouldCheck = false;
                }
            }
        }
    }

    *checkTypeset = shouldCheck;
    return true;
}

static bool
IsPropertySetInlineable(HandleObject obj, HandleId id, MutableHandleShape pshape,
                        ConstantOrRegister val, bool needsTypeBarrier, bool *checkTypeset)
{
    JS_ASSERT(obj->isNative());

    
    
    pshape.set(obj->nativeLookupPure(id));

    if (!pshape)
        return false;

    if (!pshape->hasSlot())
        return false;

    if (!pshape->hasDefaultSetter())
        return false;

    if (!pshape->writable())
        return false;

    if (needsTypeBarrier)
        return CanInlineSetPropTypeCheck(obj, id, val, checkTypeset);

    return true;
}

static bool
IsPropertyAddInlineable(HandleObject obj, HandleId id, ConstantOrRegister val, uint32_t oldSlots,
                        HandleShape oldShape, bool needsTypeBarrier, bool *checkTypeset)
{
    JS_ASSERT(obj->isNative());

    
    if (obj->lastProperty() == oldShape)
        return false;

    Shape *shape = obj->nativeLookupPure(id);
    if (!shape || shape->inDictionary() || !shape->hasSlot() || !shape->hasDefaultSetter())
        return false;

    
    
    JS_ASSERT(shape == obj->lastProperty());

    
    if (obj->getClass()->resolve != JS_ResolveStub)
        return false;

    
    
    if (obj->getClass()->addProperty != JS_PropertyStub)
        return false;

    if (!obj->nonProxyIsExtensible() || !shape->writable())
        return false;

    
    
    
    for (JSObject *proto = obj->getProto(); proto; proto = proto->getProto()) {
        
        if (!proto->isNative())
            return false;

        
        Shape *protoShape = proto->nativeLookupPure(id);
        if (protoShape && !protoShape->hasDefaultSetter())
            return false;

        
        
        
        if (proto->getClass()->resolve != JS_ResolveStub)
             return false;
    }

    
    
    
    if (obj->numDynamicSlots() != oldSlots)
        return false;

    if (needsTypeBarrier)
        return CanInlineSetPropTypeCheck(obj, id, val, checkTypeset);

    *checkTypeset = false;
    return true;
}

static SetPropertyIC::NativeSetPropCacheability
CanAttachNativeSetProp(HandleObject obj, HandleId id, ConstantOrRegister val,
                       bool needsTypeBarrier, MutableHandleObject holder,
                       MutableHandleShape shape, bool *checkTypeset)
{
    if (!obj->isNative())
        return SetPropertyIC::CanAttachNone;

    
    if (IsPropertySetInlineable(obj, id, shape, val, needsTypeBarrier, checkTypeset))
        return SetPropertyIC::CanAttachSetSlot;

    
    
    if (!LookupPropertyPure(obj, id, holder.address(), shape.address()))
        return SetPropertyIC::CanAttachNone;

    
    
    
    
    if (!shape || (obj != holder && shape->hasDefaultSetter() && shape->hasSlot()))
        return SetPropertyIC::MaybeCanAttachAddSlot;

    if (IsCacheableSetPropCallPropertyOp(obj, holder, shape) ||
        IsCacheableSetPropCallNative(obj, holder, shape))
    {
        return SetPropertyIC::CanAttachCallSetter;
    }

    return SetPropertyIC::CanAttachNone;
}

bool
SetPropertyIC::update(JSContext *cx, size_t cacheIndex, HandleObject obj,
                      HandleValue value)
{
    AutoFlushCache afc ("SetPropertyCache", cx->runtime()->jitRuntime());

    void *returnAddr;
    RootedScript script(cx, GetTopIonJSScript(cx, &returnAddr));
    IonScript *ion = script->ionScript();
    SetPropertyIC &cache = ion->getCache(cacheIndex).toSetProperty();
    RootedPropertyName name(cx, cache.name());
    RootedId id(cx, AtomToId(name));

    
    
    bool inlinable = cache.canAttachStub() && !obj->watched();
    NativeSetPropCacheability canCache = CanAttachNone;
    bool addedSetterStub = false;
    if (inlinable) {
        if (!addedSetterStub && obj->is<ProxyObject>()) {
            if (IsCacheableDOMProxy(obj)) {
                DOMProxyShadowsResult shadows = GetDOMProxyShadowsCheck()(cx, obj, id);
                if (shadows == ShadowCheckFailed)
                    return false;
                if (shadows == Shadows) {
                    if (!cache.attachDOMProxyShadowed(cx, ion, obj, returnAddr))
                        return false;
                    addedSetterStub = true;
                } else {
                    JS_ASSERT(shadows == DoesntShadow || shadows == DoesntShadowUnique);
                    if (shadows == DoesntShadowUnique)
                        cache.reset();
                    if (!cache.attachDOMProxyUnshadowed(cx, ion, obj, returnAddr))
                        return false;
                    addedSetterStub = true;
                }
            }

            if (!addedSetterStub && !cache.hasGenericProxyStub()) {
                if (!cache.attachGenericProxy(cx, ion, returnAddr))
                    return false;
                addedSetterStub = true;
            }
        }

        
        
        
        if (obj->isNative() && !obj->getType(cx))
            return false;

        RootedShape shape(cx);
        RootedObject holder(cx);
        bool checkTypeset;
        canCache = CanAttachNativeSetProp(obj, id, cache.value(), cache.needsTypeBarrier(),
                                          &holder, &shape, &checkTypeset);

        if (!addedSetterStub && canCache == CanAttachSetSlot) {
            if (!cache.attachSetSlot(cx, ion, obj, shape, checkTypeset))
                return false;
            addedSetterStub = true;
        }

        if (!addedSetterStub && canCache == CanAttachCallSetter) {
            if (!cache.attachCallSetter(cx, ion, obj, holder, shape, returnAddr))
                return false;
            addedSetterStub = true;
        }
    }

    uint32_t oldSlots = obj->numDynamicSlots();
    RootedShape oldShape(cx, obj->lastProperty());

    
    if (!SetProperty(cx, obj, name, value, cache.strict(), cache.pc()))
        return false;

    
    bool checkTypeset;
    if (!addedSetterStub && canCache == MaybeCanAttachAddSlot &&
        IsPropertyAddInlineable(obj, id, cache.value(), oldSlots, oldShape, cache.needsTypeBarrier(),
                                &checkTypeset))
    {
        if (!cache.attachAddSlot(cx, ion, obj, oldShape, checkTypeset))
            return false;
    }

    return true;
}

void
SetPropertyIC::reset()
{
    RepatchIonCache::reset();
    hasGenericProxyStub_ = false;
}

bool
SetPropertyParIC::update(ForkJoinSlice *slice, size_t cacheIndex, HandleObject obj,
                         HandleValue value)
{
    JS_ASSERT(slice->isThreadLocal(obj));

    AutoFlushCache afc("SetPropertyParCache", slice->runtime()->jitRuntime());

    IonScript *ion = GetTopIonJSScript(slice)->parallelIonScript();
    SetPropertyParIC &cache = ion->getCache(cacheIndex).toSetPropertyPar();

    RootedValue v(slice, value);
    RootedId id(slice, AtomToId(cache.name()));

    
    if (!cache.canAttachStub()) {
        return baseops::SetPropertyHelper<ParallelExecution>(slice, obj, obj, id, 0,
                                                             &v, cache.strict());
    }

    SetPropertyIC::NativeSetPropCacheability canCache = SetPropertyIC::CanAttachNone;
    bool attachedStub = false;

    {
        LockedJSContext cx(slice);

        if (cache.canAttachStub()) {
            bool alreadyStubbed;
            if (!cache.hasOrAddStubbedShape(cx, obj->lastProperty(), &alreadyStubbed))
                return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
            if (alreadyStubbed) {
                return baseops::SetPropertyHelper<ParallelExecution>(slice, obj, obj, id, 0,
                                                                     &v, cache.strict());
            }

            
            
            if (obj->hasLazyType())
                return false;

            {
                RootedShape shape(slice);
                RootedObject holder(slice);
                bool checkTypeset;
                canCache = CanAttachNativeSetProp(obj, id, cache.value(), cache.needsTypeBarrier(),
                                                  &holder, &shape, &checkTypeset);

                if (canCache == SetPropertyIC::CanAttachSetSlot) {
                    if (!cache.attachSetSlot(cx, ion, obj, shape, checkTypeset))
                        return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                    attachedStub = true;
                }
            }
        }
    }

    uint32_t oldSlots = obj->numDynamicSlots();
    RootedShape oldShape(slice, obj->lastProperty());

    if (!baseops::SetPropertyHelper<ParallelExecution>(slice, obj, obj, id, 0, &v, cache.strict()))
        return false;

    bool checkTypeset;
    if (!attachedStub && canCache == SetPropertyIC::MaybeCanAttachAddSlot &&
        IsPropertyAddInlineable(obj, id, cache.value(), oldSlots, oldShape, cache.needsTypeBarrier(),
                                &checkTypeset))
    {
        LockedJSContext cx(slice);
        if (cache.canAttachStub() && !cache.attachAddSlot(cx, ion, obj, oldShape, checkTypeset))
            return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
    }

    return true;
}

bool
SetPropertyParIC::attachSetSlot(LockedJSContext &cx, IonScript *ion, JSObject *obj, Shape *shape,
                                  bool checkTypeset)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    GenerateSetSlot(cx, masm, attacher, obj, shape, object(), value(), needsTypeBarrier(),
                    checkTypeset);
    return linkAndAttachStub(cx, masm, attacher, ion, "parallel setting");
}

bool
SetPropertyParIC::attachAddSlot(LockedJSContext &cx, IonScript *ion, JSObject *obj, Shape *oldShape,
                                bool checkTypeset)
{
    JS_ASSERT_IF(!needsTypeBarrier(), !checkTypeset);

    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    GenerateAddSlot(cx, masm, attacher, obj, oldShape, object(), value(), checkTypeset);
    return linkAndAttachStub(cx, masm, attacher, ion, "parallel adding");
}

const size_t GetElementIC::MAX_FAILED_UPDATES = 16;

 bool
GetElementIC::canAttachGetProp(JSObject *obj, const Value &idval, jsid id)
{
    uint32_t dummy;
    return (obj->isNative() &&
            idval.isString() &&
            JSID_IS_ATOM(id) &&
            !JSID_TO_ATOM(id)->isIndex(&dummy));
}

static bool
EqualStringsHelper(JSString *str1, JSString *str2)
{
    JS_ASSERT(str1->isAtom());
    JS_ASSERT(!str2->isAtom());
    JS_ASSERT(str1->length() == str2->length());

    const jschar *chars = str2->getChars(nullptr);
    if (!chars)
        return false;
    return mozilla::PodEqual(str1->asAtom().chars(), chars, str1->length());
}

bool
GetElementIC::attachGetProp(JSContext *cx, IonScript *ion, HandleObject obj,
                            const Value &idval, HandlePropertyName name,
                            void *returnAddr)
{
    JS_ASSERT(index().reg().hasValue());

    RootedObject holder(cx);
    RootedShape shape(cx);

    GetPropertyIC::NativeGetPropCacheability canCache =
        CanAttachNativeGetProp(cx, *this, obj, name, &holder, &shape,
                               true);

    bool cacheable = canCache == GetPropertyIC::CanAttachReadSlot ||
                     (canCache == GetPropertyIC::CanAttachCallGetter &&
                      output().hasValue());

    if (!cacheable) {
        IonSpew(IonSpew_InlineCaches, "GETELEM uncacheable property");
        return true;
    }

    JS_ASSERT(idval.isString());
    JS_ASSERT(idval.toString()->length() == name->length());

    Label failures;
    MacroAssembler masm(cx);
    SkipRoot skip(cx, &masm);

    
    ValueOperand val = index().reg().valueReg();
    masm.branchTestString(Assembler::NotEqual, val, &failures);

    Register scratch = output().valueReg().scratchReg();
    masm.unboxString(val, scratch);

    Label equal;
    masm.branchPtr(Assembler::Equal, scratch, ImmGCPtr(name), &equal);

    
    
    masm.loadPtr(Address(scratch, JSString::offsetOfLengthAndFlags()), scratch);
    masm.branchTest32(Assembler::NonZero, scratch, Imm32(JSString::ATOM_BIT), &failures);

    
    masm.rshiftPtr(Imm32(JSString::LENGTH_SHIFT), scratch);
    masm.branch32(Assembler::NotEqual, scratch, Imm32(name->length()), &failures);

    
    
    RegisterSet volatileRegs = RegisterSet::Volatile();
    masm.PushRegsInMask(volatileRegs);

    Register objReg = object();
    JS_ASSERT(objReg != scratch);

    if (!volatileRegs.has(objReg))
        masm.push(objReg);

    masm.setupUnalignedABICall(2, scratch);
    masm.movePtr(ImmGCPtr(name), objReg);
    masm.passABIArg(objReg);
    masm.unboxString(val, scratch);
    masm.passABIArg(scratch);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, EqualStringsHelper));
    masm.mov(ReturnReg, scratch);

    if (!volatileRegs.has(objReg))
        masm.pop(objReg);

    RegisterSet ignore = RegisterSet();
    ignore.add(scratch);
    masm.PopRegsInMaskIgnore(volatileRegs, ignore);

    masm.branchIfFalseBool(scratch, &failures);
    masm.bind(&equal);

    RepatchStubAppender attacher(*this);
    if (canCache == GetPropertyIC::CanAttachReadSlot) {
        GenerateReadSlot(cx, ion, masm, attacher, obj, holder, shape, object(), output(),
                         &failures);
    } else {
        JS_ASSERT(canCache == GetPropertyIC::CanAttachCallGetter);
        
        masm.setFramePushed(ion->frameSize());
        if (!GenerateCallGetter(cx, ion, masm, attacher, obj, name, holder, shape, liveRegs_,
                                object(), output(), returnAddr, &failures))
        {
            return false;
        }
    }

    return linkAndAttachStub(cx, masm, attacher, ion, "property");
}

 bool
GetElementIC::canAttachDenseElement(JSObject *obj, const Value &idval)
{
    return obj->isNative() && idval.isInt32();
}

static bool
GenerateDenseElement(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                     JSObject *obj, const Value &idval, Register object,
                     ConstantOrRegister index, TypedOrValueRegister output)
{
    JS_ASSERT(GetElementIC::canAttachDenseElement(obj, idval));

    Label failures;

    
    RootedShape shape(cx, obj->lastProperty());
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object, shape, &failures);

    
    Register indexReg = InvalidReg;

    if (index.reg().hasValue()) {
        indexReg = output.scratchReg().gpr();
        JS_ASSERT(indexReg != InvalidReg);
        ValueOperand val = index.reg().valueReg();

        masm.branchTestInt32(Assembler::NotEqual, val, &failures);

        
        masm.unboxInt32(val, indexReg);
    } else {
        JS_ASSERT(!index.reg().typedReg().isFloat());
        indexReg = index.reg().typedReg().gpr();
    }

    
    masm.push(object);
    masm.loadPtr(Address(object, JSObject::offsetOfElements()), object);

    Label hole;

    
    Address initLength(object, ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, indexReg, &hole);

    
    masm.loadElementTypedOrValue(BaseIndex(object, indexReg, TimesEight),
                                 output, true, &hole);

    masm.pop(object);
    attacher.jumpRejoin(masm);

    
    masm.bind(&hole);
    masm.pop(object);
    masm.bind(&failures);

    attacher.jumpNextStub(masm);

    return true;
}

bool
GetElementIC::attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval)
{
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    if (!GenerateDenseElement(cx, masm, attacher, obj, idval, object(), index(), output()))
        return false;

    setHasDenseStub();
    return linkAndAttachStub(cx, masm, attacher, ion, "dense array");
}

 bool
GetElementIC::canAttachTypedArrayElement(JSObject *obj, const Value &idval,
                                         TypedOrValueRegister output)
{
    if (!obj->is<TypedArrayObject>())
        return false;

    if (!idval.isInt32() && !idval.isString())
        return false;


    
    
    
    
    uint32_t index;
    if (idval.isInt32()) {
        index = idval.toInt32();
    } else {
        index = GetIndexFromString(idval.toString());
        if (index == UINT32_MAX)
            return false;
    }
    if (index >= obj->as<TypedArrayObject>().length())
        return false;

    
    
    int arrayType = obj->as<TypedArrayObject>().type();
    bool floatOutput = arrayType == ScalarTypeRepresentation::TYPE_FLOAT32 ||
                       arrayType == ScalarTypeRepresentation::TYPE_FLOAT64;
    return !floatOutput || output.hasValue();
}

static void
GenerateGetTypedArrayElement(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                             TypedArrayObject *tarr, const Value &idval, Register object,
                             ConstantOrRegister index, TypedOrValueRegister output)
{
    JS_ASSERT(GetElementIC::canAttachTypedArrayElement(tarr, idval, output));

    Label failures;

    
    int arrayType = tarr->type();

    
    Shape *shape = tarr->lastProperty();
    masm.branchTestObjShape(Assembler::NotEqual, object, shape, &failures);

    
    Register tmpReg = output.scratchReg().gpr();
    JS_ASSERT(tmpReg != InvalidReg);
    Register indexReg = tmpReg;
    JS_ASSERT(!index.constant());
    if (idval.isString()) {
        JS_ASSERT(GetIndexFromString(idval.toString()) != UINT32_MAX);

        
        Register str;
        if (index.reg().hasValue()) {
            ValueOperand val = index.reg().valueReg();
            masm.branchTestString(Assembler::NotEqual, val, &failures);

            str = masm.extractString(val, indexReg);
        } else {
            JS_ASSERT(!index.reg().typedReg().isFloat());
            str = index.reg().typedReg().gpr();
        }

        
        RegisterSet regs = RegisterSet::Volatile();
        masm.PushRegsInMask(regs);
        regs.takeUnchecked(str);

        Register temp = regs.takeGeneral();

        masm.setupUnalignedABICall(1, temp);
        masm.passABIArg(str);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, GetIndexFromString));
        masm.mov(ReturnReg, indexReg);

        RegisterSet ignore = RegisterSet();
        ignore.add(indexReg);
        masm.PopRegsInMaskIgnore(RegisterSet::Volatile(), ignore);

        masm.branch32(Assembler::Equal, indexReg, Imm32(UINT32_MAX), &failures);

    } else {
        JS_ASSERT(idval.isInt32());

        if (index.reg().hasValue()) {
            ValueOperand val = index.reg().valueReg();
            masm.branchTestInt32(Assembler::NotEqual, val, &failures);

            
            masm.unboxInt32(val, indexReg);
        } else {
            JS_ASSERT(!index.reg().typedReg().isFloat());
            indexReg = index.reg().typedReg().gpr();
        }
    }

    
    Address length(object, TypedArrayObject::lengthOffset());
    masm.branch32(Assembler::BelowOrEqual, length, indexReg, &failures);

    
    Label popAndFail;
    Register elementReg = object;
    masm.push(object);

    
    masm.loadPtr(Address(object, TypedArrayObject::dataOffset()), elementReg);

    
    
    int width = TypedArrayObject::slotWidth(arrayType);
    BaseIndex source(elementReg, indexReg, ScaleFromElemWidth(width));
    if (output.hasValue())
        masm.loadFromTypedArray(arrayType, source, output.valueReg(), true,
                                elementReg, &popAndFail);
    else
        masm.loadFromTypedArray(arrayType, source, output.typedReg(),
                                elementReg, &popAndFail);

    masm.pop(object);
    attacher.jumpRejoin(masm);

    
    masm.bind(&popAndFail);
    masm.pop(object);
    masm.bind(&failures);

    attacher.jumpNextStub(masm);
}

bool
GetElementIC::attachTypedArrayElement(JSContext *cx, IonScript *ion, TypedArrayObject *tarr,
                                      const Value &idval)
{
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    GenerateGetTypedArrayElement(cx, masm, attacher, tarr, idval, object(), index(), output());
    return linkAndAttachStub(cx, masm, attacher, ion, "typed array");
}

bool
GetElementIC::attachArgumentsElement(JSContext *cx, IonScript *ion, JSObject *obj)
{
    JS_ASSERT(obj->is<ArgumentsObject>());

    Label failures;
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    Register tmpReg = output().scratchReg().gpr();
    JS_ASSERT(tmpReg != InvalidReg);

    const Class *clasp = obj->is<StrictArgumentsObject>() ? &StrictArgumentsObject::class_
                                                          : &NormalArgumentsObject::class_;

    masm.branchTestObjClass(Assembler::NotEqual, object(), tmpReg, clasp, &failures);

    
    masm.unboxInt32(Address(object(), ArgumentsObject::getInitialLengthSlotOffset()), tmpReg);
    masm.branchTest32(Assembler::NonZero, tmpReg, Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT),
                      &failures);
    masm.rshiftPtr(Imm32(ArgumentsObject::PACKED_BITS_COUNT), tmpReg);

    
    Register indexReg;
    JS_ASSERT(!index().constant());

    
    Label failureRestoreIndex;
    if (index().reg().hasValue()) {
        ValueOperand val = index().reg().valueReg();
        masm.branchTestInt32(Assembler::NotEqual, val, &failures);
        indexReg = val.scratchReg();

        masm.unboxInt32(val, indexReg);
        masm.branch32(Assembler::AboveOrEqual, indexReg, tmpReg, &failureRestoreIndex);
    } else {
        JS_ASSERT(index().reg().type() == MIRType_Int32);
        indexReg = index().reg().typedReg().gpr();
        masm.branch32(Assembler::AboveOrEqual, indexReg, tmpReg, &failures);
    }
    
    Label failurePopIndex;
    masm.push(indexReg);

    
    masm.loadPrivate(Address(object(), ArgumentsObject::getDataSlotOffset()), tmpReg);
    masm.loadPtr(Address(tmpReg, offsetof(ArgumentsData, deletedBits)), tmpReg);

    
    const uint32_t shift = FloorLog2<(sizeof(size_t) * JS_BITS_PER_BYTE)>::value;
    JS_ASSERT(shift == 5 || shift == 6);
    masm.rshiftPtr(Imm32(shift), indexReg);
    masm.loadPtr(BaseIndex(tmpReg, indexReg, ScaleFromElemWidth(sizeof(size_t))), tmpReg);

    
    masm.branchPtr(Assembler::NotEqual, tmpReg, ImmPtr(nullptr), &failurePopIndex);

    
    masm.loadPrivate(Address(object(), ArgumentsObject::getDataSlotOffset()), tmpReg);
    masm.addPtr(Imm32(ArgumentsData::offsetOfArgs()), tmpReg);

    
    masm.pop(indexReg);
    BaseIndex elemIdx(tmpReg, indexReg, ScaleFromElemWidth(sizeof(Value)));

    
    masm.branchTestMagic(Assembler::Equal, elemIdx, &failureRestoreIndex);

    if (output().hasTyped()) {
        JS_ASSERT(!output().typedReg().isFloat());
        JS_ASSERT(index().reg().type() == MIRType_Boolean ||
                  index().reg().type() == MIRType_Int32 ||
                  index().reg().type() == MIRType_String ||
                  index().reg().type() == MIRType_Object);
        masm.branchTestMIRType(Assembler::NotEqual, elemIdx, index().reg().type(),
                               &failureRestoreIndex);
    }

    masm.loadTypedOrValue(elemIdx, output());

    
    if (index().reg().hasValue())
        masm.tagValue(JSVAL_TYPE_INT32, indexReg, index().reg().valueReg());

    
    attacher.jumpRejoin(masm);

    
    masm.bind(&failurePopIndex);
    masm.pop(indexReg);
    masm.bind(&failureRestoreIndex);
    if (index().reg().hasValue())
        masm.tagValue(JSVAL_TYPE_INT32, indexReg, index().reg().valueReg());
    masm.bind(&failures);
    attacher.jumpNextStub(masm);


    if (obj->is<StrictArgumentsObject>()) {
        JS_ASSERT(!hasStrictArgumentsStub_);
        hasStrictArgumentsStub_ = true;
        return linkAndAttachStub(cx, masm, attacher, ion, "ArgsObj element (strict)");
    }

    JS_ASSERT(!hasNormalArgumentsStub_);
    hasNormalArgumentsStub_ = true;
    return linkAndAttachStub(cx, masm, attacher, ion, "ArgsObj element (normal)");
}

bool
GetElementIC::update(JSContext *cx, size_t cacheIndex, HandleObject obj,
                     HandleValue idval, MutableHandleValue res)
{
    void *returnAddr;
    IonScript *ion = GetTopIonJSScript(cx, &returnAddr)->ionScript();
    GetElementIC &cache = ion->getCache(cacheIndex).toGetElement();
    RootedScript script(cx);
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    if (cache.isDisabled()) {
        if (!GetObjectElementOperation(cx, JSOp(*pc), obj, true, idval, res))
            return false;
        types::TypeScript::Monitor(cx, script, pc, res);
        return true;
    }

    
    AutoFlushCache afc ("GetElementCache", cx->runtime()->jitRuntime());
    AutoDetectInvalidation adi(cx, res.address(), ion);

    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, idval, &id))
        return false;

    bool attachedStub = false;
    if (cache.canAttachStub()) {
        if (IsOptimizableArgumentsObjectForGetElem(obj, idval) &&
            !cache.hasArgumentsStub(obj->is<StrictArgumentsObject>()) &&
            !cache.index().constant() &&
            (cache.index().reg().hasValue() ||
             cache.index().reg().type() == MIRType_Int32) &&
            (cache.output().hasValue() || !cache.output().typedReg().isFloat()))
        {
            if (!cache.attachArgumentsElement(cx, ion, obj))
                return false;
            attachedStub = true;
        }
        if (!attachedStub && cache.monitoredResult() && canAttachGetProp(obj, idval, id)) {
            RootedPropertyName name(cx, JSID_TO_ATOM(id)->asPropertyName());
            if (!cache.attachGetProp(cx, ion, obj, idval, name, returnAddr))
                return false;
            attachedStub = true;
        }
        if (!attachedStub && !cache.hasDenseStub() && canAttachDenseElement(obj, idval)) {
            if (!cache.attachDenseElement(cx, ion, obj, idval))
                return false;
            attachedStub = true;
        }
        if (!attachedStub && canAttachTypedArrayElement(obj, idval, cache.output())) {
            Rooted<TypedArrayObject*> tarr(cx, &obj->as<TypedArrayObject>());
            if (!cache.attachTypedArrayElement(cx, ion, tarr, idval))
                return false;
            attachedStub = true;
        }
    }

    if (!GetObjectElementOperation(cx, JSOp(*pc), obj, true, idval, res))
        return false;

    
    if (!attachedStub) {
        cache.incFailedUpdates();
        if (cache.shouldDisable()) {
            IonSpew(IonSpew_InlineCaches, "Disable inline cache");
            cache.disable();
        }
    } else {
        cache.resetFailedUpdates();
    }

    types::TypeScript::Monitor(cx, script, pc, res);
    return true;
}

void
GetElementIC::reset()
{
    RepatchIonCache::reset();
    hasDenseStub_ = false;
    hasStrictArgumentsStub_ = false;
    hasNormalArgumentsStub_ = false;
}

static bool
IsDenseElementSetInlineable(JSObject *obj, const Value &idval)
{
    if (!obj->is<ArrayObject>())
        return false;

    if (obj->watched())
        return false;

    if (!idval.isInt32())
        return false;

    
    
    
    
    JSObject *curObj = obj;
    while (curObj) {
        
        if (!curObj->isNative())
            return false;

        
        if (curObj->isIndexed())
            return false;

        curObj = curObj->getProto();
    }

    return true;
}

static bool
IsTypedArrayElementSetInlineable(JSObject *obj, const Value &idval, const Value &value)
{
    
    return (obj->is<TypedArrayObject>() && idval.isInt32() &&
            !value.isString() && !value.isObject());
}

static bool
GenerateSetDenseElement(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                        JSObject *obj, const Value &idval, bool guardHoles, Register object,
                        ValueOperand indexVal, ConstantOrRegister value, Register tempToUnboxIndex,
                        Register temp)
{
    JS_ASSERT(obj->isNative());
    JS_ASSERT(idval.isInt32());

    Label failures;
    Label outOfBounds; 

    Label markElem, storeElement; 

    
    Shape *shape = obj->lastProperty();
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object, shape, &failures);

    
    masm.branchTestInt32(Assembler::NotEqual, indexVal, &failures);

    
    Register index = masm.extractInt32(indexVal, tempToUnboxIndex);

    {
        
        Register elements = temp;
        masm.loadPtr(Address(object, JSObject::offsetOfElements()), elements);

        
        BaseIndex target(elements, index, TimesEight);

        
        
        
        
        if (guardHoles) {
            Address initLength(elements, ObjectElements::offsetOfInitializedLength());
            masm.branch32(Assembler::BelowOrEqual, initLength, index, &outOfBounds);
        } else {
            
            Address capacity(elements, ObjectElements::offsetOfCapacity());
            masm.branch32(Assembler::BelowOrEqual, capacity, index, &outOfBounds);

            
            Address initLength(elements, ObjectElements::offsetOfInitializedLength());
            masm.branch32(Assembler::Below, initLength, index, &outOfBounds);

            
            masm.branch32(Assembler::NotEqual, initLength, index, &markElem);
            {
                
                Int32Key newLength(index);
                masm.bumpKey(&newLength, 1);
                masm.storeKey(newLength, initLength);

                
                Label bumpedLength;
                Address length(elements, ObjectElements::offsetOfLength());
                masm.branch32(Assembler::AboveOrEqual, length, index, &bumpedLength);
                masm.storeKey(newLength, length);
                masm.bind(&bumpedLength);

                
                masm.bumpKey(&newLength, -1);
                masm.jump(&storeElement);
            }
            
            masm.bind(&markElem);
        }

        if (cx->zone()->needsBarrier())
            masm.callPreBarrier(target, MIRType_Value);

        
        if (guardHoles)
            masm.branchTestMagic(Assembler::Equal, target, &failures);
        else
            masm.bind(&storeElement);
        masm.storeConstantOrRegister(value, target);
    }
    attacher.jumpRejoin(masm);

    
    masm.bind(&outOfBounds);
    masm.bind(&failures);
    attacher.jumpNextStub(masm);

    return true;
}

bool
SetElementIC::attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval)
{
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    if (!GenerateSetDenseElement(cx, masm, attacher, obj, idval,
                                 guardHoles(), object(), index(),
                                 value(), tempToUnboxIndex(),
                                 temp()))
    {
        return false;
    }

    setHasDenseStub();
    const char *message = guardHoles()            ?
                            "dense array (holes)" :
                            "dense array";
    return linkAndAttachStub(cx, masm, attacher, ion, message);
}

static bool
GenerateSetTypedArrayElement(JSContext *cx, MacroAssembler &masm, IonCache::StubAttacher &attacher,
                             TypedArrayObject *tarr, Register object,
                             ValueOperand indexVal, ConstantOrRegister value,
                             Register tempUnbox, Register temp, FloatRegister tempFloat)
{
    Label failures, done, popObjectAndFail;

    
    Shape *shape = tarr->lastProperty();
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object, shape, &failures);

    
    masm.branchTestInt32(Assembler::NotEqual, indexVal, &failures);
    Register index = masm.extractInt32(indexVal, tempUnbox);

    
    Address length(object, TypedArrayObject::lengthOffset());
    masm.unboxInt32(length, temp);
    masm.branch32(Assembler::BelowOrEqual, temp, index, &done);

    
    Register elements = temp;
    masm.loadPtr(Address(object, TypedArrayObject::dataOffset()), elements);

    
    int arrayType = tarr->type();
    int width = TypedArrayObject::slotWidth(arrayType);
    BaseIndex target(elements, index, ScaleFromElemWidth(width));

    if (arrayType == ScalarTypeRepresentation::TYPE_FLOAT32) {
        if (LIRGenerator::allowFloat32Optimizations()) {
            if (!masm.convertConstantOrRegisterToFloat(cx, value, tempFloat, &failures))
                return false;
        } else {
            if (!masm.convertConstantOrRegisterToDouble(cx, value, tempFloat, &failures))
                return false;
        }
        masm.storeToTypedFloatArray(arrayType, tempFloat, target);
    } else if (arrayType == ScalarTypeRepresentation::TYPE_FLOAT64) {
        if (!masm.convertConstantOrRegisterToDouble(cx, value, tempFloat, &failures))
            return false;
        masm.storeToTypedFloatArray(arrayType, tempFloat, target);
    } else {
        
        
        
        masm.push(object);

        if (arrayType == ScalarTypeRepresentation::TYPE_UINT8_CLAMPED) {
            if (!masm.clampConstantOrRegisterToUint8(cx, value, tempFloat, object,
                                                     &popObjectAndFail))
            {
                return false;
            }
        } else {
            if (!masm.truncateConstantOrRegisterToInt32(cx, value, tempFloat, object,
                                                        &popObjectAndFail))
            {
                return false;
            }
        }
        masm.storeToTypedIntArray(arrayType, object, target);

        masm.pop(object);
    }

    
    masm.bind(&done);
    attacher.jumpRejoin(masm);

    if (popObjectAndFail.used()) {
        masm.bind(&popObjectAndFail);
        masm.pop(object);
    }

    masm.bind(&failures);
    attacher.jumpNextStub(masm);
    return true;
}

bool
SetElementIC::attachTypedArrayElement(JSContext *cx, IonScript *ion, TypedArrayObject *tarr)
{
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);
    if (!GenerateSetTypedArrayElement(cx, masm, attacher, tarr,
                                      object(), index(), value(),
                                      tempToUnboxIndex(), temp(), tempFloat()))
    {
        return false;
    }

    return linkAndAttachStub(cx, masm, attacher, ion, "typed array");
}

bool
SetElementIC::update(JSContext *cx, size_t cacheIndex, HandleObject obj,
                     HandleValue idval, HandleValue value)
{
    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    SetElementIC &cache = ion->getCache(cacheIndex).toSetElement();

    bool attachedStub = false;
    if (cache.canAttachStub()) {
        if (!cache.hasDenseStub() && IsDenseElementSetInlineable(obj, idval)) {
            if (!cache.attachDenseElement(cx, ion, obj, idval))
                return false;
            attachedStub = true;
        }
        if (!attachedStub && IsTypedArrayElementSetInlineable(obj, idval, value)) {
            TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
            if (!cache.attachTypedArrayElement(cx, ion, tarr))
                return false;
        }
    }

    if (!SetObjectElement(cx, obj, idval, value, cache.strict()))
        return false;
    return true;
}

void
SetElementIC::reset()
{
    RepatchIonCache::reset();
    hasDenseStub_ = false;
}

bool
SetElementParIC::attachDenseElement(LockedJSContext &cx, IonScript *ion, JSObject *obj,
                                    const Value &idval)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    if (!GenerateSetDenseElement(cx, masm, attacher, obj, idval,
                                 guardHoles(), object(), index(),
                                 value(), tempToUnboxIndex(),
                                 temp()))
    {
        return false;
    }

    const char *message = guardHoles()                     ?
                            "parallel dense array (holes)" :
                            "parallel dense array";

    return linkAndAttachStub(cx, masm, attacher, ion, message);
}

bool
SetElementParIC::attachTypedArrayElement(LockedJSContext &cx, IonScript *ion,
                                         TypedArrayObject *tarr)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    if (!GenerateSetTypedArrayElement(cx, masm, attacher, tarr,
                                      object(), index(), value(),
                                      tempToUnboxIndex(), temp(), tempFloat()))
    {
        return false;
    }

    return linkAndAttachStub(cx, masm, attacher, ion, "parallel typed array");
}

bool
SetElementParIC::update(ForkJoinSlice *slice, size_t cacheIndex, HandleObject obj,
                        HandleValue idval, HandleValue value)
{
    IonScript *ion = GetTopIonJSScript(slice)->parallelIonScript();
    SetElementParIC &cache = ion->getCache(cacheIndex).toSetElementPar();

    
    if (!cache.canAttachStub())
        return SetElementPar(slice, obj, idval, value, cache.strict());

    {
        LockedJSContext cx(slice);

        if (cache.canAttachStub()) {
            bool alreadyStubbed;
            if (!cache.hasOrAddStubbedShape(cx, obj->lastProperty(), &alreadyStubbed))
                return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
            if (alreadyStubbed)
                return SetElementPar(slice, obj, idval, value, cache.strict());

            bool attachedStub = false;
            if (IsDenseElementSetInlineable(obj, idval)) {
                if (!cache.attachDenseElement(cx, ion, obj, idval))
                    return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                attachedStub = true;
            }
            if (!attachedStub && IsTypedArrayElementSetInlineable(obj, idval, value)) {
                TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
                if (!cache.attachTypedArrayElement(cx, ion, tarr))
                    return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
            }
        }
    }

    return SetElementPar(slice, obj, idval, value, cache.strict());
}

bool
GetElementParIC::attachReadSlot(LockedJSContext &cx, IonScript *ion, JSObject *obj,
                                const Value &idval, PropertyName *name, JSObject *holder,
                                Shape *shape)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);

    
    Label failures;
    ValueOperand val = index().reg().valueReg();
    masm.branchTestValue(Assembler::NotEqual, val, idval, &failures);

    GenerateReadSlot(cx, ion, masm, attacher, obj, holder, shape, object(), output(),
                     &failures);

    return linkAndAttachStub(cx, masm, attacher, ion, "parallel getelem reading");
}

bool
GetElementParIC::attachDenseElement(LockedJSContext &cx, IonScript *ion, JSObject *obj,
                                    const Value &idval)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    if (!GenerateDenseElement(cx, masm, attacher, obj, idval, object(), index(), output()))
        return false;

    return linkAndAttachStub(cx, masm, attacher, ion, "parallel dense element");
}

bool
GetElementParIC::attachTypedArrayElement(LockedJSContext &cx, IonScript *ion,
                                         TypedArrayObject *tarr, const Value &idval)
{
    MacroAssembler masm(cx);
    DispatchStubPrepender attacher(*this);
    GenerateGetTypedArrayElement(cx, masm, attacher, tarr, idval, object(), index(), output());
    return linkAndAttachStub(cx, masm, attacher, ion, "parallel typed array");
}

bool
GetElementParIC::update(ForkJoinSlice *slice, size_t cacheIndex, HandleObject obj,
                        HandleValue idval, MutableHandleValue vp)
{
    AutoFlushCache afc("GetElementParCache", slice->runtime()->jitRuntime());

    IonScript *ion = GetTopIonJSScript(slice)->parallelIonScript();
    GetElementParIC &cache = ion->getCache(cacheIndex).toGetElementPar();

    
    
    if (!GetObjectElementOperationPure(slice, obj, idval, vp.address()))
        return false;

    
    if (!cache.canAttachStub())
        return true;

    {
        LockedJSContext cx(slice);

        if (cache.canAttachStub()) {
            bool alreadyStubbed;
            if (!cache.hasOrAddStubbedShape(cx, obj->lastProperty(), &alreadyStubbed))
                return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
            if (alreadyStubbed)
                return true;

            jsid id;
            if (!ValueToIdPure(idval, &id))
                return false;

            bool attachedStub = false;
            if (cache.monitoredResult() &&
                GetElementIC::canAttachGetProp(obj, idval, id))
            {
                RootedShape shape(cx);
                RootedObject holder(cx);
                RootedPropertyName name(cx, JSID_TO_ATOM(id)->asPropertyName());

                GetPropertyIC::NativeGetPropCacheability canCache =
                    CanAttachNativeGetProp(cx, cache, obj, name, &holder, &shape);

                if (canCache == GetPropertyIC::CanAttachReadSlot)
                {
                    if (!cache.attachReadSlot(cx, ion, obj, idval, name, holder, shape))
                        return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                    attachedStub = true;
                }
            }
            if (!attachedStub &&
                GetElementIC::canAttachDenseElement(obj, idval))
            {
                if (!cache.attachDenseElement(cx, ion, obj, idval))
                    return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                attachedStub = true;
            }
            if (!attachedStub &&
                GetElementIC::canAttachTypedArrayElement(obj, idval, cache.output()))
            {
                if (!cache.attachTypedArrayElement(cx, ion, &obj->as<TypedArrayObject>(), idval))
                    return slice->setPendingAbortFatal(ParallelBailoutFailedIC);
                attachedStub = true;
            }
        }
    }

    return true;
}

bool
BindNameIC::attachGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain)
{
    JS_ASSERT(scopeChain->is<GlobalObject>());

    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    
    attacher.branchNextStub(masm, Assembler::NotEqual, scopeChainReg(),
                            ImmGCPtr(scopeChain));
    masm.movePtr(ImmGCPtr(scopeChain), outputReg());

    attacher.jumpRejoin(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "global");
}

static inline void
GenerateScopeChainGuard(MacroAssembler &masm, JSObject *scopeObj,
                        Register scopeObjReg, Shape *shape, Label *failures)
{
    if (scopeObj->is<CallObject>()) {
        
        
        
        CallObject *callObj = &scopeObj->as<CallObject>();
        if (!callObj->isForEval()) {
            JSFunction *fun = &callObj->callee();
            JSScript *script = fun->nonLazyScript();
            if (!script->funHasExtensibleScope())
                return;
        }
    } else if (scopeObj->is<GlobalObject>()) {
        
        
        
        if (shape && !shape->configurable())
            return;
    }

    Address shapeAddr(scopeObjReg, JSObject::offsetOfShape());
    masm.branchPtr(Assembler::NotEqual, shapeAddr, ImmGCPtr(scopeObj->lastProperty()), failures);
}

static void
GenerateScopeChainGuards(MacroAssembler &masm, JSObject *scopeChain, JSObject *holder,
                         Register outputReg, Label *failures, bool skipLastGuard = false)
{
    JSObject *tobj = scopeChain;

    
    
    while (true) {
        JS_ASSERT(IsCacheableNonGlobalScope(tobj) || tobj->is<GlobalObject>());

        if (skipLastGuard && tobj == holder)
            break;

        GenerateScopeChainGuard(masm, tobj, outputReg, nullptr, failures);

        if (tobj == holder)
            break;

        
        tobj = &tobj->as<ScopeObject>().enclosingScope();
        masm.extractObject(Address(outputReg, ScopeObject::offsetOfEnclosingScope()), outputReg);
    }
}

bool
BindNameIC::attachNonGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain, JSObject *holder)
{
    JS_ASSERT(IsCacheableNonGlobalScope(scopeChain));

    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    
    Label failures;
    attacher.branchNextStubOrLabel(masm, Assembler::NotEqual,
                                   Address(scopeChainReg(), JSObject::offsetOfShape()),
                                   ImmGCPtr(scopeChain->lastProperty()),
                                   holder != scopeChain ? &failures : nullptr);

    if (holder != scopeChain) {
        JSObject *parent = &scopeChain->as<ScopeObject>().enclosingScope();
        masm.extractObject(Address(scopeChainReg(), ScopeObject::offsetOfEnclosingScope()), outputReg());

        GenerateScopeChainGuards(masm, parent, holder, outputReg(), &failures);
    } else {
        masm.movePtr(scopeChainReg(), outputReg());
    }

    
    
    attacher.jumpRejoin(masm);

    
    if (holder != scopeChain) {
        masm.bind(&failures);
        attacher.jumpNextStub(masm);
    }

    return linkAndAttachStub(cx, masm, attacher, ion, "non-global");
}

static bool
IsCacheableScopeChain(JSObject *scopeChain, JSObject *holder)
{
    while (true) {
        if (!IsCacheableNonGlobalScope(scopeChain)) {
            IonSpew(IonSpew_InlineCaches, "Non-cacheable object on scope chain");
            return false;
        }

        if (scopeChain == holder)
            return true;

        scopeChain = &scopeChain->as<ScopeObject>().enclosingScope();
        if (!scopeChain) {
            IonSpew(IonSpew_InlineCaches, "Scope chain indirect hit");
            return false;
        }
    }

    MOZ_ASSUME_UNREACHABLE("Invalid scope chain");
}

JSObject *
BindNameIC::update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain)
{
    AutoFlushCache afc ("BindNameCache", cx->runtime()->jitRuntime());

    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    BindNameIC &cache = ion->getCache(cacheIndex).toBindName();
    HandlePropertyName name = cache.name();

    RootedObject holder(cx);
    if (scopeChain->is<GlobalObject>()) {
        holder = scopeChain;
    } else {
        if (!LookupNameWithGlobalDefault(cx, name, scopeChain, &holder))
            return nullptr;
    }

    
    
    if (cache.canAttachStub()) {
        if (scopeChain->is<GlobalObject>()) {
            if (!cache.attachGlobal(cx, ion, scopeChain))
                return nullptr;
        } else if (IsCacheableScopeChain(scopeChain, holder)) {
            if (!cache.attachNonGlobal(cx, ion, scopeChain, holder))
                return nullptr;
        } else {
            IonSpew(IonSpew_InlineCaches, "BINDNAME uncacheable scope chain");
        }
    }

    return holder;
}

bool
NameIC::attachReadSlot(JSContext *cx, IonScript *ion, HandleObject scopeChain,
                       HandleObject holderBase, HandleObject holder,
                       HandleShape shape)
{
    MacroAssembler masm(cx);
    Label failures;
    RepatchStubAppender attacher(*this);

    Register scratchReg = outputReg().valueReg().scratchReg();

    
    
    masm.mov(scopeChainReg(), scratchReg);
    GenerateScopeChainGuards(masm, scopeChain, holderBase, scratchReg, &failures,
                             true);

    
    
    GenerateReadSlot(cx, ion, masm, attacher, holderBase, holder, shape, scratchReg,
                     outputReg(), failures.used() ? &failures : nullptr);

    return linkAndAttachStub(cx, masm, attacher, ion, "generic");
}

static bool
IsCacheableNameReadSlot(JSContext *cx, HandleObject scopeChain, HandleObject obj,
                        HandleObject holder, HandleShape shape, jsbytecode *pc,
                        const TypedOrValueRegister &output)
{
    if (!shape)
        return false;
    if (!obj->isNative())
        return false;

    if (obj->is<GlobalObject>()) {
        
        if (!IsCacheableGetPropReadSlot(obj, holder, shape) &&
            !IsCacheableNoProperty(obj, holder, shape, pc, output))
            return false;
    } else if (obj->is<CallObject>()) {
        JS_ASSERT(obj == holder);
        if (!shape->hasDefaultGetter())
            return false;
    } else {
        
        return false;
    }

    RootedObject obj2(cx, scopeChain);
    while (obj2) {
        if (!IsCacheableNonGlobalScope(obj2) && !obj2->is<GlobalObject>())
            return false;

        
        if (obj2->is<GlobalObject>() || obj2 == obj)
            break;

        obj2 = obj2->enclosingScope();
    }

    return obj == obj2;
}

bool
NameIC::attachCallGetter(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                         HandleShape shape, void *returnAddr)
{
    MacroAssembler masm(cx);

    
    
    masm.setFramePushed(ion->frameSize());

    RepatchStubAppender attacher(*this);
    if (!GenerateCallGetter(cx, ion, masm, attacher, obj, name(), holder, shape, liveRegs_,
                            scopeChainReg(), outputReg(), returnAddr))
    {
         return false;
    }

    const char *attachKind = "name getter";
    return linkAndAttachStub(cx, masm, attacher, ion, attachKind);
}

static bool
IsCacheableNameCallGetter(JSObject *scopeChain, JSObject *obj, JSObject *holder, Shape *shape)
{
    if (obj != scopeChain)
        return false;

    if (!obj->is<GlobalObject>())
        return false;

    return IsCacheableGetPropCallNative(obj, holder, shape) ||
        IsCacheableGetPropCallPropertyOp(obj, holder, shape);
}

bool
NameIC::update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain,
               MutableHandleValue vp)
{
    AutoFlushCache afc ("GetNameCache", cx->runtime()->jitRuntime());

    void *returnAddr;
    IonScript *ion = GetTopIonJSScript(cx, &returnAddr)->ionScript();

    NameIC &cache = ion->getCache(cacheIndex).toName();
    RootedPropertyName name(cx, cache.name());

    RootedScript script(cx);
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    RootedObject obj(cx);
    RootedObject holder(cx);
    RootedShape shape(cx);
    if (!LookupName(cx, name, scopeChain, &obj, &holder, &shape))
        return false;

    if (cache.canAttachStub()) {
        if (IsCacheableNameReadSlot(cx, scopeChain, obj, holder, shape, pc, cache.outputReg())) {
            if (!cache.attachReadSlot(cx, ion, scopeChain, obj, holder, shape))
                return false;
        } else if (IsCacheableNameCallGetter(scopeChain, obj, holder, shape)) {
            if (!cache.attachCallGetter(cx, ion, obj, holder, shape, returnAddr))
                return false;
        }
    }

    if (cache.isTypeOf()) {
        if (!FetchName<true>(cx, obj, holder, name, shape, vp))
            return false;
    } else {
        if (!FetchName<false>(cx, obj, holder, name, shape, vp))
            return false;
    }

    
    types::TypeScript::Monitor(cx, script, pc, vp);

    return true;
}

bool
CallsiteCloneIC::attach(JSContext *cx, IonScript *ion, HandleFunction original,
                        HandleFunction clone)
{
    MacroAssembler masm(cx);
    RepatchStubAppender attacher(*this);

    
    attacher.branchNextStub(masm, Assembler::NotEqual, calleeReg(), ImmGCPtr(original));

    
    masm.movePtr(ImmGCPtr(clone), outputReg());

    attacher.jumpRejoin(masm);

    return linkAndAttachStub(cx, masm, attacher, ion, "generic");
}

JSObject *
CallsiteCloneIC::update(JSContext *cx, size_t cacheIndex, HandleObject callee)
{
    AutoFlushCache afc ("CallsiteCloneCache", cx->runtime()->jitRuntime());

    
    
    RootedFunction fun(cx, &callee->as<JSFunction>());
    if (!fun->hasScript() || !fun->nonLazyScript()->shouldCloneAtCallsite())
        return fun;

    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    CallsiteCloneIC &cache = ion->getCache(cacheIndex).toCallsiteClone();

    RootedFunction clone(cx, CloneFunctionAtCallsite(cx, fun, cache.callScript(), cache.callPc()));
    if (!clone)
        return nullptr;

    if (cache.canAttachStub()) {
        if (!cache.attach(cx, ion, fun, clone))
            return nullptr;
    }

    return clone;
}

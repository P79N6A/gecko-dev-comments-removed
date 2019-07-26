






#include "mozilla/DebugOnly.h"

#include "CodeGenerator.h"
#include "Ion.h"
#include "IonCaches.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"

#include "vm/Shape.h"

#include "jsinterpinlines.h"

#include "IonFrames-inl.h"

using namespace js;
using namespace js::ion;

using mozilla::DebugOnly;

void
CodeLocationJump::repoint(IonCode *code, MacroAssembler *masm)
{
    JS_ASSERT(!absolute_);
    size_t new_off = (size_t)raw_;
#ifdef JS_SMALL_BRANCH
    size_t jumpTableEntryOffset = reinterpret_cast<size_t>(jumpTableEntry_);
#endif
    if (masm != NULL) {
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
     JS_ASSERT(!absolute_);
     size_t new_off = (size_t)raw_;
     if (masm != NULL) {
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
    static const char *names[] =
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
    AssertCanGC();
    Linker linker(masm);
    *code = linker.newCode(cx);
    if (!code)
        return LINK_ERROR;

    if (ion->invalidated())
        return CACHE_FLUSHED;

    return LINK_GOOD;
}

const size_t IonCache::MAX_STUBS = 16;






const ImmWord STUB_ADDR = ImmWord(uintptr_t(0xdeadc0de));

void
IonCache::attachStub(MacroAssembler &masm, IonCode *code, CodeOffsetJump &rejoinOffset,
                     CodeOffsetJump *exitOffset, CodeOffsetLabel *stubLabel)
{
    JS_ASSERT(canAttachStub());
    incrementStubCount();

    rejoinOffset.fixup(&masm);
    CodeLocationJump rejoinJump(code, rejoinOffset);

    
    PatchJump(rejoinJump, rejoinLabel());

    
    
    PatchJump(lastJump_, CodeLocationLabel(code));

    
    
    if (exitOffset) {
        exitOffset->fixup(&masm);
        CodeLocationJump exitJump(code, *exitOffset);

        
        
        PatchJump(exitJump, fallbackLabel());

        
        
        lastJump_ = exitJump;
    }

    
    
    
    if (stubLabel) {
        stubLabel->fixup(&masm);
        Assembler::patchDataWithValueCheck(CodeLocationLabel(code, *stubLabel),
                                           ImmWord(uintptr_t(code)), STUB_ADDR);
    }
}

bool
IonCache::linkAndAttachStub(JSContext *cx, MacroAssembler &masm, IonScript *ion,
                            const char *attachKind, CodeOffsetJump &rejoinOffset,
                            CodeOffsetJump *exitOffset, CodeOffsetLabel *stubLabel)
{
    IonCode *code = NULL;
    LinkStatus status = linkCode(cx, masm, ion, &code);
    if (status != LINK_GOOD)
        return status != LINK_ERROR;

    attachStub(masm, code, rejoinOffset, exitOffset, stubLabel);

    IonSpew(IonSpew_InlineCaches, "Generated %s %s stub at %p",
            attachKind, CacheName(kind()), code->raw());
    return true;
}

static bool
IsCacheableListBase(JSObject *obj)
{
    if (!obj->isProxy())
        return false;

    BaseProxyHandler *handler = GetProxyHandler(obj);

    if (handler->family() != GetListBaseHandlerFamily())
        return false;

    if (obj->numFixedSlots() <= GetListBaseExpandoSlot())
        return false;

    return true;
}

static void
GeneratePrototypeGuards(JSContext *cx, MacroAssembler &masm, JSObject *obj, JSObject *holder,
                        Register objectReg, Register scratchReg, Label *failures)
{
    JS_ASSERT(obj != holder);

    if (obj->hasUncacheableProto()) {
        
        
        masm.loadPtr(Address(objectReg, JSObject::offsetOfType()), scratchReg);
        Address proto(scratchReg, offsetof(types::TypeObject, proto));
        masm.branchPtr(Assembler::NotEqual, proto, ImmGCPtr(obj->getProto()), failures);
    }

    JSObject *pobj = IsCacheableListBase(obj)
                     ? obj->getTaggedProto().toObjectOrNull()
                     : obj->getProto();
    if (!pobj)
        return;
    while (pobj != holder) {
        if (pobj->hasUncacheableProto()) {
            JS_ASSERT(!pobj->hasSingletonType());
            masm.movePtr(ImmGCPtr(pobj), scratchReg);
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
        




        JSObject *proto = IsCacheableListBase(obj)
                     ? obj->getTaggedProto().toObjectOrNull()
                     : obj->getProto();
        if (!proto || !proto->isNative())
            return false;
        obj = proto;
    }
    return true;
}

static bool
IsCacheableGetPropReadSlot(JSObject *obj, JSObject *holder, UnrootedShape shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (!shape->hasSlot() || !shape->hasDefaultGetter())
        return false;

    return true;
}

static bool
IsCacheableNoProperty(JSObject *obj, JSObject *holder, UnrootedShape shape, jsbytecode *pc,
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
IsCacheableGetPropCallNative(JSObject *obj, JSObject *holder, UnrootedShape shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (!shape->hasGetterValue() || !shape->getterValue().isObject())
        return false;

    return shape->getterValue().toObject().isFunction() &&
           shape->getterValue().toObject().toFunction()->isNative();
}

static bool
IsCacheableGetPropCallPropertyOp(JSObject *obj, JSObject *holder, UnrootedShape shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (shape->hasSlot() || shape->hasGetterValue() || shape->hasDefaultGetter())
        return false;

    return true;
}

struct GetNativePropertyStub
{
    CodeOffsetJump exitOffset;
    CodeOffsetJump rejoinOffset;
    CodeOffsetLabel stubCodePatchOffset;

    void generateReadSlot(JSContext *cx, MacroAssembler &masm, JSObject *obj, PropertyName *propName,
                          JSObject *holder, HandleShape shape, Register object, TypedOrValueRegister output,
                          RepatchLabel *failures, Label *nonRepatchFailures = NULL)
    {
        
        
        
        bool multipleFailureJumps = (nonRepatchFailures != NULL) && nonRepatchFailures->used();
        exitOffset = masm.branchPtrWithPatch(Assembler::NotEqual,
                                             Address(object, JSObject::offsetOfShape()),
                                             ImmGCPtr(obj->lastProperty()),
                                             failures);

        bool restoreScratch = false;
        Register scratchReg = Register::FromCode(0); 

        
        
        
        Label prototypeFailures;
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

        
        Register holderReg;
        if (obj != holder) {
            
            GeneratePrototypeGuards(cx, masm, obj, holder, object, scratchReg, &prototypeFailures);

            if (holder) {
                
                holderReg = scratchReg;
                masm.movePtr(ImmGCPtr(holder), holderReg);
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

        
        if (holder && holder->isFixedSlot(shape->slot())) {
            Address addr(holderReg, JSObject::getFixedSlotOffset(shape->slot()));
            masm.loadTypedOrValue(addr, output);
        } else if (holder) {
            masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), scratchReg);

            Address addr(scratchReg, holder->dynamicSlotIndex(shape->slot()) * sizeof(Value));
            masm.loadTypedOrValue(addr, output);
        } else {
            JS_ASSERT(!holder);
            masm.moveValue(UndefinedValue(), output.valueReg());
        }

        if (restoreScratch)
            masm.pop(scratchReg);

        RepatchLabel rejoin_;
        rejoinOffset = masm.jumpWithPatch(&rejoin_);
        masm.bind(&rejoin_);

        if (obj != holder || multipleFailureJumps) {
            masm.bind(&prototypeFailures);
            if (restoreScratch)
                masm.pop(scratchReg);
            masm.bind(failures);
            if (multipleFailureJumps)
                masm.bind(nonRepatchFailures);
            RepatchLabel exit_;
            exitOffset = masm.jumpWithPatch(&exit_);
            masm.bind(&exit_);
        } else {
            masm.bind(failures);
        }
    }

    bool generateCallGetter(JSContext *cx, MacroAssembler &masm, JSObject *obj,
                            PropertyName *propName, JSObject *holder, HandleShape shape,
                            RegisterSet &liveRegs, Register object, TypedOrValueRegister output,
                            void *returnAddr, jsbytecode *pc,
                            RepatchLabel *failures, Label *nonRepatchFailures = NULL)
    {
        
        Label stubFailure;
        masm.branchPtr(Assembler::NotEqual, Address(object, JSObject::offsetOfShape()),
                       ImmGCPtr(obj->lastProperty()), &stubFailure);

        
        
        
        
        if (IsCacheableListBase(obj)) {
            Address handlerAddr(object, JSObject::getFixedSlotOffset(JSSLOT_PROXY_HANDLER));
            Address expandoAddr(object, JSObject::getFixedSlotOffset(GetListBaseExpandoSlot()));

            
            masm.branchPrivatePtr(Assembler::NotEqual, handlerAddr, ImmWord(GetProxyHandler(obj)), &stubFailure);

            
            
            RegisterSet listBaseRegSet(RegisterSet::All());
            listBaseRegSet.take(AnyRegister(object));
            ValueOperand tempVal = listBaseRegSet.takeValueOperand();
            masm.pushValue(tempVal);

            Label failListBaseCheck;
            Label listBaseOk;

            Value expandoVal = obj->getFixedSlot(GetListBaseExpandoSlot());
            JSObject *expando = expandoVal.isObject() ? &(expandoVal.toObject()) : NULL;
            JS_ASSERT_IF(expando, expando->isNative() && expando->getProto() == NULL);

            masm.loadValue(expandoAddr, tempVal);
            if (expando && expando->nativeLookup(cx, propName)) {
                
                

                
                
                

                masm.branchTestObject(Assembler::NotEqual, tempVal, &failListBaseCheck);
                masm.extractObject(tempVal, tempVal.scratchReg());
                masm.branchPtr(Assembler::Equal,
                               Address(tempVal.scratchReg(), JSObject::offsetOfShape()),
                               ImmGCPtr(expando->lastProperty()),
                               &listBaseOk);
            } else {
                
                
                masm.branchTestUndefined(Assembler::Equal, tempVal, &listBaseOk);
            }

            
            masm.bind(&failListBaseCheck);
            masm.popValue(tempVal);
            masm.jump(&stubFailure);

            
            masm.bind(&listBaseOk);
            masm.popValue(tempVal);
        }

        
        bool restoreScratch = false;
        Register scratchReg = Register::FromCode(0); 

        
        
        
        Label prototypeFailures;
        JS_ASSERT(output.hasValue());
        scratchReg = output.valueReg().scratchReg();

        
        if (obj != holder)
            GeneratePrototypeGuards(cx, masm, obj, holder, object, scratchReg, &prototypeFailures);

        
        Register holderReg = scratchReg;
        masm.movePtr(ImmGCPtr(holder), holderReg);
        masm.branchPtr(Assembler::NotEqual,
                       Address(holderReg, JSObject::offsetOfShape()),
                       ImmGCPtr(holder->lastProperty()),
                       &prototypeFailures);

        if (restoreScratch)
            masm.pop(scratchReg);

        

        
        masm.PushRegsInMask(liveRegs);

        
        
        RegisterSet regSet(RegisterSet::All());
        regSet.take(AnyRegister(object));

        
        
        
        
        scratchReg               = regSet.takeGeneral();
        Register argJSContextReg = regSet.takeGeneral();
        Register argUintNReg     = regSet.takeGeneral();
        Register argVpReg        = regSet.takeGeneral();

        
        bool callNative = IsCacheableGetPropCallNative(obj, holder, shape);
        JS_ASSERT_IF(!callNative, IsCacheableGetPropCallPropertyOp(obj, holder, shape));

        
        DebugOnly<uint32_t> initialStack = masm.framePushed();

        Label success, exception;

        
        
        
        
        
        
        
        
        
        stubCodePatchOffset = masm.PushWithPatch(STUB_ADDR);

        if (callNative) {
            JS_ASSERT(shape->hasGetterValue() && shape->getterValue().isObject() &&
                      shape->getterValue().toObject().isFunction());
            JSFunction *target = shape->getterValue().toObject().toFunction();

            JS_ASSERT(target);
            JS_ASSERT(target->isNative());

            
            
            
            

            
            
            masm.Push(TypedOrValueRegister(MIRType_Object, AnyRegister(object)));
            
            masm.Push(ObjectValue(*target));

            
            masm.loadJSContext(argJSContextReg);
            masm.move32(Imm32(0), argUintNReg);
            masm.movePtr(StackPointer, argVpReg);

            if (!masm.buildOOLFakeExitFrame(returnAddr))
                return false;
            masm.enterFakeExitFrame(ION_FRAME_OOL_NATIVE_GETTER);

            
            masm.setupUnalignedABICall(3, scratchReg);
            masm.passABIArg(argJSContextReg);
            masm.passABIArg(argUintNReg);
            masm.passABIArg(argVpReg);
            masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));

            
            masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

            
            masm.loadValue(
                Address(StackPointer, IonOOLNativeGetterExitFrameLayout::offsetOfResult()),
                JSReturnOperand);
        } else {
            Register argObjReg       = argUintNReg;
            Register argIdReg        = regSet.takeGeneral();

            PropertyOp target = shape->getterOp();
            JS_ASSERT(target);
            

            
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

            if (!masm.buildOOLFakeExitFrame(returnAddr))
                return false;
            masm.enterFakeExitFrame(ION_FRAME_OOL_PROPERTY_OP);

            
            masm.setupUnalignedABICall(4, scratchReg);
            masm.passABIArg(argJSContextReg);
            masm.passABIArg(argObjReg);
            masm.passABIArg(argIdReg);
            masm.passABIArg(argVpReg);
            masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target));

            
            masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

            
            masm.loadValue(
                Address(StackPointer, IonOOLPropertyOpExitFrameLayout::offsetOfResult()),
                JSReturnOperand);
        }

        
        
        masm.jump(&success);

        
        masm.bind(&exception);
        masm.handleException();

        
        masm.bind(&success);
        masm.storeCallResultValue(output);

        
        

        
        if (callNative)
            masm.adjustStack(IonOOLNativeGetterExitFrameLayout::Size());
        else
            masm.adjustStack(IonOOLPropertyOpExitFrameLayout::Size());
        JS_ASSERT(masm.framePushed() == initialStack);

        
        masm.PopRegsInMask(liveRegs);

        
        RepatchLabel rejoin_;
        rejoinOffset = masm.jumpWithPatch(&rejoin_);
        masm.bind(&rejoin_);

        
        masm.bind(&prototypeFailures);
        if (restoreScratch)
            masm.pop(scratchReg);
        masm.bind(&stubFailure);
        if (nonRepatchFailures)
            masm.bind(nonRepatchFailures);
        RepatchLabel exit_;
        exitOffset = masm.jumpWithPatch(&exit_);
        masm.bind(&exit_);

        return true;
    }
};

bool
GetPropertyIC::attachReadSlot(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                              HandleShape shape)
{
    MacroAssembler masm;
    RepatchLabel failures;

    GetNativePropertyStub getprop;
    getprop.generateReadSlot(cx, masm, obj, name(), holder, shape, object(), output(), &failures);

    const char *attachKind = "non idempotent reading";
    if (idempotent())
        attachKind = "idempotent reading";
    return linkAndAttachStub(cx, masm, ion, attachKind, getprop.rejoinOffset, &getprop.exitOffset);
}

bool
GetPropertyIC::attachCallGetter(JSContext *cx, IonScript *ion, JSObject *obj,
                                JSObject *holder, HandleShape shape,
                                const SafepointIndex *safepointIndex, void *returnAddr)
{
    MacroAssembler masm;
    RepatchLabel failures;

    JS_ASSERT(!idempotent());
    JS_ASSERT(allowGetters());

    
    
    masm.setFramePushed(ion->frameSize());

    GetNativePropertyStub getprop;
    if (!getprop.generateCallGetter(cx, masm, obj, name(), holder, shape, liveRegs_,
                                    object(), output(), returnAddr, pc, &failures))
    {
         return false;
    }

    const char *attachKind = "non idempotent calling";
    if (idempotent())
        attachKind = "idempotent calling";
    return linkAndAttachStub(cx, masm, ion, attachKind, getprop.rejoinOffset, &getprop.exitOffset,
                             &getprop.stubCodePatchOffset);
}

bool
GetPropertyIC::attachArrayLength(JSContext *cx, IonScript *ion, JSObject *obj)
{
    JS_ASSERT(obj->isArray());
    JS_ASSERT(!idempotent());

    Label failures;
    MacroAssembler masm;

    
    RootedObject globalObj(cx, &script->global());
    RootedShape shape(cx, obj->lastProperty());
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object(), shape, &failures);

    
    Register outReg;
    if (output().hasValue()) {
        outReg = output().valueReg().scratchReg();
    } else {
        JS_ASSERT(output().type() == MIRType_Int32);
        outReg = output().typedReg().gpr();
    }

    masm.loadPtr(Address(object(), JSObject::offsetOfElements()), outReg);
    masm.load32(Address(outReg, ObjectElements::offsetOfLength()), outReg);

    
    JS_ASSERT(object() != outReg);
    masm.branchTest32(Assembler::Signed, outReg, outReg, &failures);

    if (output().hasValue())
        masm.tagValue(JSVAL_TYPE_INT32, outReg, output().valueReg());

    
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&failures);
    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    JS_ASSERT(!hasArrayLengthStub_);
    hasArrayLengthStub_ = true;
    return linkAndAttachStub(cx, masm, ion, "array length", rejoinOffset, &exitOffset);
}

bool
GetPropertyIC::attachTypedArrayLength(JSContext *cx, IonScript *ion, JSObject *obj)
{
    JS_ASSERT(obj->isTypedArray());
    JS_ASSERT(!idempotent());

    Label failures;
    MacroAssembler masm;

    Register tmpReg;
    if (output().hasValue()) {
        tmpReg = output().valueReg().scratchReg();
    } else {
        JS_ASSERT(output().type() == MIRType_Int32);
        tmpReg = output().typedReg().gpr();
    }
    JS_ASSERT(object() != tmpReg);

    
    masm.loadObjClass(object(), tmpReg);
    masm.branchPtr(Assembler::Below, tmpReg, ImmWord(&TypedArray::classes[0]), &failures);
    masm.branchPtr(Assembler::AboveOrEqual, tmpReg, ImmWord(&TypedArray::classes[TypedArray::TYPE_MAX]), &failures);

    
    masm.loadTypedOrValue(Address(object(), TypedArray::lengthOffset()), output());

    
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&failures);
    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    JS_ASSERT(!hasTypedArrayLengthStub_);
    hasTypedArrayLengthStub_ = true;
    return linkAndAttachStub(cx, masm, ion, "typed array length", rejoinOffset, &exitOffset);
}

static bool
TryAttachNativeGetPropStub(JSContext *cx, IonScript *ion,
                           GetPropertyIC &cache, HandleObject obj,
                           HandlePropertyName name,
                           const SafepointIndex *safepointIndex,
                           void *returnAddr, bool *isCacheable)
{
    JS_ASSERT(!*isCacheable);

    RootedObject checkObj(cx, obj);
    bool isListBase = IsCacheableListBase(obj);
    if (isListBase)
        checkObj = obj->getTaggedProto().toObjectOrNull();

    if (!checkObj || !checkObj->isNative())
        return true;

    
    
    
    if (cache.idempotent() && !checkObj->hasIdempotentProtoChain())
        return true;

    RootedShape shape(cx);
    RootedObject holder(cx);
    if (!JSObject::lookupProperty(cx, checkObj, name, &holder, &shape))
        return false;

    
    
    bool readSlot = false;
    bool callGetter = false;

    RootedScript script(cx);
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    if (IsCacheableGetPropReadSlot(obj, holder, shape) ||
        IsCacheableNoProperty(obj, holder, shape, pc, cache.output()))
    {
        
        
        if (!obj->isProxy())
            readSlot = true;
    } else if (IsCacheableGetPropCallNative(checkObj, holder, shape) ||
               IsCacheableGetPropCallPropertyOp(checkObj, holder, shape))
    {
        
        
        if (!cache.idempotent() && cache.allowGetters())
            callGetter = true;
    }

    
    if (!readSlot && !callGetter)
        return true;

    
    
    
    
    
    if (cache.idempotent() &&
        holder &&
        holder->hasSingletonType() &&
        holder->getSlot(shape->slot()).isUndefined())
    {
        return true;
    }

    *isCacheable = true;

    
    JS_ASSERT_IF(readSlot, !callGetter);
    JS_ASSERT_IF(callGetter, !readSlot);

    
    if (!cache.canAttachStub())
        return true;

    if (readSlot)
        return cache.attachReadSlot(cx, ion, obj, holder, shape);
    else if (obj->isArray() && !cache.hasArrayLengthStub() && cx->names().length == name)
        return cache.attachArrayLength(cx, ion, obj);
    return cache.attachCallGetter(cx, ion, obj, holder, shape, safepointIndex, returnAddr);
}

bool
GetPropertyIC::update(JSContext *cx, size_t cacheIndex,
                      HandleObject obj, MutableHandleValue vp)
{
    AutoFlushCache afc ("GetPropertyCache");
    const SafepointIndex *safepointIndex;
    void *returnAddr;
    RootedScript topScript(cx, GetTopIonJSScript(cx, &safepointIndex, &returnAddr));
    IonScript *ion = topScript->ionScript();

    GetPropertyIC &cache = ion->getCache(cacheIndex).toGetProperty();
    RootedPropertyName name(cx, cache.name());

    
    AutoDetectInvalidation adi(cx, vp.address(), ion);

    
    if (cache.idempotent())
        adi.disable();

    
    
    
    bool isCacheable = false;
    if (!TryAttachNativeGetPropStub(cx, ion, cache, obj, name,
                                    safepointIndex, returnAddr,
                                    &isCacheable))
    {
        return false;
    }

    if (!isCacheable && cache.canAttachStub() &&
        !cache.idempotent() && cx->names().length == name)
    {
        if (cache.output().type() != MIRType_Value && cache.output().type() != MIRType_Int32) {
            
            
            isCacheable = false;
        } else if (obj->isTypedArray() && !cache.hasTypedArrayLengthStub()) {
            isCacheable = true;
            if (!cache.attachTypedArrayLength(cx, ion, obj))
                return false;
        }
    }

    if (cache.idempotent() && !isCacheable) {
        
        
        
        
        
        
        IonSpew(IonSpew_InlineCaches, "Invalidating from idempotent cache %s:%d",
                topScript->filename, topScript->lineno);

        topScript->invalidatedIdempotentCache = true;

        
        if (!topScript->hasIonScript())
            return true;

        return Invalidate(cx, topScript);
    }

    RootedId id(cx, NameToId(name));
    if (obj->getOps()->getProperty) {
        if (!JSObject::getGeneric(cx, obj, obj, id, vp))
            return false;
    } else {
        if (!GetPropertyHelper(cx, obj, id, 0, vp))
            return false;
    }

    if (!cache.idempotent()) {
        RootedScript script(cx);
        jsbytecode *pc;
        cache.getScriptedLocation(&script, &pc);

        
        

#if JS_HAS_NO_SUCH_METHOD
        
        if (JSOp(*pc) == JSOP_CALLPROP && JS_UNLIKELY(vp.isPrimitive())) {
            if (!OnUnknownMethod(cx, obj, IdToValue(id), vp))
                return false;
        }
#endif

        
        types::TypeScript::Monitor(cx, script, pc, vp);
    }

    return true;
}

void
IonCache::updateBaseAddress(IonCode *code, MacroAssembler &masm)
{
    initialJump_.repoint(code, &masm);
    lastJump_.repoint(code, &masm);
    fallbackLabel_.repoint(code, &masm);
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
    
    
    PatchJump(initialJump_, fallbackLabel_);

    this->stubCount_ = 0;
    this->lastJump_ = initialJump_;
}

bool
SetPropertyIC::attachNativeExisting(JSContext *cx, IonScript *ion,
                                    HandleObject obj, HandleShape shape)
{
    MacroAssembler masm;

    RepatchLabel exit_;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(object(), JSObject::offsetOfShape()),
                                ImmGCPtr(obj->lastProperty()),
                                &exit_);
    masm.bind(&exit_);

    if (obj->isFixedSlot(shape->slot())) {
        Address addr(object(), JSObject::getFixedSlotOffset(shape->slot()));

        if (cx->zone()->needsBarrier())
            masm.callPreBarrier(addr, MIRType_Value);

        masm.storeConstantOrRegister(value(), addr);
    } else {
        Register slotsReg = object();
        masm.loadPtr(Address(object(), JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(shape->slot()) * sizeof(Value));

        if (cx->zone()->needsBarrier())
            masm.callPreBarrier(addr, MIRType_Value);

        masm.storeConstantOrRegister(value(), addr);
    }

    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    return linkAndAttachStub(cx, masm, ion, "setting", rejoinOffset, &exitOffset);
}

bool
SetPropertyIC::attachSetterCall(JSContext *cx, IonScript *ion,
                                HandleObject obj, HandleObject holder, HandleShape shape,
                                void *returnAddr)
{
    MacroAssembler masm;

    
    
    masm.setFramePushed(ion->frameSize());

    Label failure;
    masm.branchPtr(Assembler::NotEqual,
                   Address(object(), JSObject::offsetOfShape()),
                   ImmGCPtr(obj->lastProperty()),
                   &failure);

    
    
    {
        RegisterSet regSet(RegisterSet::All());
        regSet.take(AnyRegister(object()));
        if (!value().constant())
            regSet.maybeTake(value().reg());
        Register scratchReg = regSet.takeGeneral();
        masm.push(scratchReg);

        Label protoFailure;
        Label protoSuccess;

        
        if (obj != holder)
            GeneratePrototypeGuards(cx, masm, obj, holder, object(), scratchReg, &protoFailure);

        masm.movePtr(ImmGCPtr(holder), scratchReg);
        masm.branchPtr(Assembler::NotEqual,
                       Address(scratchReg, JSObject::offsetOfShape()),
                       ImmGCPtr(holder->lastProperty()),
                       &protoFailure);

        masm.jump(&protoSuccess);

        masm.bind(&protoFailure);
        masm.pop(scratchReg);
        masm.jump(&failure);

        masm.bind(&protoSuccess);
        masm.pop(scratchReg);
    }

    

    
    masm.PushRegsInMask(liveRegs_);

    
    
    RegisterSet regSet(RegisterSet::All());
    regSet.take(AnyRegister(object()));

    
    
    
    
    Register scratchReg     = regSet.takeGeneral();
    Register argJSContextReg = regSet.takeGeneral();
    Register argObjReg       = regSet.takeGeneral();
    Register argIdReg        = regSet.takeGeneral();
    Register argStrictReg    = regSet.takeGeneral();
    Register argVpReg        = regSet.takeGeneral();

    
    DebugOnly<uint32_t> initialStack = masm.framePushed();

    Label success, exception;

    
    
    
    
    
    
    
    
    
    CodeOffsetLabel stubCodePatchOffset = masm.PushWithPatch(STUB_ADDR);

    StrictPropertyOp target = shape->setterOp();
    JS_ASSERT(target);
    
    

    
    if (value().constant())
        masm.Push(value().value());
    else
        masm.Push(value().reg());
    masm.movePtr(StackPointer, argVpReg);

    masm.move32(Imm32(strict() ? 1 : 0), argStrictReg);

    
    RootedId propId(cx);
    if (!shape->getUserId(cx, &propId))
        return false;
    masm.Push(propId, argIdReg);
    masm.movePtr(StackPointer, argIdReg);

    masm.Push(object());
    masm.movePtr(StackPointer, argObjReg);

    masm.loadJSContext(argJSContextReg);

    if (!masm.buildOOLFakeExitFrame(returnAddr))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_OOL_PROPERTY_OP);

    
    masm.setupUnalignedABICall(5, scratchReg);
    masm.passABIArg(argJSContextReg);
    masm.passABIArg(argObjReg);
    masm.passABIArg(argIdReg);
    masm.passABIArg(argStrictReg);
    masm.passABIArg(argVpReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target));

    
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

    masm.jump(&success);

    
    masm.bind(&exception);
    masm.handleException();

    
    masm.bind(&success);

    
    

    
    masm.adjustStack(IonOOLPropertyOpExitFrameLayout::Size());
    JS_ASSERT(masm.framePushed() == initialStack);

    
    masm.PopRegsInMask(liveRegs_);

    
    RepatchLabel rejoin;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin);
    masm.bind(&rejoin);

    
    masm.bind(&failure);
    RepatchLabel exit;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit);
    masm.bind(&exit);

    return linkAndAttachStub(cx, masm, ion, "calling", rejoinOffset, &exitOffset,
                             &stubCodePatchOffset);
}

bool
SetPropertyIC::attachNativeAdding(JSContext *cx, IonScript *ion, JSObject *obj,
                                  HandleShape oldShape, HandleShape newShape,
                                  HandleShape propShape)
{
    MacroAssembler masm;

    Label failures;

    
    masm.branchPtr(Assembler::NotEqual, Address(object(), JSObject::offsetOfType()),
                   ImmGCPtr(obj->type()), &failures);

    
    masm.branchTestObjShape(Assembler::NotEqual, object(), oldShape, &failures);

    Label protoFailures;
    masm.push(object());    

    JSObject *proto = obj->getProto();
    Register protoReg = object();
    while (proto) {
        UnrootedShape protoShape = proto->lastProperty();

        
        masm.loadPtr(Address(protoReg, JSObject::offsetOfType()), protoReg);
        masm.loadPtr(Address(protoReg, offsetof(types::TypeObject, proto)), protoReg);

        
        masm.branchTestPtr(Assembler::Zero, protoReg, protoReg, &protoFailures);
        masm.branchTestObjShape(Assembler::NotEqual, protoReg, protoShape, &protoFailures);

        proto = proto->getProto();
    }

    masm.pop(object());     

    
    Address shapeAddr(object(), JSObject::offsetOfShape());
    if (cx->zone()->needsBarrier())
        masm.callPreBarrier(shapeAddr, MIRType_Shape);
    masm.storePtr(ImmGCPtr(newShape), shapeAddr);

    
    if (obj->isFixedSlot(propShape->slot())) {
        Address addr(object(), JSObject::getFixedSlotOffset(propShape->slot()));
        masm.storeConstantOrRegister(value(), addr);
    } else {
        Register slotsReg = object();

        masm.loadPtr(Address(object(), JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(propShape->slot()) * sizeof(Value));
        masm.storeConstantOrRegister(value(), addr);
    }

    
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&protoFailures);
    masm.pop(object());
    masm.bind(&failures);

    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    return linkAndAttachStub(cx, masm, ion, "adding", rejoinOffset, &exitOffset);
}

static bool
IsPropertyInlineable(JSObject *obj)
{
    if (!obj->isNative())
        return false;

    if (obj->watched())
        return false;

    return true;
}

static bool
IsPropertySetInlineable(JSContext *cx, HandleObject obj, HandleId id, MutableHandleShape pshape)
{
    UnrootedShape shape = obj->nativeLookup(cx, id);

    if (!shape)
        return false;

    if (!shape->hasSlot())
        return false;

    if (!shape->hasDefaultSetter())
        return false;

    if (!shape->writable())
        return false;

    pshape.set(shape);

    return true;
}

static bool
IsPropertySetterCallInlineable(JSContext *cx, HandleObject obj, HandleObject holder,
                               HandleShape shape)
{
    if (!shape)
        return false;

    if (!holder->isNative())
        return false;

    if (shape->hasSlot())
        return false;

    if (shape->hasDefaultSetter())
        return false;

    if (!shape->writable())
        return false;

    
    
    if (shape->hasSetterValue())
        return false;

    return true;
}

static bool
IsPropertyAddInlineable(JSContext *cx, HandleObject obj, HandleId id, uint32_t oldSlots,
                        MutableHandleShape pShape)
{
    
    if (pShape.get())
        return false;

    RootedShape shape(cx, obj->nativeLookup(cx, id));
    if (!shape || shape->inDictionary() || !shape->hasSlot() || !shape->hasDefaultSetter())
        return false;

    
    if (obj->getClass()->resolve != JS_ResolveStub)
        return false;

    if (!obj->isExtensible() || !shape->writable())
        return false;

    
    
    
    for (JSObject *proto = obj->getProto(); proto; proto = proto->getProto()) {
        
        if (!proto->isNative())
            return false;

        
        UnrootedShape protoShape = proto->nativeLookup(cx, id);
        if (protoShape && !protoShape->hasDefaultSetter())
            return false;

        
        
        if (proto->getClass()->resolve != JS_ResolveStub)
             return false;
    }

    
    
    
    if (obj->numDynamicSlots() != oldSlots)
        return false;

    pShape.set(shape);
    return true;
}

bool
SetPropertyIC::update(JSContext *cx, size_t cacheIndex, HandleObject obj,
                      HandleValue value)
{
    AutoFlushCache afc ("SetPropertyCache");

    void *returnAddr;
    const SafepointIndex *safepointIndex;
    RootedScript script(cx, GetTopIonJSScript(cx, &safepointIndex, &returnAddr));
    IonScript *ion = script->ion;
    SetPropertyIC &cache = ion->getCache(cacheIndex).toSetProperty();
    RootedPropertyName name(cx, cache.name());
    RootedId id(cx, AtomToId(name));
    RootedShape shape(cx);
    RootedObject holder(cx);

    
    
    bool inlinable = cache.canAttachStub() && IsPropertyInlineable(obj);
    bool addedSetterStub = false;
    if (inlinable) {
        RootedShape shape(cx);
        if (IsPropertySetInlineable(cx, obj, id, &shape)) {
            if (!cache.attachNativeExisting(cx, ion, obj, shape))
                return false;
            addedSetterStub = true;
        } else {
            RootedObject holder(cx);
            if (!JSObject::lookupProperty(cx, obj, name, &holder, &shape))
                return false;

            if (IsPropertySetterCallInlineable(cx, obj, holder, shape)) {
                if (!cache.attachSetterCall(cx, ion, obj, holder, shape, returnAddr))
                    return false;
                addedSetterStub = true;
            }
        }
    }

    uint32_t oldSlots = obj->numDynamicSlots();
    RootedShape oldShape(cx, obj->lastProperty());

    
    if (!SetProperty(cx, obj, name, value, cache.strict(), cache.isSetName()))
        return false;

    
    if (inlinable && !addedSetterStub && obj->lastProperty() != oldShape &&
        IsPropertyAddInlineable(cx, obj, id, oldSlots, &shape))
    {
        RootedShape newShape(cx, obj->lastProperty());
        if (!cache.attachNativeAdding(cx, ion, obj, oldShape, newShape, shape))
            return false;
    }

    return true;
}

bool
GetElementIC::attachGetProp(JSContext *cx, IonScript *ion, HandleObject obj,
                            const Value &idval, HandlePropertyName name)
{
    JS_ASSERT(index().reg().hasValue());

    RootedObject holder(cx);
    RootedShape shape(cx);
    if (!JSObject::lookupProperty(cx, obj, name, &holder, &shape))
        return false;

    RootedScript script(cx);
    jsbytecode *pc;
    getScriptedLocation(&script, &pc);

    if (!IsCacheableGetPropReadSlot(obj, holder, shape) &&
        !IsCacheableNoProperty(obj, holder, shape, pc, output())) {
        IonSpew(IonSpew_InlineCaches, "GETELEM uncacheable property");
        return true;
    }

    JS_ASSERT(idval.isString());

    RepatchLabel failures;
    Label nonRepatchFailures;
    MacroAssembler masm;

    
    ValueOperand val = index().reg().valueReg();
    masm.branchTestValue(Assembler::NotEqual, val, idval, &nonRepatchFailures);

    GetNativePropertyStub getprop;
    getprop.generateReadSlot(cx, masm, obj, name, holder, shape, object(), output(), &failures,
                             &nonRepatchFailures);

    return linkAndAttachStub(cx, masm, ion, "property", getprop.rejoinOffset, &getprop.exitOffset);
}

bool
GetElementIC::attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval)
{
    JS_ASSERT(obj->isNative());
    JS_ASSERT(idval.isInt32());

    Label failures;
    MacroAssembler masm;

    Register scratchReg = output().scratchReg().gpr();
    JS_ASSERT(scratchReg != InvalidReg);

    
    RootedObject globalObj(cx, &script->global());
    RootedShape shape(cx, obj->lastProperty());
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object(), shape, &failures);

    
    Register indexReg = InvalidReg;

    if (index().reg().hasValue()) {
        indexReg = output().scratchReg().gpr();
        JS_ASSERT(indexReg != InvalidReg);
        ValueOperand val = index().reg().valueReg();

        masm.branchTestInt32(Assembler::NotEqual, val, &failures);

        
        masm.unboxInt32(val, indexReg);
    } else {
        JS_ASSERT(!index().reg().typedReg().isFloat());
        indexReg = index().reg().typedReg().gpr();
    }

    
    masm.push(object());
    masm.loadPtr(Address(object(), JSObject::offsetOfElements()), object());

    Label hole;

    
    Address initLength(object(), ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, indexReg, &hole);

    
    masm.loadElementTypedOrValue(BaseIndex(object(), indexReg, TimesEight),
                                 output(), true, &hole);

    masm.pop(object());
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&hole);
    masm.pop(object());
    masm.bind(&failures);

    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    setHasDenseStub();
    return linkAndAttachStub(cx, masm, ion, "dense array", rejoinOffset, &exitOffset);
}

bool
GetElementIC::attachTypedArrayElement(JSContext *cx, IonScript *ion, JSObject *obj,
                                      const Value &idval)
{
    JS_ASSERT(obj->isTypedArray());
    JS_ASSERT(idval.isInt32());

    Label failures;
    MacroAssembler masm;

    
    int arrayType = TypedArray::type(obj);

    
    
    bool floatOutput = arrayType == TypedArray::TYPE_FLOAT32 ||
                       arrayType == TypedArray::TYPE_FLOAT64;
    JS_ASSERT_IF(!output().hasValue(), !floatOutput);

    Register tmpReg = output().scratchReg().gpr();
    JS_ASSERT(tmpReg != InvalidReg);

    
    
    masm.branchTestObjClass(Assembler::NotEqual, object(), tmpReg, obj->getClass(), &failures);

    
    Register indexReg = tmpReg;

    JS_ASSERT(!index().constant());
    if (index().reg().hasValue()) {
        ValueOperand val = index().reg().valueReg();
        masm.branchTestInt32(Assembler::NotEqual, val, &failures);

        
        masm.unboxInt32(val, indexReg);
    } else {
        JS_ASSERT(!index().reg().typedReg().isFloat());
        indexReg = index().reg().typedReg().gpr();
    }

    
    Address length(object(), TypedArray::lengthOffset());
    masm.branch32(Assembler::BelowOrEqual, length, indexReg, &failures);

    
    Label popAndFail;
    Register elementReg = object();
    masm.push(object());

    
    masm.loadPtr(Address(object(), TypedArray::dataOffset()), elementReg);

    
    
    int width = TypedArray::slotWidth(arrayType);
    BaseIndex source(elementReg, indexReg, ScaleFromElemWidth(width));
    if (output().hasValue())
        masm.loadFromTypedArray(arrayType, source, output().valueReg(), true,
                                elementReg, &popAndFail);
    else
        masm.loadFromTypedArray(arrayType, source, output().typedReg(),
                                elementReg, &popAndFail);

    masm.pop(object());
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&popAndFail);
    masm.pop(object());
    masm.bind(&failures);

    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    return linkAndAttachStub(cx, masm, ion, "typed array", rejoinOffset, &exitOffset);
}

bool
GetElementIC::update(JSContext *cx, size_t cacheIndex, HandleObject obj,
                     HandleValue idval, MutableHandleValue res)
{
    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    GetElementIC &cache = ion->getCache(cacheIndex).toGetElement();
    RootedScript script(cx);
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);
    RootedValue lval(cx, ObjectValue(*obj));

    if (cache.isDisabled()) {
        if (!GetElementOperation(cx, JSOp(*pc), &lval, idval, res))
            return false;
        types::TypeScript::Monitor(cx, script, pc, res);
        return true;
    }

    
    AutoFlushCache afc ("GetElementCache");
    AutoDetectInvalidation adi(cx, res.address(), ion);

    RootedId id(cx);
    if (!FetchElementId(cx, obj, idval, &id, res))
        return false;

    bool attachedStub = false;
    if (cache.canAttachStub()) {
        if (obj->isNative() && cache.monitoredResult()) {
            uint32_t dummy;
            if (idval.isString() && JSID_IS_ATOM(id) && !JSID_TO_ATOM(id)->isIndex(&dummy)) {
                RootedPropertyName name(cx, JSID_TO_ATOM(id)->asPropertyName());
                if (!cache.attachGetProp(cx, ion, obj, idval, name))
                    return false;
                attachedStub = true;
            }
        } else if (!cache.hasDenseStub() && obj->isNative() && idval.isInt32()) {
            if (!cache.attachDenseElement(cx, ion, obj, idval))
                return false;
            attachedStub = true;
        } else if (obj->isTypedArray() && idval.isInt32()) {
            int arrayType = TypedArray::type(obj);
            bool floatOutput = arrayType == TypedArray::TYPE_FLOAT32 ||
                               arrayType == TypedArray::TYPE_FLOAT64;
            if (!floatOutput || cache.output().hasValue()) {
                if (!cache.attachTypedArrayElement(cx, ion, obj, idval))
                    return false;
                attachedStub = true;
            }
        }
    }

    if (!GetElementOperation(cx, JSOp(*pc), &lval, idval, res))
        return false;

    
    
    if (!attachedStub && !cache.canAttachStub())
        cache.disable();

    types::TypeScript::Monitor(cx, script, pc, res);
    return true;
}

bool
BindNameIC::attachGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain)
{
    JS_ASSERT(scopeChain->isGlobal());

    MacroAssembler masm;

    
    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.branchPtrWithPatch(Assembler::NotEqual, scopeChainReg(),
                                                        ImmGCPtr(scopeChain), &exit_);
    masm.bind(&exit_);
    masm.movePtr(ImmGCPtr(scopeChain), outputReg());

    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    return linkAndAttachStub(cx, masm, ion, "global", rejoinOffset, &exitOffset);
}

static inline void
GenerateScopeChainGuard(MacroAssembler &masm, JSObject *scopeObj,
                        Register scopeObjReg, UnrootedShape shape, Label *failures)
{
    AutoAssertNoGC nogc;
    if (scopeObj->isCall()) {
        
        
        
        CallObject *callObj = &scopeObj->asCall();
        if (!callObj->isForEval()) {
            RawFunction fun = &callObj->callee();
            UnrootedScript script = fun->nonLazyScript();
            if (!script->funHasExtensibleScope)
                return;
        }
    } else if (scopeObj->isGlobal()) {
        
        
        
        if (shape && !shape->configurable())
            return;
    }

    Address shapeAddr(scopeObjReg, JSObject::offsetOfShape());
    masm.branchPtr(Assembler::NotEqual, shapeAddr, ImmGCPtr(scopeObj->lastProperty()), failures);
}

static void
GenerateScopeChainGuards(MacroAssembler &masm, JSObject *scopeChain, JSObject *holder,
                         Register outputReg, Label *failures)
{
    JSObject *tobj = scopeChain;

    
    
    while (true) {
        JS_ASSERT(IsCacheableNonGlobalScope(tobj) || tobj->isGlobal());

        GenerateScopeChainGuard(masm, tobj, outputReg, NULL, failures);
        if (tobj == holder)
            break;

        
        tobj = &tobj->asScope().enclosingScope();
        masm.extractObject(Address(outputReg, ScopeObject::offsetOfEnclosingScope()), outputReg);
    }
}

bool
BindNameIC::attachNonGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain, JSObject *holder)
{
    JS_ASSERT(IsCacheableNonGlobalScope(scopeChain));

    MacroAssembler masm;

    
    RepatchLabel failures;
    Label nonRepatchFailures;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(scopeChainReg(), JSObject::offsetOfShape()),
                                ImmGCPtr(scopeChain->lastProperty()),
                                &failures);

    if (holder != scopeChain) {
        JSObject *parent = &scopeChain->asScope().enclosingScope();
        masm.extractObject(Address(scopeChainReg(), ScopeObject::offsetOfEnclosingScope()), outputReg());

        GenerateScopeChainGuards(masm, parent, holder, outputReg(), &nonRepatchFailures);
    } else {
        masm.movePtr(scopeChainReg(), outputReg());
    }

    
    
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&failures);
    masm.bind(&nonRepatchFailures);
    if (holder != scopeChain) {
        RepatchLabel exit_;
        exitOffset = masm.jumpWithPatch(&exit_);
        masm.bind(&exit_);
    }

    return linkAndAttachStub(cx, masm, ion, "non-global", rejoinOffset, &exitOffset);
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

        scopeChain = &scopeChain->asScope().enclosingScope();
        if (!scopeChain) {
            IonSpew(IonSpew_InlineCaches, "Scope chain indirect hit");
            return false;
        }
    }

    JS_NOT_REACHED("Shouldn't get here");
    return false;
}

JSObject *
BindNameIC::update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain)
{
    AutoFlushCache afc ("BindNameCache");

    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    BindNameIC &cache = ion->getCache(cacheIndex).toBindName();
    HandlePropertyName name = cache.name();

    RootedObject holder(cx);
    if (scopeChain->isGlobal()) {
        holder = scopeChain;
    } else {
        if (!LookupNameWithGlobalDefault(cx, name, scopeChain, &holder))
            return NULL;
    }

    
    
    if (cache.canAttachStub()) {
        if (scopeChain->isGlobal()) {
            if (!cache.attachGlobal(cx, ion, scopeChain))
                return NULL;
        } else if (IsCacheableScopeChain(scopeChain, holder)) {
            if (!cache.attachNonGlobal(cx, ion, scopeChain, holder))
                return NULL;
        } else {
            IonSpew(IonSpew_InlineCaches, "BINDNAME uncacheable scope chain");
        }
    }

    return holder;
}

bool
NameIC::attach(JSContext *cx, IonScript *ion, HandleObject scopeChain, HandleObject holder, HandleShape shape)
{
    AssertCanGC();
    MacroAssembler masm;
    Label failures;

    Register scratchReg = outputReg().valueReg().scratchReg();

    masm.mov(scopeChainReg(), scratchReg);
    GenerateScopeChainGuards(masm, scopeChain, holder, scratchReg, &failures);

    unsigned slot = shape->slot();
    if (holder->isFixedSlot(slot)) {
        Address addr(scratchReg, JSObject::getFixedSlotOffset(slot));
        masm.loadTypedOrValue(addr, outputReg());
    } else {
        masm.loadPtr(Address(scratchReg, JSObject::offsetOfSlots()), scratchReg);

        Address addr(scratchReg, holder->dynamicSlotIndex(slot) * sizeof(Value));
        masm.loadTypedOrValue(addr, outputReg());
    }

    RepatchLabel rejoin;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin);
    masm.bind(&rejoin);

    CodeOffsetJump exitOffset;

    if (failures.used()) {
        masm.bind(&failures);

        RepatchLabel exit;
        exitOffset = masm.jumpWithPatch(&exit);
        masm.bind(&exit);
    }

    return linkAndAttachStub(cx, masm, ion, "generic", rejoinOffset,
                             (failures.bound() ? &exitOffset : NULL));
}

static bool
IsCacheableName(JSContext *cx, HandleObject scopeChain, HandleObject obj, HandleObject holder,
                HandleShape shape, jsbytecode *pc, const TypedOrValueRegister &output)
{
    if (!shape)
        return false;
    if (!obj->isNative())
        return false;
    if (obj != holder)
        return false;

    if (obj->isGlobal()) {
        
        if (!IsCacheableGetPropReadSlot(obj, holder, shape) &&
            !IsCacheableNoProperty(obj, holder, shape, pc, output))
            return false;
    } else if (obj->isCall()) {
        if (!shape->hasDefaultGetter())
            return false;
    } else {
        
        return false;
    }

    RootedObject obj2(cx, scopeChain);
    while (obj2) {
        if (!IsCacheableNonGlobalScope(obj2) && !obj2->isGlobal())
            return false;

        
        if (obj2->isGlobal() || obj2 == obj)
            break;

        obj2 = obj2->enclosingScope();
    }

    return obj == obj2;
}

bool
NameIC::update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain,
               MutableHandleValue vp)
{
    AutoFlushCache afc ("GetNameCache");

    IonScript *ion = GetTopIonJSScript(cx)->ionScript();

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

    if (cache.canAttachStub() &&
        IsCacheableName(cx, scopeChain, obj, holder, shape, pc, cache.outputReg()))
    {
        if (!cache.attach(cx, ion, scopeChain, obj, shape))
            return false;
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
    MacroAssembler masm;

    
    RepatchLabel exit;
    CodeOffsetJump exitOffset = masm.branchPtrWithPatch(Assembler::NotEqual, calleeReg(),
                                                        ImmWord(uintptr_t(original.get())), &exit);
    masm.bind(&exit);

    
    masm.movePtr(ImmWord(uintptr_t(clone.get())), outputReg());

    RepatchLabel rejoin;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin);
    masm.bind(&rejoin);

    return linkAndAttachStub(cx, masm, ion, "generic", rejoinOffset, &exitOffset);
}

JSObject *
CallsiteCloneIC::update(JSContext *cx, size_t cacheIndex, HandleObject callee)
{
    AutoFlushCache afc ("CallsiteCloneCache");

    
    
    RootedFunction fun(cx, callee->toFunction());
    if (!fun->nonLazyScript()->shouldCloneAtCallsite)
        return fun;

    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    CallsiteCloneIC &cache = ion->getCache(cacheIndex).toCallsiteClone();

    RootedFunction clone(cx, CloneFunctionAtCallsite(cx, fun, cache.callScript(), cache.callPc()));
    if (!clone)
        return NULL;

    if (cache.canAttachStub()) {
        if (!cache.attach(cx, ion, fun, clone))
            return NULL;
    }

    return clone;
}

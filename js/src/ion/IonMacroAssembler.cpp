








































#include "jsinfer.h"
#include "jsinferinlines.h"
#include "IonMacroAssembler.h"
#include "MoveEmitter.h"

using namespace js;
using namespace js::ion;

uint32
MacroAssembler::setupABICall(uint32 args, uint32 returnSize, const MoveOperand *returnOperand)
{
    JS_ASSERT(!inCall_);
    JS_ASSERT_IF(returnSize <= sizeof(void *), !returnOperand);
    inCall_ = true;

    callProperties_ = None;
    uint32 stackForArgs = args > NumArgRegs
                          ? (args - NumArgRegs) * sizeof(void *)
                          : 0;
    uint32 stackForRes = 0;

    
    
    
    
    if (returnSize > sizeof(void *)) {
        callProperties_ |= LargeReturnValue;

        
        
        
        
        
        
        
        
        if (args >= NumArgRegs) {
            callProperties_ |= ReturnArgConsumeStack;
            stackForRes = sizeof(void *);
        }

        
        
        
        
        
        

        if (returnOperand) {
            setAnyABIArg(0, *returnOperand);
        } else {
            setAnyABIArg(0, MoveOperand(StackPointer, stackForArgs + stackForRes));
            stackForRes += returnSize;
        }
    }

    return stackForArgs + stackForRes;
}

void
MacroAssembler::setupAlignedABICall(uint32 args, uint32 returnSize, const MoveOperand *returnOperand)
{
    uint32 stackForCall = setupABICall(args, returnSize, returnOperand);

    
    
    
    dynamicAlignment_ = false;
    stackAdjust_ = alignStackForCall(stackForCall);
    reserveStack(stackAdjust_);
}

void
MacroAssembler::setupUnalignedABICall(uint32 args, const Register &scratch, uint32 returnSize,
                                      const MoveOperand *returnOperand)
{
    uint32 stackForCall = setupABICall(args, returnSize, returnOperand);

    
    
    dynamicAlignment_ = true;
    stackAdjust_ = dynamicallyAlignStackForCall(stackForCall, scratch);
    reserveStack(stackAdjust_);
}

void
MacroAssembler::setAnyABIArg(uint32 arg, const MoveOperand &from)
{
    MoveOperand to;
    Register dest;
    if (GetArgReg(arg, &dest)) {
        to = MoveOperand(dest);
    } else {
        
        
        uint32 disp = GetArgStackDisp(arg);
        to = MoveOperand(StackPointer, disp);
    }
    enoughMemory_ &= moveResolver_.addMove(from, to, Move::GENERAL);
}

void
MacroAssembler::callWithABI(void *fun)
{
    JS_ASSERT(inCall_);

    
    if (stackAdjust_ >= sizeof(void *)) {
        enoughMemory_ &= moveResolver_.resolve();
        if (!enoughMemory_)
            return;

        MoveEmitter emitter(*this);
        emitter.emit(moveResolver());
        emitter.finish();
    }

#ifdef DEBUG
    checkCallAlignment();
#endif

    call(fun);
}

void
MacroAssembler::getABIRes(uint32 offset, const MoveOperand &to)
{
    JS_ASSERT(inCall_);

    
    JS_ASSERT_IF(to.isMemory(), to.base().code() & Registers::NonVolatileMask);

    MoveOperand from;

    callProperties_ |= HasGetRes;
    if (callProperties_ & LargeReturnValue) {
        
        from = MoveOperand(ReturnReg, offset);
    } else {
        
        JS_ASSERT(!offset);
        from = MoveOperand(ReturnReg);
    }

    enoughMemory_ &= moveResolver_.addMove(from, to, Move::GENERAL);
}

void
MacroAssembler::finishABICall()
{
    JS_ASSERT(inCall_);

    
    if (callProperties_ & HasGetRes) {
        enoughMemory_ &= moveResolver_.resolve();
        if (!enoughMemory_)
            return;

        MoveEmitter emitter(*this);
        emitter.emit(moveResolver());
        emitter.finish();
    }

    
    
    
    stackAdjust_ -= callProperties_ & ReturnArgConsumeStack ? sizeof(void *) : 0;

    freeStack(stackAdjust_);
    if (dynamicAlignment_)
        restoreStackFromDynamicAlignment();

    JS_ASSERT(inCall_);
    inCall_ = false;
}

template <typename T> void
MacroAssembler::guardTypeSet(const T &address, types::TypeSet *types,
                             Register scratch, Label *mismatched)
{
    JS_ASSERT(!types->unknown());

    Label matched;
    Register tag = extractTag(address, scratch);

    if (types->hasType(types::Type::DoubleType())) {
        
        JS_ASSERT(types->hasType(types::Type::Int32Type()));
        branchTestNumber(Equal, tag, &matched);
    } else if (types->hasType(types::Type::Int32Type())) {
        branchTestInt32(Equal, tag, &matched);
    }

    if (types->hasType(types::Type::UndefinedType()))
        branchTestUndefined(Equal, tag, &matched);
    if (types->hasType(types::Type::BooleanType()))
        branchTestBoolean(Equal, tag, &matched);
    if (types->hasType(types::Type::StringType()))
        branchTestString(Equal, tag, &matched);
    if (types->hasType(types::Type::NullType()))
        branchTestNull(Equal, tag, &matched);

    if (types->hasType(types::Type::AnyObjectType())) {
        branchTestObject(Equal, tag, &matched);
    } else if (types->getObjectCount()) {
        branchTestObject(NotEqual, tag, mismatched);
        Register obj = extractObject(address, scratch);

        Label notSingleton;
        branchTest32(Zero, Address(obj, offsetof(JSObject, flags)),
                     Imm32(JSObject::SINGLETON_TYPE), &notSingleton);

        unsigned count = types->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                branchPtr(Equal, obj, ImmGCPtr(object), &matched);
        }
        jump(mismatched);

        bind(&notSingleton);
        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);
        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                branchPtr(Equal, obj, ImmGCPtr(object), &matched);
        }
    }

    jump(mismatched);
    bind(&matched);
}

template void MacroAssembler::guardTypeSet(const Address &address, types::TypeSet *types,
                                           Register scratch, Label *mismatched);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, types::TypeSet *types,
                                           Register scratch, Label *mismatched);

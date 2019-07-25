








































#include "jsinfer.h"
#include "jsinferinlines.h"
#include "IonMacroAssembler.h"
#include "MoveEmitter.h"

using namespace js;
using namespace js::ion;

void
MacroAssembler::setupAlignedABICall(uint32 args)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;

    uint32 stackForArgs = args > NumArgRegs
                          ? (args - NumArgRegs) * sizeof(void *)
                          : 0;

    
    
    stackAdjust_ = alignStackForCall(stackForArgs);
    dynamicAlignment_ = false;
    reserveStack(stackAdjust_);
}

void
MacroAssembler::setupUnalignedABICall(uint32 args, const Register &scratch)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;

    uint32 stackForArgs = args > NumArgRegs
                          ? (args - NumArgRegs) * sizeof(void *)
                          : 0;

    
    
    
    stackAdjust_ = dynamicallyAlignStackForCall(stackForArgs, scratch);
    dynamicAlignment_ = true;
    reserveStack(stackAdjust_);
}

void
MacroAssembler::setABIArg(uint32 arg, const MoveOperand &from)
{
    MoveOperand to;
    Register dest;
    if (GetArgReg(arg, &dest))
        to = MoveOperand(dest);
    else
    {
        
        
        uint32 disp = GetArgStackDisp(arg);
        to = MoveOperand(StackPointer, disp);
    }
    enoughMemory_ &= moveResolver_.addMove(from, to, Move::GENERAL);
}

void
MacroAssembler::callWithABI(void *fun)
{
    if (NumArgRegs) {
        
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

    
    
    movePtr(ImmWord(fun), CallReg);
    call(CallReg);

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










































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
MacroAssembler::setABIArg(uint32 arg, const Register &reg)
{
    Register dest;
    if (!GetArgReg(arg, &dest)) {
        
        
        setStackArg(reg, arg);
        return;
    }
    enoughMemory_ &= moveResolver_.addMove(MoveOperand(reg), MoveOperand(dest), Move::GENERAL);
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

    
    
    movePtr(ImmWord(fun), ReturnReg);
    call(ReturnReg);

    freeStack(stackAdjust_);
    if (dynamicAlignment_)
        restoreStackFromDynamicAlignment();

    JS_ASSERT(inCall_);
    inCall_ = false;
}


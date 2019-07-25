








































#include "MacroAssembler-x64.h"
#include "ion/MoveEmitter.h"
#include "ion/IonFrames.h"

using namespace js;
using namespace js::ion;

uint32
MacroAssemblerX64::setupABICall(uint32 args)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;

    uint32 stackForArgs = args > NumArgRegs
                          ? (args - NumArgRegs) * sizeof(void *)
                          : 0;
    return stackForArgs;
}

static const uint32 StackAlignment = 16;

void
MacroAssemblerX64::setupAlignedABICall(uint32 args)
{
    
    
    
    uint32 stackForCall = setupABICall(args);
    uint32 total = stackForCall + ShadowStackSpace;
    uint32 displacement = total + framePushed_;

    stackAdjust_ = total + ComputeByteAlignment(displacement, StackAlignment);
    dynamicAlignment_ = false;
    reserveStack(stackAdjust_);
}

void
MacroAssemblerX64::setupUnalignedABICall(uint32 args, const Register &scratch)
{
    uint32 stackForCall = setupABICall(args);

    
    
    
    
    movq(rsp, scratch);
    andq(Imm32(~(StackAlignment - 1)), rsp);
    push(scratch);

    uint32 total = stackForCall + ShadowStackSpace;
    uint32 displacement = total + STACK_SLOT_SIZE;

    stackAdjust_ = total + ComputeByteAlignment(displacement, StackAlignment);
    dynamicAlignment_ = true;
    reserveStack(stackAdjust_);
}

void
MacroAssemblerX64::setABIArg(uint32 arg, const MoveOperand &from)
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
MacroAssemblerX64::setABIArg(uint32 arg, const Register &reg)
{
    setABIArg(arg, MoveOperand(reg));
}

void
MacroAssemblerX64::callWithABI(void *fun)
{
    JS_ASSERT(inCall_);

    
    {
        enoughMemory_ &= moveResolver_.resolve();
        if (!enoughMemory_)
            return;

        MoveEmitter emitter(*this);
        emitter.emit(moveResolver_);
        emitter.finish();
    }

#ifdef DEBUG
    {
        Label good;
        movl(rsp, rax);
        testq(rax, Imm32(StackAlignment - 1));
        j(Equal, &good);
        breakpoint();
        bind(&good);
    }
#endif

    call(ImmWord(fun));

    freeStack(stackAdjust_);
    if (dynamicAlignment_)
        pop(rsp);

    JS_ASSERT(inCall_);
    inCall_ = false;
}

void
MacroAssemblerX64::handleException()
{
    
    subq(Imm32(sizeof(ResumeFromException)), rsp);
    movq(rsp, rax);

    
    setupUnalignedABICall(1, rcx);
    setABIArg(0, rax);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, ion::HandleException));

    
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    movq(Operand(rsp, offsetof(ResumeFromException, stackPointer)), rsp);
    ret();
}

